open_project -reset quadrature_demodulator
set_top quadrature_demodulator
add_file quadrature_demodulator.cpp
open_solution solution1 -reset
set_part {xc7z020clg400-1}
create_clock -period 10ns
csynth_design
export_design -flow impl -format ip_catalog