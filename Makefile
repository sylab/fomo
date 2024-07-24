# NOTE: ROOT_DIR is *project* root directory
# Exported variables that will be passed onto other Makefiles
export ROOT_DIR=$(PWD)
export BUILD_DIR=$(ROOT_DIR)/build
export KBUILD_DIR=$(ROOT_DIR)/kbuild
export SRC_DIR=$(ROOT_DIR)/src
export SCRIPTS_DIR=$(ROOT_DIR)/scripts
export LINUX_DIR=$(ROOT_DIR)/linux

# NOTE: This is a directory that will hold generated header files
export POLICY_REGISTRY_DIR=$(SRC_DIR)/policy_registry

# NOTE: This determines whether to compile the dm-cache kernel module or not.
export COMPILE_DMCACHE=0

export CFLAGS=-Wall -Wno-unused -std=gnu89
export CPPFLAGS=-Wall -Wno-unused -std=gnu++11

SUBDIRS= scripts src
EXTRA_DIRS=\
	$(BUILD_DIR) \
	$(KBUILD_DIR) \
	$(POLICY_REGISTRY_DIR)

.PHONY: all prep subdirs $(SUBDIRS)

all: prep subdirs

prep: \
	$(EXTRA_DIRS) \
	$(LINUX_DIR)

$(EXTRA_DIRS):
	$(info create folders in $(ROOT_DIR))
	@mkdir -p $@

# Fail if no linux directory is found
$(LINUX_DIR):
ifdef $(COMPILE_DMCACHE)
	@test -d $(LINUX_DIR) || \
		echo "Please create Linux source directory 'linux'" \
	             "in project's root directory" && exit 2
else
	$(info skipping check for local linux directory)
endif

subdirs: $(SUBDIRS)

$(SUBDIRS):
	@$(MAKE) -C $@

.PHONY: clean
clean:
	$(info cleaning $(BUILD_DIR))
	@rm -f $(BUILD_DIR)/* 
	$(info cleaning $(KBUILD_DIR))
	@rm -f $(KBUILD_DIR)/* 
