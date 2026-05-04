module wave_table_synth (
    input  logic        clk,
    input logic        reset,
    input logic [15:0]  writedata,
	output logic [15:0]  readdata,
    input logic        write,
    input logic        chipselect,
    input logic [16:0]  address,
    input  logic        ready_left,
    input  logic        ready_right,
    output logic [15:0] sample,
    output logic        sample_valid
);


    wire in_wavetable = (address[16:15] == 2'b00);
    wire in_osc_region = (address[16:15] == 2'b01);
    wire in_osc_registers = in_osc_region && (address[14:7] == 8'h00);

    wire [4:0] osc_addr = address[6:2];
    wire [1:0] reg_addr = address[1:0];


    (* ramstyle = "M10K" *) logic [15:0] wavetable_bram [0:32767];


    logic [14:0] bram_raddr;
    logic [15:0] bram_rdata;

    always_ff @(posedge clk) begin
        if (chipselect && write && in_wavetable) begin
            wavetable_mem[address[14:0]] <= writedata;
        end
        bram_rdata <= wavetable_mem[bram_raddr];
    end

    logic [15:0] step_size_reg [0:31];
    logic [1:0] ctrl_reg [0:31];
    logic [1:0] table_sel_reg [0:31];

    always_ff @(posedge clk) begin
        if (reset) begin
            for(int i = 0; i < 32; i++) begin 
                step_size_reg[i] <= 16h'0;
                ctrl_reg[i] <= 2'b00;
                table_sel_reg[i] <= 2'h0;
            end
        end else if (chipselect && write && in_osc_registers) begin
            case (reg_addr)
                2'd0: step_size_reg[osc_addr] <= writedata;
                2'd1: ctrl_reg[osc_addr] <= writedata[1:0];
                2'd2: table_sel_reg[voice_addr] <= writedata[1:0];
                2'd3: ; /* nada */
            endcase
        end
    end



    wire sink_ready = ready_left & ready_right;

    logic sweep_active;
    logic sweep_done;

    wire sample_tick = sink_ready && !sweep_active_active && !sample_valid;

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
        .bram_rdata   (bram_rdata),
        .bram_raddr   (bram_raddr),
        .voice_sample (osc_sample),
        .voice_idx    (osc_idx),
        .voice_valid  (osc_valid),
        .sweep_done   (sweep_done)
    );


endmodule
