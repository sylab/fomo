WRAPPER_DIR=
WRAPPER_CFLAGS=-I $(INCLUDE_DIR) $(CFLAGS)

BUILD_WRAPS_O=\
	$(BUILD_DIR)/arc_wrapper.o \
	$(BUILD_DIR)/larc_wrapper.o \
	$(BUILD_DIR)/lfu_wrapper.o \
	$(BUILD_DIR)/lirs_wrapper.o \
	$(BUILD_DIR)/lru_wrapper.o \
	$(BUILD_DIR)/marc_wrapper.o \
	$(BUILD_DIR)/mstar_wrapper.o \
	$(BUILD_DIR)/fomo_wrapper.o

.PHONY: policy_registry

policy_registry: $(BUILD_DIR)/libwrap.a

$(BUILD_DIR)/libwrap.a: $(BUILD_WRAPS_O)
	$(info  AR $(notdir $@))
	@ar rcs $(BUILD_DIR)/libwrap.a $(BUILD_WRAPS_O)

$(BUILD_DIR)/mstar_wrapper.o: \
	$(MSTAR_DIR)/mstar_wrapper.c
	$(info CC $(notdir $@))
	@gcc -o $@ -c $(WRAPPER_CFLAGS) $(MSTAR_DIR)/mstar_wrapper.c

$(BUILD_DIR)/fomo_wrapper.o: \
	$(FOMO_DIR)/fomo_wrapper.c
	$(info CC $(notdir $@))
	@gcc -o $@ -c $(WRAPPER_CFLAGS) $(FOMO_DIR)/fomo_wrapper.c

$(BUILD_DIR)/arc_wrapper.o: \
	$(ALGS_DIR)/arc/arc_wrapper.c
	$(info CC $(notdir $@))
	@gcc -o $@ -c $(WRAPPER_CFLAGS) $(ALGS_DIR)/arc/arc_wrapper.c

$(BUILD_DIR)/larc_wrapper.o: \
	$(ALGS_DIR)/larc/larc_wrapper.c
	$(info CC $(notdir $@))
	@gcc -o $@ -c $(WRAPPER_CFLAGS) $(ALGS_DIR)/larc/larc_wrapper.c

$(BUILD_DIR)/lfu_wrapper.o: \
	$(ALGS_DIR)/lfu/lfu_wrapper.c
	$(info CC $(notdir $@))
	@gcc -o $@ -c $(WRAPPER_CFLAGS) $(ALGS_DIR)/lfu/lfu_wrapper.c

$(BUILD_DIR)/lirs_wrapper.o: \
	$(ALGS_DIR)/lirs/lirs_wrapper.c
	$(info CC $(notdir $@))
	@gcc -o $@ -c $(WRAPPER_CFLAGS) $(ALGS_DIR)/lirs/lirs_wrapper.c

$(BUILD_DIR)/lru_wrapper.o: \
	$(ALGS_DIR)/lru/lru_wrapper.c
	$(info CC $(notdir $@))
	@gcc -o $@ -c $(WRAPPER_CFLAGS) $(ALGS_DIR)/lru/lru_wrapper.c

$(BUILD_DIR)/marc_wrapper.o: \
	$(ALGS_DIR)/marc/marc_wrapper.c
	$(info CC $(notdir $@))
	@gcc -o $@ -c $(WRAPPER_CFLAGS) $(ALGS_DIR)/marc/marc_wrapper.c

