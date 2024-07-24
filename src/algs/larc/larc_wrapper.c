#include "larc_wrapper.h"
#include "alg_wrapper.h"
#include "common.h"
#include "larc_policy.h"
#include "larc_policy_struct.h"

struct larc_wrapper {
  struct alg_wrapper alg_w;

  struct larc_policy *larc;

  bool g_size;
};

struct larc_wrapper *to_larc_wrapper(struct alg_wrapper *alg_w) {
  return container_of(alg_w, struct larc_wrapper, alg_w);
}

int larc_wrapper_init(struct alg_wrapper *alg_w, char *fields_str) {
  struct larc_wrapper *larc_w = to_larc_wrapper(alg_w);
  int r = 0;

  if (fields_str != NULL) {
    struct alg_field larc_fields[] = {{"g_size", &larc_w->g_size},
                                      {NULL, NULL}};

    r = alg_wrapper_read(fields_str, larc_fields);
    if (r) {
      return r;
    }
  }

  return r;
}

void larc_wrapper_print(struct alg_wrapper *alg_w) {
  struct larc_wrapper *larc_w = to_larc_wrapper(alg_w);

  if (larc_w->g_size) {
    LOG_PRINT("%u", larc_w->larc->g_size);
  }
}

struct alg_wrapper *larc_wrapper_create(cblock_t cache_size,
                                        cblock_t meta_size) {
  struct larc_wrapper *larc_w = mem_alloc(sizeof(*larc_w));
  struct cache_nucleus *cn;

  if (larc_w == NULL) {
    goto larc_wrapper_create_fail;
  }

  cn = larc_create(cache_size, meta_size);

  if (cn == NULL) {
    goto larc_policy_create_fail;
  }

  larc_w->alg_w.nucleus = cn;
  larc_w->larc = to_larc_policy(cn);

  alg_wrapper_init_functions(&larc_w->alg_w, larc_wrapper_init,
                             larc_wrapper_print);

  larc_w->g_size = false;

  return &larc_w->alg_w;

larc_policy_create_fail:
  mem_free(larc_w);
larc_wrapper_create_fail:
  return NULL;
}
