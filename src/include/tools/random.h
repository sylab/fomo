#ifndef TOOLS_RANDOM_H
#define TOOLS_RANDOM_H

#include "common.h"
#ifdef __KERNEL__
#include <linux/random.h>
#else /* endif __KERNEL__ */
#include <stdlib.h>
#endif /* !__KERNEL__ */

struct random_state {
#ifdef __KERNEL__
  struct rnd_state rnd_state;
#endif
  unsigned prev_random;
  unsigned seed;
};

static void prandom_init_seed(struct random_state *random_state,
                              unsigned seed) {
  random_state->seed = seed;
#ifdef __KERNEL__
  prandom_seed_state(&random_state->rnd_state, seed);
#else
  srandom(seed);
#endif
}

static unsigned random_int(struct random_state *random_state) {
  unsigned random_val;
#ifdef __KERNEL__
  random_val = prandom_u32_state(&random_state->rnd_state);
#else
  random_val = (unsigned)random();
#endif
  random_state->prev_random = random_val;
  return random_val;
}

#endif /* TOOLS_RANDOM_H */
