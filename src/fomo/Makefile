FOMO_CFLAGS=-I $(INCLUDE_DIR) -I $(FOMO_DIR) $(CFLAGS)

#ALGS=$(filter-out Makefile, $(shell ls $(ALGS_DIR)))
#BUILD_ALGS_O_PATTERN=$(BUILD_DIR)/__xyz___policy.o
#BUILD_ALGS_O=\
#	$(foreach a, $(ALGS), $(subst __xyz__,$(a),$(BUILD_ALGS_O_PATTERN)))

FOMO_REQS_C= \
	$(FOMO_DIR)/fomo_policy.c 
FOMO_REQS_H= \
	$(FOMO_DIR)/fomo_policy.h 
BUILD_FOMO_O=$(BUILD_DIR)/fomo_policy.o 

EXTRA_CFLAGS=-I $(INCLUDE_DIR) \
             -I $(SRC_DIR) \
             -I $(LINUX_DIR)/drivers/md $(CFLAGS)

obj-m += fomo.o
fomo-objs := fomo_policy.o

.PHONY: fomo

fomo: $(KBUILD_DIR)/fomo.o \
	$(BUILD_DIR)/libfomo.a

# Do I make libmalgs.a rather than attaching to libalgs.a?
algs: $(BUILD_DIR)/libmalgs.a

$(BUILD_DIR)/libfomo.a: \
	$(FOMO_REQS_C) \
        $(FOMO_REQS_H) \
	$(BUILD_FOMO_O)
	$(info AR $(notdir $@))
	@ar rcs $(BUILD_DIR)/libfomo.a $(BUILD_FOMO_O)

$(BUILD_DIR)/fomo_policy.o: \
	fomo_policy.c \
	fomo_policy.h
	$(info CC $(notdir $@))
	@gcc -o $@ -c $(FOMO_CFLAGS) $(FOMO_DIR)/fomo_policy.c

$(KBUILD_DIR)/fomo.o: \
	$(FOMO_REQS_C) \
        $(FOMO_REQS_H)
ifdef $(COMPILE_DMCACHE)
	$(info CC $(notdir $@))
	make -C /lib/modules/$(shell uname -r)/build M=$(FOMO_DIR) modules
	mv $(FOMO_DIR)/fomo.o $(KBUILD_DIR)/fomo.o
	mv $(FOMO_DIR)/.fomo.o.cmd $(KBUILD_DIR)/.fomo.o.cmd
	make -C /lib/modules/$(shell uname -r)/build M=$(FOMO_DIR) clean
else
	$(info skipping $(notdir $@))
endif
