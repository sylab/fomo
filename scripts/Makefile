.PHONY: scripts

scripts: policy_registry_gen \
	 dmcache_policy_registry_gen \
	 wrapper_registry_gen \
	 wrapper_Makefile_gen

policy_registry_gen:
	@bash $(SCRIPTS_DIR)/policy_registry_gen.sh \
		> $(SRC_DIR)/policy_registry/policy_registry.h

dmcache_policy_registry_gen:
	@bash $(SCRIPTS_DIR)/dmcache_policy_registry_gen.sh \
		> $(SRC_DIR)/dmcache_policy/dmcache_policy_registry.h

wrapper_registry_gen:
	@bash $(SCRIPTS_DIR)/wrapper_registry_gen.sh \
		> $(SRC_DIR)/policy_registry/wrapper_registry.h

wrapper_Makefile_gen:
	@bash $(SCRIPTS_DIR)/wrapper_Makefile_gen.sh \
		> $(SRC_DIR)/policy_registry/Makefile

