#include "lirs_wrapper.h"
#include "alg_wrapper.h"
#include "common.h"
#include "lirs_policy.h"
#include "lirs_policy_struct.h"

struct lirs_wrapper {
  struct alg_wrapper alg_w;

  struct lirs_policy *lirs;

  bool lir_count;
};

struct lirs_wrapper *to_lirs_wrapper(struct alg_wrapper *alg_w) {
  return container_of(alg_w, struct lirs_wrapper, alg_w);
}

int lirs_wrapper_init(struct alg_wrapper *alg_w, char *fields_str) {
  struct lirs_wrapper *lirs_w = to_lirs_wrapper(alg_w);
  int r = 0;

  if (fields_str != NULL) {
    struct alg_field lirs_fields[] = {{"lir_count", &lirs_w->lir_count},
                                      {NULL, NULL}};

    r = alg_wrapper_read(fields_str, lirs_fields);
    if (r) {
      return r;
    }
  }

  return r;
}

void lirs_wrapper_print(struct alg_wrapper *alg_w) {
  struct lirs_wrapper *lirs_w = to_lirs_wrapper(alg_w);

  if (lirs_w->lir_count) {
    LOG_PRINT("%u", lirs_w->lirs->lir_count);
  }
}

struct alg_wrapper *lirs_wrapper_create(cblock_t cache_size,
                                        cblock_t meta_size) {
  struct lirs_wrapper *lirs_w = mem_alloc(sizeof(*lirs_w));
  struct cache_nucleus *cn;

  if (lirs_w == NULL) {
    goto lirs_wrapper_create_fail;
  }

  cn = lirs_create(cache_size, meta_size);

  if (cn == NULL) {
    goto lirs_policy_create_fail;
  }

  lirs_w->alg_w.nucleus = cn;
  lirs_w->lirs = to_lirs_policy(cn);

  alg_wrapper_init_functions(&lirs_w->alg_w, lirs_wrapper_init,
                             lirs_wrapper_print);

  lirs_w->lir_count = false;

  return &lirs_w->alg_w;

lirs_policy_create_fail:
  mem_free(lirs_w);
lirs_wrapper_create_fail:
  return NULL;
}
