#ifndef ALGS_LIRS_LIRS_POLICY_STRUCT_H
#define ALGS_LIRS_LIRS_POLICY_STRUCT_H

struct lirs_policy {
  struct cache_nucleus nucleus;
  struct queue q;
  struct queue s;

  // queue for HIR entries in S
  struct queue r;

  // queue for ghost HIR entries in S
  struct queue g;

  cblock_t hir_limit;
  cblock_t lir_count;

  cblock_t s_len;
  cblock_t s_maxlen;

  oblock_t last_oblock;
};

struct lirs_entry {
  struct entry e;
  // q_list is used for both HIR entries in q and g
  struct queue_head q_list;
  struct queue_head s_list;

  struct queue_head r_list;

  bool is_HIR;
};

static struct lirs_policy *to_lirs_policy(struct cache_nucleus *cn) {
  return container_of(cn, struct lirs_policy, nucleus);
}

static struct lirs_entry *to_lirs_entry(struct entry *e) {
  return container_of(e, struct lirs_entry, e);
}

#endif /* ALGS_LIRS_LIRS_POLICY_STRUCT_H */
