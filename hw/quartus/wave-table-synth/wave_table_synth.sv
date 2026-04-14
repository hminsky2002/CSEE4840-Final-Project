module wave_table_synth (
    input  logic        clk,
    input logic        reset,
    input logic [15:0]  writedata,
	output logic [15:0]  readdata,
    input logic        write,
    input logic        chipselect,
    input logic [2:0]  address,
    input  logic        ready_left,
    input  logic        ready_right,
    output logic [15:0] sample,
    output logic        sample_valid
);

    logic [31:0] clk_div;
    logic        enable_reg;

    always_ff @(posedge clk) begin
        if (reset) begin
            enable_reg <= 1'b0;
        end else if (chipselect && write) begin
            case (address)
                3'h0 : enable_reg <= writedata[0];
            endcase
        end
    end

    assign readdata = 16'b0;

    always_ff @(posedge clk) begin
        if (enable_reg && clk_div >= 56818 - 1) begin
            clk_div      <= 0;
            sample       <= sample[15] ? 16'h1FFF : 16'h8001;
        end else if (!enable_reg) begin
            clk_div      <= 0;
            sample       <= 16'h0000;
        end else begin
            clk_div      <= clk_div + 1;
        end
        sample_valid <= ready_left & ready_right;
    end

endmodule
