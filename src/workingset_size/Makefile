SET_SIZE_CFLAGS=-g -I $(INCLUDE_DIR) -I $(SRC_DIR) $(CPPFLAGS)

.PHONY: sim

set-size: $(ROOT_DIR)/set-size

# TODO header files?
$(ROOT_DIR)/set-size: set_size.cc
	$(info CC $(notdir $@))
	@g++ -o $(ROOT_DIR)/set-size \
                $(SET_SIZE_CFLAGS) set_size.cc
