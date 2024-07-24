#include "lru_wrapper.h"
#include "alg_wrapper.h"
#include "common.h"
#include "lru_policy.h"
#include "lru_policy_struct.h"

struct lru_wrapper {
  // alg_wrapper that applications can interact with
  struct alg_wrapper alg_w;

  // algorithm that is being wrapped
  struct lru_policy *lru;

  // flags for printing
  bool lru_q;
};

struct lru_wrapper *to_lru_wrapper(struct alg_wrapper *alg_w) {
  return container_of(alg_w, struct lru_wrapper, alg_w);
}

int lru_wrapper_init(struct alg_wrapper *alg_w, char *fields_str) {
  struct lru_wrapper *lru_w = to_lru_wrapper(alg_w);
  int r = 0;

  // if fields to print passed as argument
  if (fields_str != NULL) {
    struct alg_field lru_fields[] = {{"lru_q", &lru_w->lru_q}, {NULL, NULL}};

    r = alg_wrapper_read(fields_str, lru_fields);
    if (r) {
      return r;
    }
  }

  return r;
}

void lru_wrapper_print(struct alg_wrapper *alg_w) {
  struct lru_wrapper *lru_w = to_lru_wrapper(alg_w);
  struct lru_policy *lru = lru_w->lru;

  if (lru_w->lru_q) {
    LOG_PRINT_F(stderr, "lru_q: ");
    bool first = true;
    struct list_head *lru_q_lh = &lru->lru_q.queue;
    struct list_head *lru_e_lh = &queue_peek(&lru->lru_q)->list;
    while (lru_e_lh != lru_q_lh) {
      struct queue_head *qh = container_of(lru_e_lh, struct queue_head, list);
      struct lru_entry *le = container_of(qh, struct lru_entry, lru_list);

      if (first) {
        first = false;
      } else {
        LOG_PRINT_F(stderr, "->");
      }

      LOG_PRINT_F(stderr, "(%lu)", le->e.oblock);

      lru_e_lh = lru_e_lh->next;
    }
    LOG_PRINT_F(stderr, "\n");
  }
}

struct alg_wrapper *lru_wrapper_create(cblock_t cache_size,
                                       cblock_t meta_size) {
  struct lru_wrapper *lru_w = mem_alloc(sizeof(*lru_w));
  struct cache_nucleus *cn;

  if (lru_w == NULL) {
    goto lru_wrapper_create_fail;
  }

  // create lru
  cn = lru_create(cache_size, meta_size);

  if (cn == NULL) {
    goto lru_policy_create_fail;
  }

  // initialize cache_nucleus pointer of alg_wrapper
  lru_w->alg_w.nucleus = cn;
  lru_w->lru = to_lru_policy(cn);

  // initialize functions of alg_wrapper
  alg_wrapper_init_functions(&lru_w->alg_w, lru_wrapper_init,
                             lru_wrapper_print);

  // initialize flags to false
  lru_w->lru_q = false;

  return &lru_w->alg_w;

lru_policy_create_fail:
  mem_free(lru_w);
lru_wrapper_create_fail:
  return NULL;
}
