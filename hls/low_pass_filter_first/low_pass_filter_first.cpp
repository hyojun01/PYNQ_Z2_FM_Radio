#include "ap_fixed.h"
#include "ap_int.h"
#include "hls_stream.h"
#include "ap_axi_sdata.h"

#define NUM_TAPS 44
#define DECIMATION_FACTOR 4

typedef ap_fixed<16, 1> coef_t;
typedef ap_fixed<16, 1> data_t;
typedef ap_fixed<32, 10> acc_t;
typedef hls::axis<data_t, 0, 0, 0> axis_t;
typedef ap_uint<2> count_t;

void low_pass_filter_first(hls::stream<axis_t>& input, hls::stream<axis_t>& output) {
#pragma HLS INTERFACE mode = axis port = input
#pragma HLS INTERFACE mode = axis port = output
#pragma HLS INTERFACE mode = ap_ctrl_none port = return 

    static const coef_t coefficient[DECIMATION_FACTOR][11] = {
        {-4.57763672e-04, -3.63159180e-03,  1.02844238e-02, -6.68334961e-03, -3.14025879e-02,  2.10205078e-01,  6.85729980e-02,  1.47705078e-02, -1.86767578e-02,  6.80541992e-03,  2.74658203e-04},
        { 2.19726562e-03, -5.24902344e-03,  2.86865234e-03,  1.86767578e-02, -6.49108887e-02,  3.11248779e-01, -3.67736816e-02,  3.33251953e-02, -1.15356445e-02, -1.03759766e-03,  2.41088867e-03},
        { 2.41088867e-03, -1.03759766e-03, -1.15356445e-02,  3.33251953e-02, -3.67736816e-02,  3.11248779e-01, -6.49108887e-02,  1.86767578e-02,  2.86865234e-03, -5.24902344e-03,  2.19726562e-03},
        { 2.74658203e-04,  6.80541992e-03, -1.86767578e-02,  1.47705078e-02,  6.85729980e-02,  2.10205078e-01, -3.14025879e-02, -6.68334961e-03,  1.02844238e-02, -3.63159180e-03, -4.57763672e-04}
    };
    static data_t shift_register[DECIMATION_FACTOR][11];
    #pragma HLS ARRAY_PARTITION variable = coefficient type = complete dim = 0
    #pragma HLS ARRAY_PARTITION variable = shift_register type = complete dim = 0

    static count_t count = 0;
    static acc_t sum = 0;
    axis_t temp;

    input.read(temp);
    shift_polyphase_filter_register_loop:
    for (int i = 10; i > 0; i--) {
    #pragma HLS UNROLL
        shift_register[DECIMATION_FACTOR - 1 - count][i] = shift_register[DECIMATION_FACTOR - 1 - count][i - 1];
    }
    shift_register[DECIMATION_FACTOR - 1 - count][0] = temp.data;
    acc_t sum_each = 0;
    polyphase_filter_convolution_loop:
    for (int i = 0; i < 11; i++) {
    #pragma HLS UNROLL
        sum_each += shift_register[DECIMATION_FACTOR - 1 - count][i] * coefficient[DECIMATION_FACTOR - 1 - count][i];
    }
    sum += sum_each;
    if (count == DECIMATION_FACTOR - 1) {
        temp.data = (data_t)sum;
        count = 0;
        sum = 0;
        output.write(temp);
    }
    else {
        count++;
    }
}