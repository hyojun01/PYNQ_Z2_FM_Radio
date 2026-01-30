//--------------------------------------------------------------------------------
// Verilog Testbench for design_1_wrapper
//
// 1. LFM (Chirp) 신호 생성 (real/imag)
// 2. 12-bit Signed Int 변환
// 3. 16-bit Sign-Extension
// 4. AXI4-Stream 입력 전송
// 5. AXI4-Stream 출력 수신 및 출력
//--------------------------------------------------------------------------------
`timescale 1 ps / 1 ps

module tb_fm_radio_stream;

    // 시뮬레이션 파라미터
    localparam CLK_PERIOD      = 10000; // 100 MHz (10,000 ps = 10 ns)
    localparam NUM_SAMPLES     = 16401;  // 전송할 총 샘플 수
    localparam C_DATA_WIDTH_IN = 8;
    localparam C_DATA_WIDTH_OUT= 16;

    // DUT 연결을 위한 reg 및 wire
    reg  ap_clk;
    reg  ap_rst_n;

    // AXI-Stream Inputs (TB -> DUT)
    reg  [C_DATA_WIDTH_IN-1:0] real_in_tdata;
    reg  [0:0]                 real_in_tkeep;
    reg  [0:0]                 real_in_tlast;
    reg  [0:0]                 real_in_tstrb;
    reg                        real_in_tvalid;
    wire                       real_in_tready;

    reg  [C_DATA_WIDTH_IN-1:0] imag_in_tdata;
    reg  [0:0]                 imag_in_tkeep;
    reg  [0:0]                 imag_in_tlast;
    reg  [0:0]                 imag_in_tstrb;
    reg                        imag_in_tvalid;
    wire                       imag_in_tready;

    // AXI-Stream Output (DUT -> TB)
    wire [C_DATA_WIDTH_OUT-1:0] output_r_tdata;
    wire [1:0]                  output_r_tkeep;
    wire [0:0]                  output_r_tlast;
    wire [1:0]                  output_r_tstrb;
    wire                        output_r_tvalid;
    reg                         output_r_tready;

    // DUT (Device Under Test) 인스턴스화
    design_1_wrapper uut (
        .ap_clk(ap_clk),
        .ap_rst_n(ap_rst_n),

        .imag_in_tdata(imag_in_tdata),
        .imag_in_tkeep(imag_in_tkeep),
        .imag_in_tlast(imag_in_tlast),
        .imag_in_tready(imag_in_tready),
        .imag_in_tstrb(imag_in_tstrb),
        .imag_in_tvalid(imag_in_tvalid),

        .output_r_tdata(output_r_tdata),
        .output_r_tkeep(output_r_tkeep),
        .output_r_tlast(output_r_tlast),
        .output_r_tready(output_r_tready),
        .output_r_tstrb(output_r_tstrb),
        .output_r_tvalid(output_r_tvalid),

        .real_in_tdata(real_in_tdata),
        .real_in_tkeep(real_in_tkeep),
        .real_in_tlast(real_in_tlast),
        .real_in_tready(real_in_tready),
        .real_in_tstrb(real_in_tstrb),
        .real_in_tvalid(real_in_tvalid)
    );
    
    // LFM 신호 파라미터
    real M_PI           = 3.14159265;
    real amplitude      = 127.0; // 8-bit signed max (2^7 - 1)
    real f_start        = 0.00001;   // 정규화된 시작 주파수 (f/fs)
    real f_stop         = 0.002;   // 정규화된 종료 주파수 (f/fs)
    real phase          = 0.0;
    real phase_increment;
    real k; // Chirp rate
    
    // LFM 신호 생성 및 전송을 위한 변수
    real real_val, imag_val;
    reg [7:0] real_s8, imag_s8; // 8-bit 2's complement
    integer i;

    // 1. Clock 생성
    initial begin
        ap_clk = 0;
    end
    always #(CLK_PERIOD / 2) ap_clk = ~ap_clk;

    // 2. Reset 생성
    initial begin
        $display("Starting simulation...");
        ap_rst_n = 0; // Active-low reset 적용
        # (CLK_PERIOD * 10);
        ap_rst_n = 1; // Reset 해제
        # (CLK_PERIOD * 2);
    end

    // 3. 입력 스티뮬러스 (LFM 신호 생성 및 AXI-Stream 전송)
    initial begin
        // AXI-Stream 신호 초기화
        real_in_tdata  <= 0;
        real_in_tkeep  <= 0;
        real_in_tlast  <= 0;
        real_in_tstrb  <= 0;
        real_in_tvalid <= 0;
        imag_in_tdata  <= 0;
        imag_in_tkeep  <= 0;
        imag_in_tlast  <= 0;
        imag_in_tstrb  <= 0;
        imag_in_tvalid <= 0;

        // 리셋 해제 대기
        @(posedge ap_clk);
        while (ap_rst_n == 0) begin
            @(posedge ap_clk);
        end
        @(posedge ap_clk);

        $display("Reset de-asserted. Starting LFM signal generation...");

        // Chirp rate 계산 (주파수가 샘플 당 선형적으로 증가)
        // k = (f_stop - f_start) / NUM_SAMPLES;
        // phase[n] = 2*pi * (f_start*n + (k/2)*n^2)
        // phase[n] = 2*pi * (f_start*n + ((f_stop-f_start)/(2*N))*n^2)

        // 샘플 루프
        for (i = 0; i < NUM_SAMPLES; i = i + 1) begin
            
            // 1. LFM 샘플 계산 (Quadratic phase)
            phase = 2.0 * M_PI * (f_start * i + ((f_stop - f_start) / (2.0 * NUM_SAMPLES)) * i * i);

            real_val = 0.5 * amplitude * $cos(phase);
            imag_val = 0.5 * amplitude * $sin(phase);

            // 2. 8-bit Signed Int로 변환
            // $rtoi는 real 값을 2's complement integer로 변환합니다.
            real_s8 = $rtoi(real_val);
            imag_s8 = $rtoi(imag_val);

            // 4. AXI-Stream 데이터 및 제어 신호 설정
            real_in_tdata <= real_s8;
            imag_in_tdata <= imag_s8;
            
            real_in_tvalid <= 1;
            imag_in_tvalid <= 1;

            // 8-bit = 1 bytes. tkeep/tstrb는 1'b1 (0x1)
            real_in_tkeep <= 1'b1;
            imag_in_tkeep <= 1'b1;
            real_in_tstrb <= 1'b1;
            imag_in_tstrb <= 1'b1;

            // 마지막 샘플인 경우 tlast 설정
            real_in_tlast <= (i == NUM_SAMPLES - 1) ? 1'b1 : 1'b0;
            imag_in_tlast <= (i == NUM_SAMPLES - 1) ? 1'b1 : 1'b0;

            // 5. Handshake 대기 (DUT가 준비될 때까지)
            // 두 스트림이 모두 준비될 때까지 기다립니다.
            while (!(real_in_tready && imag_in_tready)) begin
                @(posedge ap_clk);
            end

            // 핸드셰이크가 발생했으므로 다음 클럭 엣지로 이동
            @(posedge ap_clk);
        end

        // 6. 모든 샘플 전송 후
        real_in_tvalid <= 0;
        imag_in_tvalid <= 0;
        real_in_tlast  <= 0;
        imag_in_tlast  <= 0;

        $display("All %0d input samples sent.", NUM_SAMPLES);

        // 파이프라인이 비워질 시간을 충분히 줍니다.
        # (CLK_PERIOD * 1000);
        $display("Simulation finished.");
        $finish;
    end

    // 4. 출력 핸들러 (AXI-Stream 수신)
    initial begin
        // 리셋 중에는 tready를 0으로 설정
        output_r_tready = 0;
        @(posedge ap_rst_n); // 리셋 해제 대기
        output_r_tready = 1; // 리셋 해제 후 항상 수신 준비 완료
    end

    always @(posedge ap_clk) begin
        if (ap_rst_n == 1) begin
            // tvalid와 tready가 모두 1일 때 (데이터 수신)
            if (output_r_tvalid && output_r_tready) begin
                $display("TIME=%t: Received output_r: TDATA=0x%h, TKEEP=0x%h, TLAST=%b",
                         $time, output_r_tdata, output_r_tkeep, output_r_tlast);
            end
        end
    end

endmodule
