#include "lfu_wrapper.h"
#include "alg_wrapper.h"
#include "common.h"
#include "lfu_policy.h"
#include "lfu_policy_struct.h"

struct lfu_wrapper {
  struct alg_wrapper alg_w;

  struct lfu_policy *lfu;
};

struct lfu_wrapper *to_lfu_wrapper(struct alg_wrapper *alg_w) {
  return container_of(alg_w, struct lfu_wrapper, alg_w);
}

int lfu_wrapper_init(struct alg_wrapper *alg_w, char *fields_str) {
  struct lfu_wrapper *lfu_w = to_lfu_wrapper(alg_w);
  int r = 0;

  if (fields_str != NULL) {
    struct alg_field lfu_fields[] = {{NULL, NULL}};

    r = alg_wrapper_read(fields_str, lfu_fields);
    if (r) {
      return r;
    }
  }

  return r;
}

void lfu_wrapper_print(struct alg_wrapper *alg_w) {
  struct lfu_wrapper *lfu_w = to_lfu_wrapper(alg_w);
}

struct alg_wrapper *lfu_wrapper_create(cblock_t cache_size,
                                       cblock_t meta_size) {
  struct lfu_wrapper *lfu_w = mem_alloc(sizeof(*lfu_w));
  struct cache_nucleus *cn;

  if (lfu_w == NULL) {
    goto lfu_wrapper_create_fail;
  }

  cn = lfu_create(cache_size, meta_size);

  if (cn == NULL) {
    goto lfu_policy_create_fail;
  }

  lfu_w->alg_w.nucleus = cn;
  lfu_w->lfu = to_lfu_policy(cn);

  alg_wrapper_init_functions(&lfu_w->alg_w, lfu_wrapper_init,
                             lfu_wrapper_print);

  return &lfu_w->alg_w;

lfu_policy_create_fail:
  mem_free(lfu_w);
lfu_wrapper_create_fail:
  return NULL;
}
