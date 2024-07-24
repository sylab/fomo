export INCLUDE_DIR=$(SRC_DIR)/include
export ALGS_DIR=$(SRC_DIR)/algs
export FOMO_DIR=$(SRC_DIR)/fomo
export MSTAR_DIR=$(SRC_DIR)/mstar
export SIM_DIR=$(SRC_DIR)/sim
export DMCACHE_POLICY_DIR=$(SRC_DIR)/dmcache_policy
export POLICY_REGISTRY_DIR=$(SRC_DIR)/policy_registry

SUBDIRS= algs fomo mstar policy_registry sim workingset_size

.PHONY: all prep subdirs $(SUBDIRS)

all: prep subdirs

prep:
	$(info check/create folders in $(SRC_DIR))
	@mkdir -p $(POLICY_REGISTRY_DIR)


subdirs: $(SUBDIRS)

$(SUBDIRS):
	@$(MAKE) -C $@
