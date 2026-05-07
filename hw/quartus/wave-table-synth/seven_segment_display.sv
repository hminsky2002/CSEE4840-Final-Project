module seven_segment_display (
    input  logic        clk,
    input  logic        reset,
    input  logic        write,
    input  logic [2:0]  addr,
    input  logic [6:0]  data,
    output logic [6:0]  hex0,
    output logic [6:0]  hex1,
    output logic [6:0]  hex2,
    output logic [6:0]  hex3,
    output logic [6:0]  hex4,
    output logic [6:0]  hex5
);

    logic [6:0] hex_reg [0:5];

    always_ff @(posedge clk) begin
        if (reset) begin
            for (int i = 0; i < 6; i++) begin
                hex_reg[i] <= 7'h7F;
            end
        end else if (write && addr < 3'd6) begin
            hex_reg[addr] <= data;
        end
    end

    assign hex0 = hex_reg[0];
    assign hex1 = hex_reg[1];
    assign hex2 = hex_reg[2];
    assign hex3 = hex_reg[3];
    assign hex4 = hex_reg[4];
    assign hex5 = hex_reg[5];

endmodule
