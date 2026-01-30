VITIS := vitis-run --tcl

configs = low_pass_filter_second

all: $(configs)

$(configs):
	$(VITIS) $@.tcl

clean:
	rm -rf $(configs)
	rm -rf logs

.PHONY: all clean $(configs)