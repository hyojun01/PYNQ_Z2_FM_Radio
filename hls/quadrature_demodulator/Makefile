VITIS := vitis-run --tcl

configs = quadrature_demodulator

all: $(configs)

$(configs):
	$(VITIS) $@.tcl

clean:
	rm -rf $(configs)
	rm -rf logs

.PHONY: all clean $(configs)