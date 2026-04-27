/*
 * Time-multiplexed 32-voice oscillator bank.
 *
 * On each sample_tick pulse, walks through all 32 voices, presenting the
 * BRAM read address for each voice on one clock and latching the returned
 * sample on the next. The phase accumulator for each voice is 24-bit; the
 * top 13 bits index an 8192-entry wavetable (concatenated with a 2-bit slot
 * select to form a 15-bit BRAM address). BRAM is carved into 4 slots.
 *
 * step_size is interpreted as Q11.5 fixed-point and left-shifted by 8 before
 * being added to the 24-bit phase so that it lines up as Q11.13.
 *
 *   software step_size formula:  round(frequency * 2^16 / sample_rate)
 *
 *   per-tick phase update:       phase += { step_size, 8'h0 }
 *
 * ctrl encoding (per design doc):
 *   2'b00 - idle (treat as stop; phase held, voice outputs 0)
 *   2'b01 - stop (phase held, voice outputs 0)
 *   2'b10 - start (phase advances, voice output = BRAM read)
 *   2'b11 - reset (phase forced to 0, voice outputs 0)
 */
module oscillator (
    input  logic         clk,
    input  logic         rst,
    input  logic         sample_tick,
    input  logic [15:0]  step_size [0:31],
    input  logic [1:0]   table_sel [0:31],
    input  logic [1:0]   ctrl      [0:31],
    input  logic [15:0]  bram_rdata,
    output logic [14:0]  bram_raddr,
    output logic [15:0]  voice_sample,
    output logic [4:0]   voice_idx,
    output logic         voice_valid,
    output logic         sweep_done
);

    typedef enum logic { IDLE, RUNNING } state_t;
    state_t state;

    logic [5:0] step;

    logic [23:0] phase [0:31];

    /* Voice whose sample is valid in bram_rdata this cycle. For step=2 this
     * is 0; for step=33 it is 31 (mod-32 wrap via 5-bit subtract). */
    wire [4:0] consume_v = step[4:0] - 5'd2;

    always_ff @(posedge clk) begin
        if (rst) begin
            state        <= IDLE;
            step         <= 6'd0;
            voice_valid  <= 1'b0;
            sweep_done   <= 1'b0;
            voice_sample <= 16'h0;
            voice_idx    <= 5'h0;
            bram_raddr   <= 15'h0;
            for (int i = 0; i < 32; i++) begin
                phase[i] <= 24'h0;
            end
        end else begin
            voice_valid <= 1'b0;
            sweep_done  <= 1'b0;

            unique case (state)
                IDLE: begin
                    if (sample_tick) begin
                        state <= RUNNING;
                        step  <= 6'd0;
                    end
                end

                RUNNING: begin
                    /* Present BRAM address for voice `step` (when still in range). */
                    if (step < 6'd32) begin
                        bram_raddr <= { table_sel[step[4:0]], phase[step[4:0]][23:11] };
                    end

                    /* Two cycles after address presentation, bram_rdata holds that
                     * voice's sample. Consume voice (step - 2) and advance its
                     * phase accumulator. */
                    if (step > 6'd1) begin
                        voice_idx   <= consume_v;
                        voice_valid <= 1'b1;
                        if (ctrl[consume_v] == 2'b10) begin
                            voice_sample <= bram_rdata;
                        end else begin
                            voice_sample <= 16'h0;
                        end

                        if (ctrl[consume_v] == 2'b11) begin
                            phase[consume_v] <= 24'h0;
                        end else if (ctrl[consume_v] == 2'b10) begin
                            phase[consume_v] <= phase[consume_v] + { step_size[consume_v], 8'h0 };
                        end
                        /* ctrl == 2'b00 or 2'b01 -> hold phase */
                    end

                    if (step == 6'd33) begin
                        sweep_done <= 1'b1;
                        state      <= IDLE;
                    end else begin
                        step <= step + 6'd1;
                    end
                end
            endcase
        end
    end

endmodule
