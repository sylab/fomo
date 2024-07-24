#ifndef MSTAR_LOGIC_H
#define MSTAR_LOGIC_H

#include "mstar_phase.h"
#include "mstar_transitions.h"

struct mstar_logic {
  // NOTE: HR_sample in the paper
  struct phase_stats this_phase;
  // NOTE: for exit_hr, g_hits_old
  //       in new_phase(), g_hits_old updated separately
  struct phase_stats prev_phase;
  // NOTE: HR_state in the paper
  //       size of 0 as it just keeps a running sum and "len"
  struct window_stats state_stats;

  struct mstar_state *state;
  unsigned TABU;
  unsigned tabu_counter;
  void (*enter_new_phase)(struct mstar_logic *);
  struct mstar_transition_set transition_set;
  struct mstar_transition *last_transition;
  bool recent_transition;
};

void static_new_phase(struct mstar_logic *logic);
void adaptive_new_phase(struct mstar_logic *logic);
void update_mstar(struct mstar_logic *logic, bool cache_hit, bool ghost_hit);
void mstar_logic_exit(struct mstar_logic *logic);
int mstar_logic_init(struct mstar_logic *logic, bool adaptive,
                     cblock_t cache_size);

#endif /* MSTAR_LOGIC_H */
