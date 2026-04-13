module oscillator #(
    parameter TABLE_DEPTH = 48000,
    parameter STEP_WIDTH  = 16
) (
    input  logic                   clk,
    input  logic                   rst,
    input  logic [STEP_WIDTH-1:0]  step,
    input  logic                   sample_en,

    output logic [15:0]            wavetable_addr,
    output logic                   valid
);
    logic [15:0] phase_acc;

    always_ff @(posedge clk) begin
        if (rst) begin
            phase_acc <= '0;
            valid     <= '0;
        end else begin
            valid <= '0;
            if (sample_en) begin
                if (phase_acc >= TABLE_DEPTH - step)
                    phase_acc <= phase_acc - (TABLE_DEPTH - step);
                else
                    phase_acc <= phase_acc + step;

                valid <= 1'b1;
            end
        end
    end

    assign wavetable_addr = phase_acc;

endmodule
