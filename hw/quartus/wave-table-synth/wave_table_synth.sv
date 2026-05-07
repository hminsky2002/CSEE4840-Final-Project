module wave_table_synth (
    input  logic        clk,
    input logic        reset,
    input logic [15:0]  writedata,
	output logic [15:0]  readdata,
    input logic        write,
    input logic        chipselect,
    input logic [17:0]  address,
    input  logic        ready_left,
    input  logic        ready_right,
    output logic [15:0] sample,
    output logic        sample_valid,
    output logic [6:0]  hex0,
    output logic [6:0]  hex1,
    output logic [6:0]  hex2,
    output logic [6:0]  hex3,
    output logic [6:0]  hex4,
    output logic [6:0]  hex5
);


    wire in_wavetable = (address[17] == 1'b0);
    wire in_osc_region = (address[17] == 1'b1);
    wire in_osc_registers = in_osc_region && (address[16:7] == 10'h000);
    wire in_hex_registers = in_osc_region && (address[16:8] == 9'h001);

    wire [4:0] osc_addr = address[6:2];
    wire [1:0] reg_addr = address[1:0];
    wire [2:0] hex_addr = address[2:0];

    seven_segment_display u_seven_segment (
        .clk    (clk),
        .reset  (reset),
        .write  (chipselect && write && in_hex_registers),
        .addr   (hex_addr),
        .data   (writedata[6:0]),
        .hex0   (hex0),
        .hex1   (hex1),
        .hex2   (hex2),
        .hex3   (hex3),
        .hex4   (hex4),
        .hex5   (hex5)
    );


    (* ramstyle = "M10K" *) logic [15:0] wavetable_bram [0:131071];


    logic [16:0] bram_raddr;
    logic [15:0] bram_rdata;

    always_ff @(posedge clk) begin
        if (chipselect && write && in_wavetable) begin
            wavetable_bram[address[16:0]] <= writedata;
        end
        bram_rdata <= wavetable_bram[bram_raddr];
    end

    logic [15:0] step_size_reg [0:31];
    logic [1:0] ctrl_reg [0:31];
    logic [1:0] table_sel_reg [0:31];
    logic [15:0] amp_reg [0:31];

    always_ff @(posedge clk) begin
        if (reset) begin
            for(int i = 0; i < 32; i++) begin
                step_size_reg[i] <= 16'h0;
                ctrl_reg[i] <= 2'b00;
                table_sel_reg[i] <= 2'h0;
                amp_reg[i] <= 16'hFFFF;
            end
        end else if (chipselect && write && in_osc_registers) begin
            case (reg_addr)
                2'd0: step_size_reg[osc_addr] <= writedata;
                2'd1: ctrl_reg[osc_addr] <= writedata[1:0];
                2'd2: table_sel_reg[osc_addr] <= writedata[1:0];
                2'd3: amp_reg[osc_addr] <= writedata;
            endcase
        end
    end



    wire sink_ready = ready_left & ready_right;

    logic sweep_active;
    logic sweep_done;

    wire sample_tick = sink_ready && !sweep_active && !sample_valid;

    always_ff @(posedge clk) begin
        if(reset) begin
            sweep_active <= 1'b0;
        end else if (sample_tick) begin 
            sweep_active <= 1'b1;
        end else if (sweep_done) begin 
            sweep_active <= 1'b0;
        end 
    end

    logic [15:0] osc_sample;
    logic [4:0]  osc_idx;
    logic        osc_valid;

    oscillator u_oscillator (
        .clk          (clk),
        .rst          (reset),
        .sample_tick  (sample_tick),
        .step_size    (step_size_reg),
        .table_sel    (table_sel_reg),
        .ctrl         (ctrl_reg),
        .amp          (amp_reg),
        .bram_rdata   (bram_rdata),
        .bram_raddr   (bram_raddr),
        .osc_sample (osc_sample),
        .osc_idx    (osc_idx),
        .osc_valid  (osc_valid),
        .sweep_done   (sweep_done)
    );

    logic signed [31:0] mix_acc;

    always_ff @(posedge clk) begin
        if(reset) begin 
            mix_acc <= '0;
            sample <= 16'h0;
        end else begin 
            if (sample_tick) begin 
                mix_acc <= '0;
            end else if (osc_valid) begin 
                //sign extension nonsense that i don't understand
                mix_acc <= mix_acc + $signed({{16{osc_sample[15]}}, osc_sample});
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

    always_ff @(posedge clk) begin
        if(reset) begin 
            sample_valid <= 1'b0;
        end else begin 
            sample_valid <= sweep_done;
        end
    end
    

endmodule
