module wave_table_synth (
    input  logic        clk,
    input  logic        reset,
    input  logic [15:0] writedata,
    output logic [15:0] readdata,
    input  logic        write,
    input  logic        chipselect,
    /* verilator lint_off UNUSEDSIGNAL */
    input  logic [17:0] address,
    /* verilator lint_on UNUSEDSIGNAL */
    input  logic        ready_left,
    input  logic        ready_right,
    output logic [15:0] sample,
    output logic        sample_valid
);

    localparam int NUM_VOICES      = 8;
    localparam int WAVETABLE_DEPTH = 12 * 4096; // 12 slots x 4096 samples

    // ─── Address decode ─────────────────────────────────────────
    // address[17]   = 0: control registers, 1: wavetable data
    // Control: address[4:2] = voice (0-7), address[1:0] = register
    // Wavetable: address[16:12] = slot, address[11:0] = sample index
    wire        is_wavetable = address[17];
    wire [2:0]  voice_sel    = address[4:2];
    wire [1:0]  reg_sel      = address[1:0];

    // ─── Per-voice control registers ────────────────────────────
    logic        note_on_reg    [0:NUM_VOICES-1];
    logic [15:0] step_size_reg  [0:NUM_VOICES-1];
    logic [6:0]  velocity_reg   [0:NUM_VOICES-1];
    logic [3:0]  slot_select_reg[0:NUM_VOICES-1];

    always_ff @(posedge clk) begin
        if (reset) begin
            for (int i = 0; i < NUM_VOICES; i++) begin
                note_on_reg[i]     <= 1'b0;
                step_size_reg[i]   <= 16'd0;
                velocity_reg[i]    <= 7'd0;
                slot_select_reg[i] <= 4'd0;
            end
        end else if (chipselect && write && !is_wavetable) begin
            case (reg_sel)
                2'd0: note_on_reg[voice_sel]     <= writedata[0];
                2'd1: step_size_reg[voice_sel]   <= writedata;
                2'd2: velocity_reg[voice_sel]    <= writedata[6:0];
                2'd3: slot_select_reg[voice_sel] <= writedata[3:0];
            endcase
        end
    end

    // ─── Wavetable BRAM (dual-port) ────────────────────────────
    // Port A: Avalon write + registered read (for debug readback)
    // Port B: DDS read (time-multiplexed across voices)
    logic [15:0] wavetable_mem [0:WAVETABLE_DEPTH-1];

    logic [15:0] avalon_rdata;
    logic [15:0] dds_read_addr;
    logic [15:0] dds_rdata;

    always_ff @(posedge clk) begin
        if (chipselect && write && is_wavetable)
            wavetable_mem[address[15:0]] <= writedata;
        avalon_rdata <= wavetable_mem[address[15:0]];
        dds_rdata    <= wavetable_mem[dds_read_addr];
    end

    // ─── Avalon readback mux ────────────────────────────────────
    logic        is_wavetable_r;
    logic [15:0] ctrl_rdata;

    always_ff @(posedge clk) begin
        is_wavetable_r <= is_wavetable;
        case (reg_sel)
            2'd0: ctrl_rdata <= {15'b0, note_on_reg[voice_sel]};
            2'd1: ctrl_rdata <= step_size_reg[voice_sel];
            2'd2: ctrl_rdata <= {9'b0, velocity_reg[voice_sel]};
            2'd3: ctrl_rdata <= {12'b0, slot_select_reg[voice_sel]};
        endcase
    end

    assign readdata = is_wavetable_r ? avalon_rdata : ctrl_rdata;

    // ─── Per-voice phase accumulators (20-bit) ──────────────────
    // Top 12 bits [19:8] = wavetable index, bottom 8 bits = fractional
    logic [19:0] phase_acc [0:NUM_VOICES-1];

    // ─── Time-multiplexed DDS engine ────────────────────────────
    // States: IDLE → READ → WAIT → ACC (×8 voices) → DONE
    // WAIT lets the registered BRAM output settle before ACC uses it.
    // sample_valid pulses high for exactly one cycle per sample; the
    // S_IDLE gate (!sample_valid) throttles the next compute so we don't
    // push duplicates while the Avalon-ST sink's ready is high.
    wire sink_ready = ready_left & ready_right;
    typedef enum logic [2:0] {
        S_IDLE, S_READ, S_WAIT, S_ACC, S_DONE
    } state_t;
    state_t state;

    logic [2:0]  voice_idx;
    logic signed [18:0] mix_acc;

    /* verilator lint_off UNUSEDSIGNAL */
    wire signed [23:0] voice_product = $signed(dds_rdata)
                                     * $signed({1'b0, velocity_reg[voice_idx]});
    /* verilator lint_on UNUSEDSIGNAL */
    wire signed [15:0] voice_scaled = voice_product[22:7];

    always_ff @(posedge clk) begin
        if (reset) begin
            state       <= S_IDLE;
            voice_idx   <= 3'd0;
            mix_acc     <= 19'sd0;
            sample      <= 16'd0;
            sample_valid <= 1'b0;
            for (int i = 0; i < NUM_VOICES; i++)
                phase_acc[i] <= 20'd0;
        end else begin
            sample_valid <= 1'b0;

            case (state)
                S_IDLE: begin
                    // Start a new sample when the sink can accept one and
                    // we don't have a pending sample waiting for handshake.
                    if (sink_ready && !sample_valid) begin
                        voice_idx <= 3'd0;
                        mix_acc   <= 19'sd0;
                        state     <= S_READ;
                    end
                end

                S_READ: begin
                    dds_read_addr <= {slot_select_reg[voice_idx],
                                      phase_acc[voice_idx][19:8]};
                    state <= S_WAIT;
                end

                S_WAIT: begin
                    state <= S_ACC;
                end

                S_ACC: begin
                    if (note_on_reg[voice_idx]) begin
                        mix_acc <= mix_acc + {{3{voice_scaled[15]}}, voice_scaled};
                        phase_acc[voice_idx] <= phase_acc[voice_idx]
                                              + {4'd0, step_size_reg[voice_idx]};
                    end

                    if (voice_idx == 3'd7) begin
                        state <= S_DONE;
                    end else begin
                        voice_idx <= voice_idx + 3'd1;
                        state     <= S_READ;
                    end
                end

                S_DONE: begin
                    sample       <= mix_acc[18:3];
                    sample_valid <= 1'b1;
                    state        <= S_IDLE;
                end

                default: state <= S_IDLE;
            endcase

            // Reset phase accumulator when voice is turned off
            for (int i = 0; i < NUM_VOICES; i++) begin
                if (!note_on_reg[i])
                    phase_acc[i] <= 20'd0;
            end
        end
    end

endmodule
