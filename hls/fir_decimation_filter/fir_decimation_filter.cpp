#include "ap_fixed.h"
#include "ap_int.h"
#include "hls_stream.h"
#include "ap_axi_sdata.h"

#define NUM_TAPS 25
#define DECIMATION_FACTOR 5

typedef ap_fixed<16, 1> coef_t;
typedef ap_fixed<8, 1> data_t;
typedef ap_fixed<32, 10> acc_t;
typedef hls::axis<data_t, 0, 0, 0> axis_t;
typedef ap_uint<3> count_t;

void fir_decimation_filter(hls::stream<axis_t>& input, hls::stream<axis_t>& output) {
#pragma HLS INTERFACE mode = axis port = input
#pragma HLS INTERFACE mode = axis port = output
#pragma HLS INTERFACE mode = ap_ctrl_none port = return 
#pragma HLS PIPELINE II = 1

    static const coef_t coefficient[DECIMATION_FACTOR][5] = {
        {0.0193289951, 0.0263706029, 0.0934108918, -0.0620798848, -0.0234956754 },
        {0.0996113346, 0.0498237899, 0.3025603220, -0.0751905724, -0.0303894120 },
        {-0.0138526849, 0.0000086279, 0.3999505120, 0.0000086279, -0.0138526849 },
        {-0.0303894120, -0.0751905724, 0.3025603220, 0.0498237899, 0.0996113346 },
        {-0.0234956754, -0.0620798848, 0.0934108918, 0.0263706029, 0.0193289951 }
    };
    static data_t shift_register[DECIMATION_FACTOR][5];
    #pragma HLS ARRAY_PARTITION variable = coefficient type = complete dim = 0
    #pragma HLS ARRAY_PARTITION variable = shift_register type = complete dim = 0

    static count_t count = 0;
    static acc_t sum = 0;
    axis_t temp;

    input.read(temp);
    shift_polyphase_filter_register_loop:
    for (int i = 4; i > 0; i--) {
    #pragma HLS UNROLL
        shift_register[DECIMATION_FACTOR - 1 - count][i] = shift_register[DECIMATION_FACTOR - 1 - count][i - 1];
    }
    shift_register[DECIMATION_FACTOR - 1 - count][0] = temp.data;
    acc_t sum_each = 0;
    polyphase_filter_convolution_loop:
    for (int i = 0; i < 5; i++) {
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