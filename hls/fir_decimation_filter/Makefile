VITIS := vitis-run --tcl

configs = fir_decimation_filter

all: $(configs)

$(configs):
	$(VITIS) $@.tcl

clean:
	rm -rf $(configs)
	rm -rf logs

.PHONY: all clean $(configs)