#ifndef MSTAR_TRANSITIONS_H
#define MSTAR_TRANSITIONS_H

#include "common.h"

struct mstar_logic;

enum mstar_state_id { STABLE, UNSTABLE, UNIQUE };

struct mstar_state {
  enum mstar_state_id id;
  void (*change_if_able)(struct mstar_logic *);
};

struct mstar_transition {
  unsigned min;
  unsigned max;
  unsigned stat;
  bool (*was_improvement)(struct mstar_logic *);
  void (*increase_difficulty)(struct mstar_logic *);
  void (*decrease_difficulty)(struct mstar_logic *);
};

// NOTE: assumes cur is a percent in thousandths
bool transition_improved_hitrate(struct mstar_logic *logic);
bool transition_improved_unique(struct mstar_logic *logic);
void transition_increase_param(struct mstar_logic *logic);
void transition_decrease_param(struct mstar_logic *logic);

struct mstar_transition_set {
  struct mstar_transition unstable_stable_threshold;
  struct mstar_transition unstable_stable_percent;
  struct mstar_transition unstable_unique_threshold;
  struct mstar_transition unstable_unique_percent;
  struct mstar_transition stable_unstable_percent;
  struct mstar_transition unique_unstable_threshold;
  struct mstar_transition unique_unstable_percent;
};

void mstar_transition_copy(struct mstar_transition *to,
                           struct mstar_transition *from);
void mstar_transition_set_init(struct mstar_transition_set *set);

void change_from_unstable_if_able(struct mstar_logic *logic);
void change_from_stable_if_able(struct mstar_logic *logic);
void change_from_unique_if_able(struct mstar_logic *logic);

static struct mstar_state mstar_unstable = {
    .id = UNSTABLE,
    .change_if_able = change_from_unstable_if_able,
};

static struct mstar_state mstar_stable = {
    .id = STABLE,
    .change_if_able = change_from_stable_if_able,
};

static struct mstar_state mstar_unique = {
    .id = UNIQUE,
    .change_if_able = change_from_unique_if_able,
};

#endif
