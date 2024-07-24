#include "marc_wrapper.h"
#include "alg_wrapper.h"
#include "common.h"
#include "marc_policy.h"
#include "marc_policy_struct.h"

struct marc_wrapper {
  struct alg_wrapper alg_w;
  struct marc_policy *marc;

  bool state;
};

struct marc_wrapper *to_marc_wrapper(struct alg_wrapper *alg_w) {
  return container_of(alg_w, struct marc_wrapper, alg_w);
}

int marc_wrapper_init(struct alg_wrapper *alg_w, char *fields_str) {
  struct marc_wrapper *marc_w = to_marc_wrapper(alg_w);
  int r = 0;

  if (fields_str != NULL) {
    struct alg_field marc_fields[] = {{"state", &marc_w->state}, {NULL, NULL}};

    r = alg_wrapper_read(fields_str, marc_fields);
    if (r) {
      return r;
    }
  }

  return r;
}

void marc_wrapper_print(struct alg_wrapper *alg_w) {
  struct marc_wrapper *marc_w = to_marc_wrapper(alg_w);
  struct marc_policy *marc = marc_w->marc;

  if (marc_w->state) {
    LOG_PRINT("%d", marc->state);
  }
}

struct alg_wrapper *marc_wrapper_create(cblock_t cache_size,
                                        cblock_t meta_size) {
  struct marc_wrapper *marc_w = mem_alloc(sizeof(*marc_w));
  struct cache_nucleus *cn;

  if (marc_w == NULL) {
    goto marc_wrapper_create_fail;
  }

  cn = marc_create(cache_size, meta_size);

  if (cn == NULL) {
    goto marc_policy_create_fail;
  }

  marc_w->alg_w.nucleus = cn;
  marc_w->marc = to_marc_policy(cn);

  alg_wrapper_init_functions(&marc_w->alg_w, marc_wrapper_init,
                             marc_wrapper_print);

  marc_w->state = false;

  return &marc_w->alg_w;

marc_policy_create_fail:
  mem_free(marc_w);
marc_wrapper_create_fail:
  return NULL;
}
