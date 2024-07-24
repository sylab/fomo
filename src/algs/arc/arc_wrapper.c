#include "arc_wrapper.h"
#include "alg_wrapper.h"
#include "arc_policy.h"
#include "arc_policy_struct.h"
#include "common.h"

struct arc_wrapper {
  struct alg_wrapper alg_w;

  struct arc_policy *arc;

  bool p;
};

struct arc_wrapper *to_arc_wrapper(struct alg_wrapper *alg_w) {
  return container_of(alg_w, struct arc_wrapper, alg_w);
}

int arc_wrapper_init(struct alg_wrapper *alg_w, char *fields_str) {
  struct arc_wrapper *arc_w = to_arc_wrapper(alg_w);
  int r = 0;

  if (fields_str != NULL) {
    struct alg_field arc_fields[] = {{"p", &arc_w->p}, {NULL, NULL}};

    r = alg_wrapper_read(fields_str, arc_fields);
    if (r) {
      return r;
    }
  }

  return r;
}

void arc_wrapper_print(struct alg_wrapper *alg_w) {
  struct arc_wrapper *arc_w = to_arc_wrapper(alg_w);

  if (arc_w->p) {
    LOG_PRINT("%u", arc_w->arc->p);
  }
}

struct alg_wrapper *arc_wrapper_create(cblock_t cache_size,
                                       cblock_t meta_size) {
  struct arc_wrapper *arc_w = mem_alloc(sizeof(*arc_w));
  struct cache_nucleus *cn;

  if (arc_w == NULL) {
    goto arc_wrapper_create_fail;
  }

  cn = arc_create(cache_size, meta_size);

  if (cn == NULL) {
    goto arc_policy_create_fail;
  }

  arc_w->alg_w.nucleus = cn;
  arc_w->arc = to_arc_policy(cn);

  alg_wrapper_init_functions(&arc_w->alg_w, arc_wrapper_init,
                             arc_wrapper_print);

  arc_w->p = false;

  return &arc_w->alg_w;

arc_policy_create_fail:
  mem_free(arc_w);
arc_wrapper_create_fail:
  return NULL;
}
