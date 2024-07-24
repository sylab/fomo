#!/bin/bash

# generate a header file for policy_registry
# prefix with headers and unchanged functions
# for each algs
#     fill in templating alg struct
#     write templated alg function
# any finishing touches

algs=$(ls ../src/algs | grep -v "Makefile")

pre_gen() {
echo "\
#ifndef DMCACHE_POLICY_REGISTRY_H
#define DMCACHE_POLICY_REGISTRY_H

// TODO proper includes
#include \"dmcache_policy/dmcache_base.h\"
"

#static void policy_registry_push(struct registry *r,
#                                 struct policy_type *policy) {
#  registry_push(r, &policy->entry);
#}
#
#static struct policy_type *policy_registry_pop(struct registry *r) {
#  struct registry_entry *re = registry_pop(r);
#  if (re) {
#    return container_of(re, struct policy_type, entry);
#  }
#  return NULL;
#}
#
#static struct policy_type *policy_registry_find(const struct registry *r,
#                                                const char *name) {
#  struct registry_entry *re = registry_find(r, name);
#  if (re) {
#    return container_of(re, struct policy_type, entry);
#  }
#
#  return NULL;
#}
#"
}

struct_gen() {
for alg in ${1}; do
echo "\
#include \"algs/${alg}/${alg}_policy.h\"
struct dm_cache_policy *dm_cache_policy_${alg}_create(cblock_t cache_size,
                                                      sector_t origin_size,
                                                      sector_t cache_block_size) {
  struct dmcache_base *dmcb = dmcache_base_create(${alg}_create, cache_size);
  if (!dmcb)
    return NULL;
  return &dmcb->dm_cache_policy;
}

struct dm_cache_policy_type ${alg}_policy_type = {
  .name = \"${alg}\",
  .version = {1, 0, 0},
  .owner = THIS_MODULE,
  .create = dm_cache_policy_${alg}_create
};
"
done
}

mstar_gen() {
echo "\
#include \"mstar/mstar_policy.h\"
"
for alg in ${1}; do
echo "\
struct cache_nucleus *mstar_${alg}_create(cblock_t cache_size, cblock_t meta_size) {
  return mstar_create(cache_size, meta_size, ${alg}_create, false);
}

struct dm_cache_policy *dm_cache_policy_mstar_${alg}_create(cblock_t cache_size,
                                                       sector_t origin_size,
                                                       sector_t cache_block_size) {
  struct dmcache_base *dmcb = dmcache_base_create(mstar_${alg}_create, cache_size);
  if (!dmcb)
    return NULL;
  return &dmcb->dm_cache_policy;
}

struct dm_cache_policy_type mstar_${alg}_policy_type = {
  .name = \"mstar_${alg}\",
  .version = {1, 0, 0},
  .owner = THIS_MODULE,
  .create = dm_cache_policy_mstar_${alg}_create
};

struct cache_nucleus *mstar_${alg}_lw_create(cblock_t cache_size, cblock_t meta_size) {
  return mstar_create(cache_size, meta_size, ${alg}_create, true);
}

struct dm_cache_policy *dm_cache_policy_mstar_${alg}_lw_create(cblock_t cache_size,
                                                          sector_t origin_size,
                                                          sector_t cache_block_size) {
  struct dmcache_base *dmcb = dmcache_base_create(mstar_${alg}_lw_create, cache_size);
  if (!dmcb)
    return NULL;
  return &dmcb->dm_cache_policy;
}

struct dm_cache_policy_type mstar_${alg}_lw_policy_type = {
  .name = \"mstar_${alg}_lw\",
  .version = {1, 0, 0},
  .owner = THIS_MODULE,
  .create = dm_cache_policy_mstar_${alg}_lw_create
};
"
done
}

fomo_gen() {
echo "\
#include \"fomo/fomo_policy.h\"
"
for alg in ${1}; do
echo "\
struct cache_nucleus *fomo_${alg}_create(cblock_t cache_size, cblock_t meta_size) {
  return fomo_create(cache_size, meta_size, ${alg}_create);
}

struct dm_cache_policy *dm_cache_policy_fomo_${alg}_create(cblock_t cache_size,
                                                           sector_t origin_size,
                                                           sector_t cache_block_size) {
  struct dmcache_base *dmcb = dmcache_base_create(fomo_${alg}_create, cache_size);
  if (!dmcb)
    return NULL;
  return &dmcb->dm_cache_policy;
}

struct dm_cache_policy_type fomo_${alg}_policy_type = {
  .name = \"fomo_${alg}\",
  .version = {1, 0, 0},
  .owner = THIS_MODULE,
  .create = dm_cache_policy_fomo_${alg}_create
};
"
done
}


init_gen() {
algs=${1}
declare -a algs_reverse
echo "\
static int policy_registry_init(void) {
  int r;
"

for alg in ${algs}; do
echo "\
  r = dm_cache_policy_register(&${alg}_policy_type);
  LOG_INFO(\"${alg} registry %p %d\", &${alg}_policy_type, r);
  if (r)
    goto ${alg}_policy_type_register_failed;
"
algs_reverse=("${alg}" "${algs_reverse[@]}")
done

for alg in ${algs}; do
echo "\
  r = dm_cache_policy_register(&m${alg}_policy_type);
  LOG_INFO(\"m${alg} registry %p %d\", &m${alg}_policy_type, r);
  if (r)
    goto m${alg}_policy_type_register_failed;
"
algs_reverse=("m${alg}" "${algs_reverse[@]}")
done

for alg in ${algs}; do
echo "\
  r = dm_cache_policy_register(&m${alg}_lw_policy_type);
  LOG_INFO(\"m${alg}_lw registry %p %d\", &m${alg}_lw_policy_type, r);
  if (r)
    goto m${alg}_lw_policy_type_register_failed;
"
algs_reverse=("m${alg}_lw" "${algs_reverse[@]}")
done

for alg in ${algs}; do
echo "\
  r = dm_cache_policy_register(&fomo_${alg}_policy_type);
  LOG_INFO(\"fomo_${alg} registry %p %d\", &fomo_${alg}_policy_type, r);
  if (r)
    goto fomo_${alg}_policy_type_register_failed;
"
algs_reverse=("fomo_${alg}" "${algs_reverse[@]}")
done

echo "\
  return 0;

"

for alg in ${algs_reverse[@]}; do
if [ ${alg} != ${algs_reverse[0]} ]; then
echo "\
  LOG_INFO(\"${alg} unregister %p\", &${alg}_policy_type);
  dm_cache_policy_unregister(&${alg}_policy_type);\
"
fi
echo "\
${alg}_policy_type_register_failed:\
"
done

echo "\
  return r;
}\
"
}

exit_gen() {
algs=${1}
echo "\
static void policy_registry_exit(void) {"

for alg in ${algs}; do
echo "\
  LOG_INFO(\"${alg} unregister %p\", &${alg}_policy_type);
  dm_cache_policy_unregister(&${alg}_policy_type);"
done

for alg in ${algs}; do
echo "\
  LOG_INFO(\"m${alg} unregister %p\", &m${alg}_policy_type);
  dm_cache_policy_unregister(&m${alg}_policy_type);"
done
  
for alg in ${algs}; do
echo "\
  LOG_INFO(\"m${alg}_lw unregister %p\", &m${alg}_lw_policy_type);
  dm_cache_policy_unregister(&m${alg}_lw_policy_type);"
done

for alg in ${algs}; do
echo "\
  LOG_INFO(\"fomo_${alg} unregister %p\", &fomo_${alg}_policy_type);
  dm_cache_policy_unregister(&fomo_${alg}_policy_type);"
 done
  
echo "\
}
"
}

post_gen() {
echo "\
#endif\
"
}

pre_gen
struct_gen "$algs"
mstar_gen "$algs"
fomo_gen "$algs"
init_gen "$algs"
exit_gen "$algs"
post_gen
