/*
 * Top-level WaveSURFER peripheral.
 *
 * Avalon-MM slave: 17-bit word address (256 KB byte window)
 *   Word addr 0x00000 - 0x07FFF : wavetable BRAM  (16 slots x 2048 x 16-bit)
 *   Word addr 0x08000 - 0x0807F : per-oscillator control file (32 x 4 words)
 *   Word addr 0x08080           : AMP_CTRL (low 7 bits used)
 *
 * Per-oscillator register layout (4 words = 8 bytes, word-addressed):
 *   +0 step_size      (16-bit, Q11.5 fixed-point)
 *   +1 control        (low 2 bits: 01=stop, 10=start, 11=reset)
 *   +2 table_index    (low 4 bits: which of 16 slots)
 *   +3 reserved
 *
 * Avalon-ST audio source out: one mixed sample per codec sample window.
 */
module wave_table_synth (
    input  logic         clk,
    input  logic         reset,
    input  logic [15:0]  writedata,
    output logic [15:0]  readdata,
    input  logic         write,
    input  logic         chipselect,
    input  logic [16:0]  address,
    input  logic         ready_left,
    input  logic         ready_right,
    output logic [15:0]  sample,
    output logic         sample_valid
);

    /* ---------------- Address-region decode ---------------- */
    wire in_wavetable  = (address[16:15] == 2'b00);
    wire in_osc_region = (address[16:15] == 2'b01);
    wire in_osc_file   = in_osc_region && (address[14:7] == 8'h00);
    wire in_amp_ctrl   = in_osc_region && (address[14:0] == 15'h0080);

    wire [4:0] voice_addr = address[6:2];
    wire [1:0] reg_addr   = address[1:0];

    /* ---------------- Wavetable BRAM ---------------- */
    (* ramstyle = "M10K" *) logic [15:0] wavetable_mem [0:32767];

    logic [14:0] bram_raddr;
    logic [15:0] bram_rdata;

    /* Write + read in a single always_ff block so Quartus infers a
     * true dual-port M10K. Splitting these into two blocks caused
     * Quartus to infer two disjoint memories (silent bug: reads always
     * returned 0). */
    always_ff @(posedge clk) begin
        if (chipselect && write && in_wavetable) begin
            wavetable_mem[address[14:0]] <= writedata;
        end
        bram_rdata <= wavetable_mem[bram_raddr];
    end

    /* ---------------- Per-oscillator control registers ---------------- */
    logic [15:0] step_size_reg [0:31];
    logic [1:0]  ctrl_reg      [0:31];
    logic [3:0]  table_sel_reg [0:31];
    logic [6:0]  amp_ctrl_reg;

    always_ff @(posedge clk) begin
        if (reset) begin
            for (int i = 0; i < 32; i++) begin
                step_size_reg[i] <= 16'h0;
                ctrl_reg[i]      <= 2'b00;
                table_sel_reg[i] <= 4'h0;
            end
            amp_ctrl_reg <= 7'h0;
        end else if (chipselect && write) begin
            if (in_osc_file) begin
                case (reg_addr)
                    2'd0: step_size_reg[voice_addr] <= writedata;
                    2'd1: ctrl_reg[voice_addr]      <= writedata[1:0];
                    2'd2: table_sel_reg[voice_addr] <= writedata[3:0];
                    2'd3: ; /* reserved */
                endcase
            end else if (in_amp_ctrl) begin
                amp_ctrl_reg <= writedata[6:0];
            end
        end
    end

    assign readdata = 16'h0;

    /* ---------------- Sample-tick generator ----------------
     * Free-running 50 MHz -> ~48.03 kHz divider (slightly faster than
     * the codec's 48 kHz). The sink backpressures us via sample_pending
     * so we don't overwrite a sample that hasn't been accepted yet; the
     * audio IP's FIFO paces us to exactly 48 kHz on average.
     */
    localparam int SAMPLE_DIV = 1041;  // 50_000_000 / 1041 = 48031.7 Hz
    logic [10:0] tick_cnt;
    always_ff @(posedge clk) begin
        if (reset) begin
            tick_cnt <= 11'd0;
        end else if (tick_cnt == SAMPLE_DIV - 1) begin
            tick_cnt <= 11'd0;
        end else begin
            tick_cnt <= tick_cnt + 11'd1;
        end
    end
    wire sample_tick = (tick_cnt == SAMPLE_DIV - 1) && !sample_pending;

    /* ---------------- Oscillator TDM bank ---------------- */
    logic [15:0] voice_sample;
    logic [4:0]  voice_idx;
    logic        voice_valid;
    logic        sweep_done;

    oscillator u_oscillator (
        .clk          (clk),
        .rst          (reset),
        .sample_tick  (sample_tick),
        .step_size    (step_size_reg),
        .table_sel    (table_sel_reg),
        .ctrl         (ctrl_reg),
        .bram_rdata   (bram_rdata),
        .bram_raddr   (bram_raddr),
        .voice_sample (voice_sample),
        .voice_idx    (voice_idx),
        .voice_valid  (voice_valid),
        .sweep_done   (sweep_done)
    );

    /* ---------------- Minimal mix placeholder (Phase 2 = real mixer) ---------------- */
    logic signed [31:0] mix_acc;

    always_ff @(posedge clk) begin
        if (reset) begin
            mix_acc <= '0;
            sample  <= 16'h0;
        end else begin
            if (sample_tick) begin
                mix_acc <= '0;
            end else if (voice_valid) begin
                mix_acc <= mix_acc + $signed({{16{voice_sample[15]}}, voice_sample});
            end

            if (sweep_done) begin
                if (mix_acc > 32'sd32767) begin
                    sample <= 16'h7FFF;
                end else if (mix_acc < -32'sd32768) begin
                    sample <= 16'h8000;
                end else begin
                    sample <= mix_acc[15:0];
                end
            end
        end
    end

    /* ---------------- Avalon-ST handshake ----------------
     * Raise sample_pending when the mixer latches a new sample; drop it
     * the cycle the sink accepts it (valid && ready_left && ready_right).
     * This produces exactly one valid assertion per mixed sample.
     */
    logic sample_pending;
    always_ff @(posedge clk) begin
        if (reset) begin
            sample_pending <= 1'b0;
        end else if (sweep_done) begin
            sample_pending <= 1'b1;
        end else if (sample_pending && ready_left && ready_right) begin
            sample_pending <= 1'b0;
        end
    end
    assign sample_valid = sample_pending;

endmodule
