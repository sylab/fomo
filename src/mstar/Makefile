MSTAR_CFLAGS=-I $(INCLUDE_DIR) -I $(MSTAR_DIR) $(CFLAGS)

#ALGS=$(filter-out Makefile, $(shell ls $(ALGS_DIR)))
#BUILD_ALGS_O_PATTERN=$(BUILD_DIR)/__xyz___policy.o
#BUILD_ALGS_O=\
#	$(foreach a, $(ALGS), $(subst __xyz__,$(a),$(BUILD_ALGS_O_PATTERN)))

MSTAR_REQS_C= \
	$(MSTAR_DIR)/mstar_policy.c \
	$(MSTAR_DIR)/mstar_logic.c \
	$(MSTAR_DIR)/mstar_transitions.c \
	$(MSTAR_DIR)/mstar_phase.c
MSTAR_REQS_H= \
	$(MSTAR_DIR)/mstar_policy.h \
	$(MSTAR_DIR)/mstar_logic.h \
	$(MSTAR_DIR)/mstar_transitions.h \
	$(MSTAR_DIR)/mstar_phase.h
BUILD_MSTAR_O=$(BUILD_DIR)/mstar_policy.o \
              $(BUILD_DIR)/mstar_logic.o \
              $(BUILD_DIR)/mstar_transitions.o \
              $(BUILD_DIR)/mstar_phase.o

EXTRA_CFLAGS=-I $(INCLUDE_DIR) \
             -I $(SRC_DIR) \
             -I $(LINUX_DIR)/drivers/md $(CFLAGS)

obj-y += mstar_phase.o \
         mstar_logic.o \
         mstar_transitions.o
obj-m += mstar.o
mstar-objs := mstar_phase.o \
              mstar_logic.o \
              mstar_transitions.o \
              mstar_policy.o

.PHONY: mstar

mstar: $(KBUILD_DIR)/mstar.o \
       $(BUILD_DIR)/libmstar.a

# Do I make libmalgs.a rather than attaching to libalgs.a?
algs: $(BUILD_DIR)/libmalgs.a

$(BUILD_DIR)/libmstar.a: \
	$(MSTAR_REQS_C) \
        $(MSTAR_REQS_H) \
	$(BUILD_MSTAR_O)
	$(info AR $(notdir $@))
	@ar rcs $(BUILD_DIR)/libmstar.a $(BUILD_MSTAR_O)

$(BUILD_DIR)/mstar_policy.o: \
	mstar_policy.c \
	mstar_policy.h
	$(info CC $(notdir $@))
	@gcc -o $@ -c $(MSTAR_CFLAGS) $(MSTAR_DIR)/mstar_policy.c

$(BUILD_DIR)/mstar_logic.o: \
	mstar_logic.c \
	mstar_logic.h
	$(info CC $(notdir $@))
	@gcc -o $@ -c $(MSTAR_CFLAGS) $(MSTAR_DIR)/mstar_logic.c

$(BUILD_DIR)/mstar_transitions.o: \
	mstar_transitions.c \
	mstar_transitions.h
	$(info CC $(notdir $@))
	@gcc -o $@ -c $(MSTAR_CFLAGS) $(MSTAR_DIR)/mstar_transitions.c

$(BUILD_DIR)/mstar_phase.o: \
	mstar_phase.c \
	mstar_phase.h
	$(info CC $(notdir $@))
	@gcc -o $@ -c $(MSTAR_CFLAGS) $(MSTAR_DIR)/mstar_phase.c

$(KBUILD_DIR)/mstar.o: \
	$(MSTAR_REQS_C) \
        $(MSTAR_REQS_H)
ifdef $(COMPILE_DMCACHE)
	$(info CC $(notdir $@))
	make -C /lib/modules/$(shell uname -r)/build M=$(MSTAR_DIR) modules
	mv $(MSTAR_DIR)/mstar.o $(KBUILD_DIR)/mstar.o
	mv $(MSTAR_DIR)/.mstar.o.cmd $(KBUILD_DIR)/.mstar.o.cmd
	make -C /lib/modules/$(shell uname -r)/build M=$(MSTAR_DIR) clean
else
	$(info skipping $(notdir $@))
endif
