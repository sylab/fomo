EXTRA_CFLAGS=-I $(INCLUDE_DIR) -I $(SRC_DIR) -I $(LINUX_DIR)/drivers/md $(CFLAGS)

.PHONY: dm-cache-policy

obj-m += dmcache-policy.o \
         dm-cache.o

# NOTE:
# Due to how kbuild (the program) seemingly has the inability to use
# `../` in directory paths, the kbuild and linux directories in the 
# project's root directory must be linked in this directory for access

dmcache-policy-objs := \
                       kbuild/algs.o \
                       kbuild/mstar.o \
		       kbuild/fomo.o \
                       dmcache_policy.o

dm-cache-objs := \
                 linux/drivers/md/dm-cache-target.o \
                 linux/drivers/md/dm-cache-metadata.o \
                 linux/drivers/md/dm-cache-policy.o

all: prep $(ROOT_DIR)/dmcache-policy.ko end

prep:
	ln -s $(LINUX_DIR) linux
	ln -s $(KBUILD_DIR) kbuild

end:
	rm linux kbuild

# TODO header files? hoping for an elegant solution to looking for header file changes
$(ROOT_DIR)/dmcache-policy.ko: \
		dmcache_policy.c 
	$(info CC $(notdir $@))
	make -C /lib/modules/$(shell uname -r)/build M=$(DMCACHE_POLICY_DIR) modules
	@mv $(DMCACHE_POLICY_DIR)/dmcache-policy.ko \
            $(DMCACHE_POLICY_DIR)/dm-cache.ko \
            $(ROOT_DIR)
	@make -C /lib/modules/$(shell uname -r)/build M=$(DMCACHE_POLICY_DIR) clean
