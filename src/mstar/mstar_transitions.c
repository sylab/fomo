#include "mstar_transitions.h"
#include "common.h"
#include "mstar_logic.h"
#include "mstar_phase.h"

// =================== MSTAR_TRANSITION STUFF =====================

// NOTE: assumes cur is a percent in thousandths
unsigned transition_value(struct mstar_transition *trans) {
  return ((trans->max * trans->stat) / 1000) + trans->min;
}

bool transition_improved_hitrate(struct mstar_logic *logic) {
  unsigned this_phase_avg = phase_cache_hitrate(&logic->this_phase);
  unsigned prev_phase_avg = phase_cache_hitrate(&logic->prev_phase);

  return this_phase_avg >= prev_phase_avg;
}

bool transition_improved_unique(struct mstar_logic *logic) {
  unsigned this_phase_avg = phase_cache_hitrate(&logic->this_phase);
  unsigned ghost_hits = logic->this_phase.ghost_stats.sum;
  unsigned ghost_hits_old = logic->prev_phase.ghost_stats.sum;

  // NOTE: previous version had
  //       (((g_hits - g_hits_old) * 100) / TABU) > un_us_th->cur ||
  //       Which doesn't feel very appropriate for an improvement check!
  //       It's the same as the un_us_th check in the old
  //       state_try_transition_from_unique() function!
  //
  // TODO: figure out what you want to do with this
  // NOTE: like this 10% improvement idea, here it is:
  //       ((ghost_hits * 100) >= (ghost_hits_old * 110)) ||
  return ((((ghost_hits - ghost_hits_old) * 100) / logic->TABU) >
          transition_value(&logic->transition_set.unique_unstable_threshold)) ||
         (this_phase_avg >
          transition_value(&logic->transition_set.unique_unstable_percent));
}

void transition_increase_param(struct mstar_logic *logic) {
  struct mstar_transition *last_trans = logic->last_transition;
  unsigned cur = last_trans->stat;

  if (cur == 0u) {
    last_trans->stat = 1000u;
  } else {
    last_trans->stat = min(1000u, cur + (1000u / cur));
  }
}

void transition_decrease_param(struct mstar_logic *logic) {
  struct mstar_transition *last_trans = logic->last_transition;
  unsigned cur = last_trans->stat;
  unsigned tmp;

  if (cur == 1000u) {
    // since at max, move to the other end of the spectrum (unlikely though)
    cur = 0;
  }
  tmp = 1000u / (1000u - cur);
  // NOTE: working with unsigned, so avoid going to negatives
  if (cur > tmp) {
    last_trans->stat = max(0u, cur - tmp);
  } else {
    last_trans->stat = 0u;
  }
}

struct mstar_transition unstable_stable_threshold = {
    .min = 2,
    .max = 20,
    .stat = 555, // 2 -- 10 -- 20
                 // min  cur   max
                 // convert to
                 // 10 / (20 - 2) = 0.55555..
                 // so, to 1000ths is 555
                 // new scale:
                 // 0 -- 555 -- 1000
    .was_improvement = transition_improved_hitrate,
    .increase_difficulty = transition_decrease_param,
    .decrease_difficulty = transition_increase_param,
};

struct mstar_transition unstable_stable_percent = {
    .stat = 333, // 105 -- 120 -- 150
    .was_improvement = transition_improved_hitrate,
    .increase_difficulty = transition_increase_param,
    .decrease_difficulty = transition_decrease_param,
    .min = 105,
    .max = 150,
};

struct mstar_transition unstable_unique_threshold = {
    .min = 20,
    .max = 80,
    .stat = 500, // 20 -- 50 -- 80
    .was_improvement = transition_improved_unique,
    .increase_difficulty = transition_decrease_param,
    .decrease_difficulty = transition_increase_param,
};

struct mstar_transition unstable_unique_percent = {
    .min = 2,
    .max = 25,
    .stat = 130, // 2 -- 5 -- 25
    .was_improvement = transition_improved_unique,
    .increase_difficulty = transition_decrease_param,
    .decrease_difficulty = transition_increase_param,
};

struct mstar_transition stable_unstable_percent = {
    .min = 50,
    .max = 90,
    .stat = 500, // 50 -- 70 -- 90
    .was_improvement = transition_improved_hitrate,
    .increase_difficulty = transition_decrease_param,
    .decrease_difficulty = transition_increase_param,
};

struct mstar_transition unique_unstable_threshold = {
    .min = 1,
    .max = 30,
    .stat = 0, // 1 -- 1 -- 30
    .was_improvement = transition_improved_hitrate,
    .increase_difficulty = transition_increase_param,
    .decrease_difficulty = transition_decrease_param,
};

struct mstar_transition unique_unstable_percent = {
    .min = 1,
    .max = 20,
    .stat = 237, // 1 -- 5 -- 20
    .was_improvement = transition_improved_hitrate,
    .increase_difficulty = transition_increase_param,
    .decrease_difficulty = transition_decrease_param,
};

void mstar_transition_copy(struct mstar_transition *to,
                           struct mstar_transition *from) {
  to->stat = from->stat;
  to->was_improvement = from->was_improvement;
  to->increase_difficulty = from->increase_difficulty;
  to->decrease_difficulty = from->decrease_difficulty;
  to->min = from->min;
  to->max = from->max;
}

void mstar_transition_set_init(struct mstar_transition_set *set) {
  mstar_transition_copy(&set->unstable_stable_threshold,
                        &unstable_stable_threshold);
  mstar_transition_copy(&set->unstable_stable_percent,
                        &unstable_stable_percent);
  mstar_transition_copy(&set->unstable_unique_threshold,
                        &unstable_unique_threshold);
  mstar_transition_copy(&set->unstable_unique_percent,
                        &unstable_unique_percent);
  mstar_transition_copy(&set->stable_unstable_percent,
                        &stable_unstable_percent);
  mstar_transition_copy(&set->unique_unstable_threshold,
                        &unique_unstable_threshold);
  mstar_transition_copy(&set->unique_unstable_percent,
                        &unique_unstable_percent);
}

// ===================  MSTAR_STATE STUFF ======================
// mstar_state functions and stuff below
// TODO

void state_transition_to(struct mstar_logic *logic,
                         struct mstar_transition *current_transition,
                         struct mstar_state *next_state,
                         bool clear_state_stats) {
  unsigned this_phase_avg = phase_cache_hitrate(&logic->this_phase);
  unsigned prev_phase_avg = phase_cache_hitrate(&logic->prev_phase);
  unsigned ghost_hits = logic->this_phase.ghost_stats.sum;
  unsigned ghost_hits_old = logic->prev_phase.ghost_stats.sum;

  logic->last_transition = current_transition;
  logic->state = next_state;

  logic->recent_transition = true;

  if (next_state->id == UNSTABLE) {
    logic->tabu_counter = 2 * logic->TABU;
  }

  //
  if (clear_state_stats) {
    window_stats_clear(&logic->state_stats);
  }

  // setup for new phase
  phase_stats_copy(&logic->prev_phase, &logic->this_phase);
  phase_stats_clear(&logic->this_phase);
}

void change_from_unstable_if_able(struct mstar_logic *logic) {
  struct mstar_transition *us_un_th =
      &logic->transition_set.unstable_unique_threshold;
  struct mstar_transition *us_un_p =
      &logic->transition_set.unstable_unique_percent;
  struct mstar_transition *us_s_th =
      &logic->transition_set.unstable_stable_threshold;
  struct mstar_transition *us_s_p =
      &logic->transition_set.unstable_stable_percent;
  unsigned hr_sample = phase_cache_hitrate(&logic->this_phase);
  unsigned hr_state = window_stats_average_hundredths(&logic->state_stats);

  if (transition_value(us_un_th) * hr_state > hr_sample) {
    state_transition_to(logic, us_un_th, &mstar_unique, false);
  } else if (hr_sample < transition_value(us_un_p)) {
    state_transition_to(logic, us_un_p, &mstar_unique, false);
  } else if ((100 * hr_sample < (100 + transition_value(us_s_th)) * hr_state) &&
             (100 * hr_sample > (100 - transition_value(us_s_th)) * hr_state)) {
    state_transition_to(logic, us_s_th, &mstar_stable, true);
  } else if (hr_sample >= transition_value(us_s_p) * hr_state &&
             hr_sample >= 20) {
    state_transition_to(logic, us_s_p, &mstar_stable, true);
  }
}

void change_from_stable_if_able(struct mstar_logic *logic) {
  struct mstar_transition *s_us_p =
      &logic->transition_set.stable_unstable_percent;
  unsigned hr_sample = phase_cache_hitrate(&logic->prev_phase);
  unsigned hr_state = window_stats_average_hundredths(&logic->state_stats);

  if (100 * hr_sample <= transition_value(s_us_p) * hr_state) {
    state_transition_to(logic, s_us_p, &mstar_unstable, false);
  }
}

void change_from_unique_if_able(struct mstar_logic *logic) {
  struct mstar_transition *un_us_th =
      &logic->transition_set.unique_unstable_threshold;
  struct mstar_transition *un_us_p =
      &logic->transition_set.unique_unstable_percent;
  unsigned hr_sample = phase_cache_hitrate(&logic->this_phase);
  unsigned filter_hr_sample = phase_ghost_hitrate(&logic->this_phase);
  // unsigned ghost_hits = logic->this_phase.ghost_stats.sum;
  // unsigned ghost_hits_old = logic->prev_phase.ghost_stats.sum;

  // NOTE: changed logic here looking for a percent improvement
  //       wait.... this is something that should be in the paper...
  // TODO: rewrite and check a *lot* of the logic here for inconsistencies
  //       from the paper.
  // NOTE: changed to reflect the paper, used to be:
  //       ((((g_hits - g_hits_old) * 100) / TABU) > un_us_th->cur)
  if ((transition_value(un_us_th) * hr_sample) > (100 * filter_hr_sample)) {
    state_transition_to(logic, un_us_th, &mstar_unstable, false);
  } else if (hr_sample > transition_value(un_us_p)) {
    state_transition_to(logic, un_us_p, &mstar_unstable, false);
  }
}
