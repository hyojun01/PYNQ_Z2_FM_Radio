VITIS := vitis-run --tcl

configs = low_pass_filter_first

all: $(configs)

$(configs):
	$(VITIS) $@.tcl

clean:
	rm -rf $(configs)
	rm -rf logs

.PHONY: all clean $(configs)