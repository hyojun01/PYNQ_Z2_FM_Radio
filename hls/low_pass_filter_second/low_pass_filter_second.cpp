#include "ap_fixed.h"
#include "ap_int.h"
#include "hls_stream.h"
#include "ap_axi_sdata.h"

#define NUM_TAPS 41
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

    static const coef_t coefficient[DECIMATION_FACTOR][21] = {
        { -0.0107683800, 0.0062336500, 0.0006361500, -0.0095155100, 0.0195094100, -0.0173742800, 0.0004812400, 0.0350721400, -0.0763216100, 0.1119882700, 0.3754555000, 0.1119882700, -0.0763216100, 0.0350721400, 0.0004812400, -0.0173742800, 0.0195094100, -0.0095155100, 0.0006361500, 0.0062336500, -0.0107683800 },
        { -0.0018394800, 0.0091379200, -0.0108128800, 0.0068571200, 0.0089052900, -0.0266671800, 0.0380890500, -0.0225703600, -0.0393553600, 0.2938796800, 0.2938796800, -0.0393553600, -0.0225703600, 0.0380890500, -0.0266671800, 0.0089052900, 0.0068571200, -0.0108128800, 0.0091379200, -0.0018394800, 0.0000000000 },
    };
    static data_t shift_register[DECIMATION_FACTOR][21];
    #pragma HLS ARRAY_PARTITION variable = coefficient type = complete dim = 0
    #pragma HLS ARRAY_PARTITION variable = shift_register type = complete dim = 0

    static count_t count = 0;
    static acc_t sum = 0;
    axis_t temp;

    input.read(temp);
    shift_polyphase_filter_register_loop:
    for (int i = 20; i > 0; i--) {
    #pragma HLS UNROLL
        shift_register[DECIMATION_FACTOR - 1 - count][i] = shift_register[DECIMATION_FACTOR - 1 - count][i - 1];
    }
    shift_register[DECIMATION_FACTOR - 1 - count][0] = temp.data;
    acc_t sum_each = 0;
    polyphase_filter_convolution_loop:
    for (int i = 0; i < 21; i++) {
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