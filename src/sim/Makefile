SIM_CFLAGS=-g -I $(INCLUDE_DIR) -I $(SRC_DIR) -I $(SIM_DIR) $(CFLAGS)

.PHONY: sim

sim: $(ROOT_DIR)/cache-sim

# TODO header files?
$(ROOT_DIR)/cache-sim: main.c \
                       $(BUILD_DIR)/libalgs.a \
		       $(BUILD_DIR)/libfomo.a \
                       $(BUILD_DIR)/libmstar.a \
		       $(BUILD_DIR)/libwrap.a
	$(info CC $(notdir $@))
	@gcc -o $(ROOT_DIR)/cache-sim \
                $(SIM_CFLAGS) main.c \
                $(BUILD_DIR)/libmstar.a \
		$(BUILD_DIR)/libfomo.a \
                $(BUILD_DIR)/libalgs.a \
		$(BUILD_DIR)/libwrap.a
