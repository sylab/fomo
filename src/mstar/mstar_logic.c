#include "mstar_logic.h"

void static_new_phase(struct mstar_logic *logic) {
  logic->tabu_counter = logic->TABU;

  // NOTE: equivalent to mstar->g_hits_old = mstar->g_hits
  window_stats_copy(&logic->prev_phase.ghost_stats,
                    &logic->this_phase.ghost_stats);

  logic->state->change_if_able(logic);
}

void adaptive_new_phase(struct mstar_logic *logic) {
  struct mstar_transition *last_trans = logic->last_transition;
  logic->tabu_counter = logic->TABU;

  if (logic->recent_transition) {
    logic->recent_transition = false;
    if (last_trans->was_improvement(logic)) {
      last_trans->decrease_difficulty(logic);
    } else {
      last_trans->increase_difficulty(logic);
    }
  }

  // NOTE: equivalent to mstar->g_hits_old = mstar->g_hits
  window_stats_copy(&logic->prev_phase.ghost_stats,
                    &logic->this_phase.ghost_stats);

  logic->state->change_if_able(logic);
}

void update_mstar(struct mstar_logic *logic, bool cache_hit, bool ghost_hit) {
  struct phase_stats *this_phase = &logic->this_phase;
  unsigned cache_hit_value = cache_hit ? 1u : 0u;
  unsigned ghost_hit_value = ghost_hit ? 1u : 0u;

  window_stats_push(&this_phase->cache_stats, cache_hit_value);
  window_stats_push(&this_phase->ghost_stats, ghost_hit_value);
  window_stats_push(&logic->state_stats, cache_hit_value);

  if (logic->tabu_counter > 0u) {
    logic->tabu_counter--;
  }
  if (logic->tabu_counter == 0) {
    logic->enter_new_phase(logic);
  }
}

void mstar_logic_exit(struct mstar_logic *logic) {
  phase_stats_exit(&logic->this_phase);
  phase_stats_exit(&logic->prev_phase);
  window_stats_exit(&logic->state_stats);
}

int mstar_logic_init(struct mstar_logic *logic, bool adaptive,
                     cblock_t cache_size) {
  int r;
  logic->state = &mstar_unstable;
  logic->recent_transition = false;
  logic->last_transition = NULL;
  logic->enter_new_phase = adaptive ? adaptive_new_phase : static_new_phase;
  logic->TABU = cache_size;
  logic->tabu_counter = 2 * logic->TABU;

  mstar_transition_set_init(&logic->transition_set);
  if (!adaptive) {
    // set static to old starting params
    // US to UN percent:   130 -> 260 (5 -> 10)
    // UN to US threshold:   0 -> 310 (1 -> 10)
    // UN to US percent:   236 -> 473 (5 -> 10)
    logic->transition_set.unstable_unique_percent.stat = 260;
    logic->transition_set.unique_unstable_threshold.stat = 310;
    logic->transition_set.unique_unstable_percent.stat = 473;
  }

  r = phase_stats_init(&logic->this_phase, cache_size);
  if (r) {
    goto this_phase_stats_init_fail;
  }
  r = phase_stats_init(&logic->prev_phase, cache_size);
  if (r) {
    goto prev_phase_stats_init_fail;
  }
  // just a running sum and count
  r = window_stats_init(&logic->state_stats, 0);
  if (r) {
    goto state_stats_init_fail;
  }

  return 0;

state_stats_init_fail:
  phase_stats_exit(&logic->prev_phase);
prev_phase_stats_init_fail:
  phase_stats_exit(&logic->this_phase);
this_phase_stats_init_fail:
  return r;
}
