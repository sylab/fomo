#include "fomo_wrapper.h"
#include "alg_wrapper.h"
#include "common.h"
#include "fomo_policy.h"
#include "fomo_policy_struct.h"

struct fomo_wrapper {
  struct alg_wrapper alg_w;

  struct fomo_policy *fomo;

  bool state;
  bool hr_miss_history;
};

struct fomo_wrapper *to_fomo_wrapper(struct alg_wrapper *alg_w) {
  return container_of(alg_w, struct fomo_wrapper, alg_w);
}

int fomo_wrapper_init(struct alg_wrapper *alg_w, char *fields_str) {
  struct fomo_wrapper *fomo_w = to_fomo_wrapper(alg_w);
  int r = 0;

  if (fields_str != NULL) {
    struct alg_field fomo_fields[] = {
        {"state", &fomo_w->state},
        {"hr-miss-history", &fomo_w->hr_miss_history},
        {NULL, NULL}};

    r = alg_wrapper_read(fields_str, fomo_fields);
    if (r) {
      return r;
    }
  }

  return r;
}

void fomo_wrapper_print(struct alg_wrapper *alg_w) {
  struct fomo_wrapper *fomo_w = to_fomo_wrapper(alg_w);

  if (fomo_w->state) {
    LOG_PRINT("%u", fomo_w->fomo->state);
  }
  if (fomo_w->hr_miss_history) {
    LOG_PRINT("%u", fomo_w->fomo->mh_hits_total);
  }
}

struct alg_wrapper *fomo_wrapper_create(cblock_t cache_size, cblock_t meta_size,
                                        policy_create_f policy_create) {
  struct fomo_wrapper *fomo_w = mem_alloc(sizeof(*fomo_w));
  struct cache_nucleus *cn;

  if (fomo_w == NULL) {
    goto fomo_wrapper_create_fail;
  }

  cn = fomo_create(cache_size, meta_size, policy_create);

  if (cn == NULL) {
    goto fomo_policy_create_fail;
  }

  fomo_w->alg_w.nucleus = cn;
  fomo_w->fomo = to_fomo_policy(cn);

  alg_wrapper_init_functions(&fomo_w->alg_w, fomo_wrapper_init,
                             fomo_wrapper_print);

  fomo_w->state = false;
  fomo_w->hr_miss_history = false;

  return &fomo_w->alg_w;

fomo_policy_create_fail:
  mem_free(fomo_w);
fomo_wrapper_create_fail:
  return NULL;
}
