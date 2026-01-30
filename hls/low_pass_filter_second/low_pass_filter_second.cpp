#include "ap_fixed.h"
#include "ap_int.h"
#include "hls_stream.h"
#include "ap_axi_sdata.h"

#define NUM_TAPS 44
#define DECIMATION_FACTOR 2

typedef ap_fixed<16, 1> coef_t;
typedef ap_fixed<16, 1> data_t;
typedef ap_fixed<32, 10> acc_t;
typedef hls::axis<data_t, 0, 0, 0> axis_t;
typedef ap_uint<2> count_t;

void low_pass_filter_second(hls::stream<axis_t>& input, hls::stream<axis_t>& output) {
#pragma HLS INTERFACE mode = axis port = input
#pragma HLS INTERFACE mode = axis port = output
#pragma HLS INTERFACE mode = ap_ctrl_none port = return 

    static const coef_t coefficient[DECIMATION_FACTOR][22] = {
        { 0.03948975, -0.01016235,  0.00271606,  0.00653076, -0.01870728,  0.02432251, -0.02178955,  0.00369263,  0.02966309, -0.08453369,  0.20266724,  0.37078857,  0.00427246, -0.04403687,  0.04653931, -0.03201294,  0.01354980,  0.00415039, -0.01318359,  0.01623535, -0.01074219,  0.00463867},
        { 0.00463867, -0.01074219,  0.01623535, -0.01318359,  0.00415039,  0.01354980, -0.03201294,  0.04653931, -0.04403687,  0.00427246,  0.37078857,  0.20266724, -0.08453369,  0.02966309,  0.00369263, -0.02178955,  0.02432251, -0.01870728,  0.00653076,  0.00271606, -0.01016235,  0.03948975}
    };
    static data_t shift_register[DECIMATION_FACTOR][22];
    #pragma HLS ARRAY_PARTITION variable = coefficient type = complete dim = 0
    #pragma HLS ARRAY_PARTITION variable = shift_register type = complete dim = 0

    static count_t count = 0;
    static acc_t sum = 0;
    axis_t temp;

    input.read(temp);
    shift_polyphase_filter_register_loop:
    for (int i = 21; i > 0; i--) {
    #pragma HLS UNROLL
        shift_register[DECIMATION_FACTOR - 1 - count][i] = shift_register[DECIMATION_FACTOR - 1 - count][i - 1];
    }
    shift_register[DECIMATION_FACTOR - 1 - count][0] = temp.data;
    acc_t sum_each = 0;
    polyphase_filter_convolution_loop:
    for (int i = 0; i < 22; i++) {
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