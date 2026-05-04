module oscillator(
    input logic clk,
    input logic rst,
    input logic sample_tick,
    input logic [15:0] step_size[0:31],
    input logic [1:0] table_sel[0:31],
    input logic [1:0] ctrl [0:31],
    input logic [15:0] bram_rdata,
    output logic [14:0] bram_raddr,
    output logic [15:0] osc_sample,
    output logic [4:0] osc_idx,
    output logic osc_valid,
    output logic sweep_done 
);
    typedef enum logic { IDLE, RUNNING } state_t;
    state_t state;
    logic [5:0] step;


    logic [23:0] phase [0:31];

    wire [4:0] osc_to_read = step[4:0] - 5'd2;

    always_ff @(posedge clk) begin 
        if (rst) begin 
            state <= IDLE;
            step <= 6'd0;
            osc_valid <= 1'b0;
            sweep_done <= 1'b0;
            osc_sample <= 16'h0;
            osc_idx <= 5'h0;
            bram_raddr <= 15'h0;
            for (int i = 0; i < 32; i++) begin 
                phase[i] <= 24'h0;
            end
        end else begin 
            osc_valid <= 1'b0;
            sweep_done <= 1'b0;

            unique case (state)
                IDLE: begin     
                    if(sample_tick) begin
                        state <= RUNNING;
                        step <= 6'd0;
                    end 
                end

                RUNNING: begin
                    if(step < 6'd32) begin
                        bram_raddr <= { table_sel[step[4:0]], phase[step[4:0]][23:11]};
                    end

                    if (step > 6'd1) begin 
                        osc_idx <= osc_to_read;
                        osc_valid <= 1'b1;
                        if(ctrl[osc_to_read] == 2'b10) begin 
                            osc_sample <= bram_rdata;
                        end else begin
                            osc_sample <= 16'h0;
                        end

                    if (ctrl[osc_to_read] == 2'b11) begin
                            phase[osc_to_read] <= 24'h0;
                        end else if (ctrl[osc_to_read] == 2'b10) begin
                            phase[osc_to_read] <= phase[osc_to_read] + { step_size[osc_to_read], 8'h0 };
                        end
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