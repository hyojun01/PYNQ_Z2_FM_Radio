#include "ap_fixed.h"
#include "ap_int.h"
#include "hls_stream.h"
#include "ap_axi_sdata.h"

typedef ap_fixed<32, 10> acc_t;
typedef ap_fixed<16, 1> complex_t;
typedef hls::axis<complex_t, 0, 0, 0> axis_in_t;
typedef ap_fixed<16, 1> data_t;
typedef hls::axis<data_t, 0, 0, 0> axis_out_t;

#define NUM_TAPS 3

void quadrature_demodulator(hls::stream<axis_in_t>& real, hls::stream<axis_in_t>& imag, hls::stream<axis_out_t>& output) {
#pragma HLS INTERFACE mode = axis port = real
#pragma HLS INTERFACE mode = axis port = imag
#pragma HLS INTERFACE mode = axis port = output
#pragma HLS INTERFACE mode = ap_ctrl_none port = return 

    static complex_t shift_register_real[NUM_TAPS];
    static complex_t shitf_register_imag[NUM_TAPS];
    #pragma HLS ARRAY_PARTITION variable = shift_register_real type = complete dim = 0
    #pragma HLS ARRAY_PARTITION variable = shitf_register_imag type = complete dim = 0

    axis_in_t temp_real;
    axis_in_t temp_imag;
    axis_out_t temp_output;

    acc_t result;
    acc_t diff_real;
    acc_t diff_imag;

    // const data_t scaling_factor = 61274.6530904;
    const acc_t scaling_factor = 8;

    // Operation

    real.read(temp_real);
    imag.read(temp_imag);

    real_register_shift_loop:
    for (int i = NUM_TAPS - 1; i > 0; i--) {
    #pragma HLS UNROLL
        shift_register_real[i] = shift_register_real[i - 1];
    }
    shift_register_real[0] = temp_real.data;

    imag_register_shift_loop:
    for (int i = NUM_TAPS - 1; i > 0; i--) {
    #pragma HLS UNROLL
        shitf_register_imag[i] = shitf_register_imag[i - 1];
    }
    shitf_register_imag[0] = temp_imag.data;

    diff_real = shift_register_real[0] - shift_register_real[2];
    diff_imag = shitf_register_imag[0] - shitf_register_imag[2];

    result = temp_real.data * diff_imag - temp_imag.data * diff_real;
    result = result * scaling_factor;
    
    temp_output.data = (data_t)result;
    temp_output.keep = -1;
    temp_output.strb = -1;
    temp_output.last = temp_real.last;

    output.write(temp_output);
    
}