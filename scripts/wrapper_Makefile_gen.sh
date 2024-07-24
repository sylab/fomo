#!/bin/bash

# generate a Makefile for alg_wrapper_registry
# prefix with headers and unchanged functions
# for each policy(algorithm)
#     fill in entry in policy finder, which returns the policy's create function
#     if it's the policy being looked for
# have a policy create function that finds and creates the policy

algs=$(ls ../src/algs | grep -v "Makefile")

echo "\
WRAPPER_DIR=
WRAPPER_CFLAGS=-I \$(INCLUDE_DIR) \$(CFLAGS)

BUILD_WRAPS_O=\\"

for alg in ${algs}; do
echo \
"	\$(BUILD_DIR)/${alg}_wrapper.o \\"
done
echo "\
	\$(BUILD_DIR)/mstar_wrapper.o \\
	\$(BUILD_DIR)/fomo_wrapper.o

.PHONY: policy_registry

policy_registry: \$(BUILD_DIR)/libwrap.a

\$(BUILD_DIR)/libwrap.a: \$(BUILD_WRAPS_O)
	\$(info  AR \$(notdir \$@))
	@ar rcs \$(BUILD_DIR)/libwrap.a \$(BUILD_WRAPS_O)

\$(BUILD_DIR)/mstar_wrapper.o: \\
	\$(MSTAR_DIR)/mstar_wrapper.c
	\$(info CC \$(notdir \$@))
	@gcc -o \$@ -c \$(WRAPPER_CFLAGS) \$(MSTAR_DIR)/mstar_wrapper.c

\$(BUILD_DIR)/fomo_wrapper.o: \\
	\$(FOMO_DIR)/fomo_wrapper.c
	\$(info CC \$(notdir \$@))
	@gcc -o \$@ -c \$(WRAPPER_CFLAGS) \$(FOMO_DIR)/fomo_wrapper.c
"

for alg in ${algs}; do
echo \
"\$(BUILD_DIR)/${alg}_wrapper.o: \\
	\$(ALGS_DIR)/${alg}/${alg}_wrapper.c
	\$(info CC \$(notdir \$@))
	@gcc -o \$@ -c \$(WRAPPER_CFLAGS) \$(ALGS_DIR)/${alg}/${alg}_wrapper.c
"
done
