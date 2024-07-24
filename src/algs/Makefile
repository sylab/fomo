# The algs Makefile:
# When adding a new algorithm:
#  1) Add dependencies for new algorithm to ALG_DEPS
#  2) Add new algorithm to algs-objs
#    a) If more complex, add extra rules for KBuild as well
#  3) Add new algorithm to BUILD_ALGS_O
#  4) Add new algorithm compilation for
#     $(BUILD_DIR)/new_algorithm.o
#     with proper dependencies

LRU_DEPS=$(ALGS_DIR)/lru/lru_policy.c \
	 $(ALGS_DIR)/lru/lru_policy.h

ARC_DEPS=$(ALGS_DIR)/arc/arc_policy.c \
	 $(ALGS_DIR)/arc/arc_policy.h

LARC_DEPS=$(ALGS_DIR)/larc/larc_policy.c \
	  $(ALGS_DIR)/larc/larc_policy.h

LFU_DEPS=$(ALGS_DIR)/lfu/lfu_policy.c \
	 $(ALGS_DIR)/lfu/lfu_policy.h

LIRS_DEPS=$(ALGS_DIR)/lirs/lirs_policy.c \
	  $(ALGS_DIR)/lirs/lirs_policy.h

MARC_DEPS=$(ALGS_DIR)/marc/marc_policy.c \
          $(ALGS_DIR)/marc/marc_policy.h

ALG_DEPS=$(LRU_DEPS)  \
	 $(ARC_DEPS)  \
	 $(LARC_DEPS) \
	 $(LFU_DEPS)  \
	 $(LIRS_DEPS) \
	 $(MARC_DEPS)

.PHONY: algs

algs: $(KBUILD_DIR)/algs.o \
      $(BUILD_DIR)/libalgs.a

# KERNEL PART (kbuild/algs.o)
# Compile .o and .o.cmd file for algs.o
# Since it's created here by default, move it
# to kbuild/
# Cleanup afterwards for the userspace part

EXTRA_CFLAGS=-I $(INCLUDE_DIR) \
             -I $(SRC_DIR) \
             -I $(LINUX_DIR)/drivers/md \
	     $(CFLAGS)

obj-m += algs.o
algs-objs := lru/lru_policy.o \
             arc/arc_policy.o \
             larc/larc_policy.o \
	     lfu/lfu_policy.o \
	     lirs/lirs_policy.o \
	     marc/marc_policy.o

$(KBUILD_DIR)/algs.o: \
	$(ALG_DEPS)
ifdef $(COMPILE_DMCACHE)
	$(info   CC $(notdir $@))
	make -C /lib/modules/$(shell uname -r)/build M=$(ALGS_DIR) modules
	@mv $(ALGS_DIR)/algs.o $(ALGS_DIR)/.algs.o.cmd $(KBUILD_DIR)
	@make -C /lib/modules/$(shell uname -r)/build M=$(ALGS_DIR) clean
else
	$(info   skipping $(notdir $@) compilation)
endif


# USERSPACE PART (lib/libalgs.a)
# Compile .o files for static library libalgs.a

ALGS_CFLAGS=-I $(INCLUDE_DIR) -I $(ALGS_DIR) $(CFLAGS)

BUILD_ALGS_O=\
	$(BUILD_DIR)/lru_policy.o \
	$(BUILD_DIR)/arc_policy.o \
	$(BUILD_DIR)/larc_policy.o \
	$(BUILD_DIR)/lfu_policy.o \
	$(BUILD_DIR)/lirs_policy.o \
	$(BUILD_DIR)/marc_policy.o

$(BUILD_DIR)/libalgs.a: \
	$(BUILD_ALGS_O)
	$(info AR $(notdir $@))
	@ar rcs $(BUILD_DIR)/libalgs.a $(BUILD_ALGS_O)

$(BUILD_DIR)/lru_policy.o: \
	$(LRU_DEPS)
	$(info   CC $(notdir $@))
	@gcc -o $@ -c $(ALGS_CFLAGS) $(ALGS_DIR)/lru/lru_policy.c

$(BUILD_DIR)/arc_policy.o: \
	$(ARC_DEPS)
	$(info   CC $(notdir $@))
	@gcc -o $@ -c $(ALGS_CFLAGS) $(ALGS_DIR)/arc/arc_policy.c

$(BUILD_DIR)/larc_policy.o: \
	$(LARC_DEPS)
	$(info   CC $(notdir $@))
	@gcc -o $@ -c $(ALGS_CFLAGS) $(ALGS_DIR)/larc/larc_policy.c

$(BUILD_DIR)/lfu_policy.o: \
	$(LFU_DEPS)
	$(info   CC $(notdir $@))
	@gcc -o $@ -c $(ALGS_CFLAGS) $(ALGS_DIR)/lfu/lfu_policy.c

$(BUILD_DIR)/lirs_policy.o: \
	$(LIRS_DEPS)
	$(info   CC $(notdir $@))
	@gcc -o $@ -c $(ALGS_CFLAGS) $(ALGS_DIR)/lirs/lirs_policy.c

$(BUILD_DIR)/marc_policy.o: \
	$(MARC_DEPS)
	$(info   CC $(notdir $@))
	@gcc -o $@ -c $(ALGS_CFLAGS) $(ALGS_DIR)/marc/marc_policy.c
