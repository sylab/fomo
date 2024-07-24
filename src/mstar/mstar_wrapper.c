#include "mstar_wrapper.h"
#include "alg_wrapper.h"
#include "common.h"
#include "mstar_policy.h"
#include "mstar_policy_struct.h"

struct mstar_wrapper {
  struct alg_wrapper alg_w;

  struct mstar_policy *mstar;

  bool state;
};

struct mstar_wrapper *to_mstar_wrapper(struct alg_wrapper *alg_w) {
  return container_of(alg_w, struct mstar_wrapper, alg_w);
}

int mstar_wrapper_init(struct alg_wrapper *alg_w, char *fields_str) {
  struct mstar_wrapper *mstar_w = to_mstar_wrapper(alg_w);
  int r = 0;

  if (fields_str != NULL) {
    struct alg_field mstar_fields[] = {{"state", &mstar_w->state},
                                       {NULL, NULL}};

    r = alg_wrapper_read(fields_str, mstar_fields);
    if (r) {
      return r;
    }
  }

  return r;
}

void mstar_wrapper_print(struct alg_wrapper *alg_w) {
  struct mstar_wrapper *mstar_w = to_mstar_wrapper(alg_w);

  if (mstar_w->state) {
    LOG_PRINT("%u", mstar_w->mstar->logic.state->id);
  }
}

struct alg_wrapper *mstar_wrapper_create(cblock_t cache_size,
                                         cblock_t meta_size,
                                         policy_create_f policy_create,
                                         bool adaptive) {
  struct mstar_wrapper *mstar_w = mem_alloc(sizeof(*mstar_w));
  struct cache_nucleus *cn;

  if (mstar_w == NULL) {
    goto mstar_wrapper_create_fail;
  }

  cn = mstar_create(cache_size, meta_size, policy_create, adaptive);

  if (cn == NULL) {
    goto mstar_policy_create_fail;
  }

  mstar_w->alg_w.nucleus = cn;
  mstar_w->mstar = to_mstar_policy(cn);

  alg_wrapper_init_functions(&mstar_w->alg_w, mstar_wrapper_init,
                             mstar_wrapper_print);

  mstar_w->state = false;

  return &mstar_w->alg_w;

mstar_policy_create_fail:
  mem_free(mstar_w);
mstar_wrapper_create_fail:
  return NULL;
}
