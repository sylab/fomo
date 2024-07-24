#!/bin/bash

# generate a header file for alg_wrapper_registry
# prefix with headers and unchanged functions
# for each policy(algorithm)
#     fill in entry in policy finder, which returns the policy's create function
#     if it's the policy being looked for
# have a policy create function that finds and creates the policy

algs=$(ls ../src/algs | grep -v "Makefile")

pre_gen() {
echo "\
#ifndef ALG_WRAPPER_REGISTRY_H
#define ALG_WRAPPER_REGISTRY_H

#include \"cache_nucleus.h\"
#include \"alg_wrapper.h\"

typedef struct alg_wrapper *(*alg_wrapper_create_f)(cblock_t cache_size,
                                                    cblock_t meta_size);

#define POLICY_NAME_MAX_LENGTH 60

static bool __wrapper_name_match(const char *name_a, const char *name_b) {
  return strncasecmp(name_a, name_b, POLICY_NAME_MAX_LENGTH) == 0;
}
"
}

algs_headers() {
for alg in ${1}; do
echo "#include \"algs/${alg}/${alg}_wrapper.h\""
done
}

mstar_gen() {
echo "\
#include \"mstar/mstar_wrapper.h\"
"
for alg in ${1}; do
echo "\
#include \"algs/${alg}/${alg}_policy.h\"
static struct alg_wrapper *mstar_${alg}_wrapper_create(cblock_t cache_size, cblock_t meta_size) {
  return mstar_wrapper_create(cache_size, meta_size, ${alg}_create, false);
}

static struct alg_wrapper *mstar_${alg}_lw_wrapper_create(cblock_t cache_size, cblock_t meta_size) {
  return mstar_wrapper_create(cache_size, meta_size, ${alg}_create, true);
}
"
done
}

fomo_gen() {
echo "\
#include \"fomo/fomo_wrapper.h\"
"
for alg in ${1}; do
echo "\
#include \"algs/${alg}/${alg}_policy.h\"
static struct alg_wrapper *fomo_${alg}_wrapper_create(cblock_t cache_size, cblock_t meta_size) {
  return fomo_wrapper_create(cache_size, meta_size, ${alg}_create);
}
"
done
}

find_begin() {
echo "\
static alg_wrapper_create_f find_wrapper(const char *name) {
"
}

algs_create_f() {
for alg in ${1}; do
echo "\
  if (__wrapper_name_match(name, \"${alg}\")) {
    return ${alg}_wrapper_create;
  }"
done
}

mstar_create_f() {
for alg in ${1}; do
echo "\
  if (__wrapper_name_match(name, \"mstar_${alg}\")) {
    return mstar_${alg}_wrapper_create;
  }
  if (__wrapper_name_match(name, \"mstar_${alg}_lw\")) {
    return mstar_${alg}_lw_wrapper_create;
  }"
done
}

fomo_create_f() {
for alg in ${1}; do
echo "\
  if (__wrapper_name_match(name, \"fomo_${alg}\")) {
    return fomo_${alg}_wrapper_create;
  }"
done
}

find_end() {
echo "\
  return NULL;
}
"
}

post_gen() {
echo "\
static struct alg_wrapper *create_wrapper(const char *name,
                                         cblock_t cache_size,
					 cblock_t meta_size) {
  alg_wrapper_create_f create_f = find_wrapper(name);
  if (create_f) {
    return create_f(cache_size, meta_size);
  }
  return NULL;
}

#endif\
"
}

pre_gen
algs_headers "$algs"
mstar_gen "$algs"
fomo_gen "$algs"
find_begin
algs_create_f "$algs"
mstar_create_f "$algs"
fomo_create_f "$algs"
find_end
post_gen
