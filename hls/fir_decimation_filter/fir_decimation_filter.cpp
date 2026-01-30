#include "ap_fixed.h"
#include "ap_int.h"
#include "hls_stream.h"
#include "ap_axi_sdata.h"

#define NUM_TAPS 25
#define DECIMATION_FACTOR 5

typedef ap_fixed<16, 1> coef_t;
typedef ap_fixed<8, 1> input_t;
typedef ap_fixed<16, 1> output_t;
typedef ap_fixed<32, 10> acc_t;
typedef hls::axis<input_t, 0, 0, 0> in_axis_t;
typedef hls::axis<output_t, 0, 0, 0> out_axis_t;
typedef ap_uint<3> count_t;

void fir_decimation_filter(hls::stream<in_axis_t>& input, hls::stream<out_axis_t>& output) {
#pragma HLS INTERFACE mode = axis port = input
#pragma HLS INTERFACE mode = axis port = output
#pragma HLS INTERFACE mode = ap_ctrl_none port = return 

    static const coef_t coefficient[DECIMATION_FACTOR][5] = {
        {-0.00222778, 0.01422119, 0.14111328, 0.01925659, 0.019104 },
        {-0.00692749, -0.01397705, 0.26159668, -0.04702759, 0.00759888 },
        {-0.00299072, -0.04763794, 0.3119812, -0.04763794, -0.00299072 },
        {0.00759888, -0.04702759, 0.26159668, -0.01397705, -0.00692749 },
        {0.019104, 0.01925659, 0.14111328, 0.01422119, -0.00222778 }
    };
    static input_t shift_register[DECIMATION_FACTOR][5];
    #pragma HLS ARRAY_PARTITION variable = coefficient type = complete dim = 0
    #pragma HLS ARRAY_PARTITION variable = shift_register type = complete dim = 0

    static count_t count = 0;
    static acc_t sum = 0;
    in_axis_t input_temp;
    out_axis_t output_temp;

    input.read(input_temp);
    shift_polyphase_filter_register_loop:
    for (int i = 4; i > 0; i--) {
    #pragma HLS UNROLL
        shift_register[DECIMATION_FACTOR - 1 - count][i] = shift_register[DECIMATION_FACTOR - 1 - count][i - 1];
    }
    shift_register[DECIMATION_FACTOR - 1 - count][0] = input_temp.data;
    acc_t sum_each = 0;
    polyphase_filter_convolution_loop:
    for (int i = 0; i < 5; i++) {
    #pragma HLS UNROLL
        sum_each += shift_register[DECIMATION_FACTOR - 1 - count][i] * coefficient[DECIMATION_FACTOR - 1 - count][i];
    }
    sum += sum_each;
    if (count == DECIMATION_FACTOR - 1) {
        output_temp.data = (output_t)sum;
        output_temp.keep = -1;
        output_temp.strb = -1;
        output_temp.last = input_temp.last;
        count = 0;
        sum = 0;
        output.write(output_temp);
    }
    else {
        count++;
    }
}