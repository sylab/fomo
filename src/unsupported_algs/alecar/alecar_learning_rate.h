#ifndef SRC_ALGS_ALECAR_ALECAR_LEARNING_RATE_H
#define SRC_ALGS_ALECAR_ALECAR_LEARNING_RATE_H

#include "tools/fixedptc.h"
#include "tools/random.h"

struct alecar_learning_rate {
  fixedpt learning_rate;
  fixedpt discount_rate;

  fixedpt starting_learning_rate;

  unsigned sequence_length;

  unsigned cache_hit;

  fixedpt prev_hit_rate;
  fixedpt new_hit_rate;

  fixedpt prev_change_in_hit_rate;
  fixedpt new_change_in_hit_rate;

  fixedpt prev_learning_rate;
  fixedpt new_learning_rate;

  struct random_state random;

  unsigned hit_rate_zero_counter;
  unsigned hit_rate_negative_counter;
};

static void update_cache_hit(struct alecar_learning_rate *lr) {
  ++lr->cache_hit;
}

static void update_in_delta_direction(struct alecar_learning_rate *lr,
                                      fixedpt delta_learning_rate,
                                      fixedpt *delta, fixedpt *delta_hit_rate) {
  *delta = 0;
  *delta_hit_rate = FIXEDPT_ONE;

  if ((delta_learning_rate > 0 && lr->new_change_in_hit_rate > 0) ||
      (delta_learning_rate < 0 && lr->new_change_in_hit_rate < 0)) {
    *delta = FIXEDPT_ONE;
  } else if ((delta_learning_rate < 0 && lr->new_change_in_hit_rate > 0) ||
             (delta_learning_rate > 0 && lr->new_change_in_hit_rate < 0)) {
    *delta = -FIXEDPT_ONE;
  } else if ((delta_learning_rate > 0 || delta_learning_rate < 0) &&
             lr->new_change_in_hit_rate == 0) {
    *delta_hit_rate = 0;
  }
}

static void update_in_random_direction(struct alecar_learning_rate *lr) {
  fixedpt upper_limit = fixedpt_rconst(0.99);
  fixedpt lower_limit = fixedpt_rconst(0.001);

  if (lr->learning_rate >= upper_limit) {
    lr->learning_rate = fixedpt_rconst(0.9);
  } else if (lr->learning_rate <= lower_limit) {
    lr->learning_rate = fixedpt_rconst(0.002);
  } else {
    // could change lr * 0.25 to lr / 4?
    fixedpt quarter = fixedpt_rconst(0.25);
    // increase = np.random.choice([True, False])
    unsigned limit = 10000;
    bool increase = (random_int(&lr->random) % limit) < (limit / 2);

    if (increase) {
      lr->learning_rate =
          min(fixedpt_add(lr->learning_rate,
                          fixedpt_abs(fixedpt_mul(lr->learning_rate, quarter))),
              FIXEDPT_ONE);
    } else {
      lr->learning_rate =
          max(fixedpt_sub(lr->learning_rate,
                          fixedpt_abs(fixedpt_mul(lr->learning_rate, quarter))),
              lower_limit);
    }
  }
}

static void update_learning_rate(struct alecar_learning_rate *lr) {
  fixedpt delta;
  fixedpt delta_hit_rate;
  fixedpt delta_learning_rate;

  // NOTE: updateLearningRates() only call every sequence_length
  lr->new_hit_rate = fixedpt_div(fixedpt_fromint(lr->cache_hit),
                                 fixedpt_fromint(lr->sequence_length));
  lr->new_change_in_hit_rate = fixedpt_sub(lr->new_hit_rate, lr->prev_hit_rate);

  // delta_LR = new_learning_rate - prev_learning_rate
  // delta, delta_HR = updateInDeltaDirection(delta_LR)
  delta_learning_rate =
      fixedpt_sub(lr->new_learning_rate, lr->prev_learning_rate);
  update_in_delta_direction(lr, delta_learning_rate, &delta, &delta_hit_rate);

  if (delta > 0) {
    lr->learning_rate =
        min(fixedpt_add(lr->learning_rate,
                        fixedpt_abs(fixedpt_mul(lr->learning_rate,
                                                delta_learning_rate))),
            FIXEDPT_ONE);
    lr->hit_rate_negative_counter = 0;
    lr->hit_rate_zero_counter = 0;
  } else if (delta < 0) {
    lr->learning_rate =
        max(fixedpt_sub(lr->learning_rate,
                        fixedpt_abs(fixedpt_mul(lr->learning_rate,
                                                delta_learning_rate))),
            fixedpt_rconst(0.001));
    lr->hit_rate_negative_counter = 0;
    lr->hit_rate_zero_counter = 0;
  } else if (delta == 0 && lr->new_change_in_hit_rate <= 0) {
    // TODO: is this supposed to be <= or ==?
    if (lr->new_hit_rate <= 0) {
      ++lr->hit_rate_zero_counter;
    }

    if (lr->new_change_in_hit_rate < 0) {
      ++lr->hit_rate_negative_counter;
    }

    if (lr->hit_rate_zero_counter >= 20) {
      lr->learning_rate = lr->starting_learning_rate;
      lr->hit_rate_zero_counter = 0;
    } else if (lr->new_change_in_hit_rate < 0) {
      if (lr->hit_rate_negative_counter >= 20) {
        lr->learning_rate = lr->starting_learning_rate;
        lr->hit_rate_negative_counter = 0;
      } else {
        update_in_random_direction(lr);
      }
    }

    lr->prev_learning_rate = lr->new_learning_rate;
    lr->new_learning_rate = lr->learning_rate;
    lr->prev_hit_rate = lr->new_hit_rate;
    lr->prev_change_in_hit_rate = lr->new_change_in_hit_rate;
    lr->cache_hit = 0;
  }
}

static void alecar_learning_rate_init(struct alecar_learning_rate *lr,
                                      unsigned sequence_length) {
  fixedpt optimal_learning_rate = fixedpt_sqrt(
      fixedpt_div(fixedpt_mul(fixedpt_rconst(2), fixedpt_ln(fixedpt_rconst(2))),
                  sequence_length));

  // NOTE: setting up pseudo-random, seed can be changed to whatever else
  prandom_init_seed(&lr->random, 123);

  lr->sequence_length = sequence_length;

  lr->learning_rate = fixedpt_rconst(0.1);
  lr->starting_learning_rate = lr->learning_rate;
  lr->prev_learning_rate = lr->learning_rate;
  lr->new_learning_rate = lr->learning_rate;

  lr->discount_rate = fixedpt_rconst(0.9995);

  lr->cache_hit = 0;
  lr->prev_hit_rate = 0;
  lr->new_hit_rate = 0;

  lr->prev_change_in_hit_rate = 0;
  lr->new_change_in_hit_rate = 0;

  lr->hit_rate_zero_counter = 0;
  lr->hit_rate_negative_counter = 0;
}

#endif /* SRC_ALGS_ALECAR_ALECAR_LEARNING_RATE_H */
