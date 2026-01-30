#include "ap_fixed.h"
#include "ap_int.h"
#include "hls_stream.h"
#include "ap_axi_sdata.h"

#define NUM_TAPS 41
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
#pragma HLS PIPELINE II = 1

    static const coef_t coefficient[DECIMATION_FACTOR][11] = {
        { -0.0000077433, -0.0000186248, -0.0000088219, 0.0000159231, -0.0000082580, 0.5000390250, -0.0000082580, 0.0000159231, -0.0000088219, -0.0000186248, -0.0000077433 },
        { -0.0518999719, -0.0182663193, -0.0266773613, -0.0439788632, -0.1054358580, 0.3181309800, 0.0625707326, 0.0334925482, 0.0218624622, 0.0154887756, 0.0000000000 },
        { -0.0000004130, 0.0000117251, -0.0000163227, 0.0000055939, 0.0000074290, 0.0000074290, 0.0000055939, -0.0000163227, 0.0000117251, -0.0000004130, 0.0000000000 },
        { 0.0154887756, 0.0218624622, 0.0334925482, 0.0625707326, 0.3181309800, -0.1054358580, -0.0439788632, -0.0266773613, -0.0182663193, -0.0518999719, 0.0000000000 }
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