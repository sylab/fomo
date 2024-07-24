#ifndef SIM_EXT_SIM_OUTPUTTER_H
#define SIM_EXT_SIM_OUTPUTTER_H

#include "alg_wrapper.h"
#include "cache_nucleus.h"
#include "common.h"
#include "policy_stats.h"
#include "sim_stats_struct.h"

enum sim_output_mode { DEFAULT, WATCHER };

struct sim_outputter {
  enum sim_output_mode mode;

  uint64_t output_interval;

  // uint64_t sampling_rate;

  // pointers to whatever we will interact/print from
  // belongs to functions or....?
};

void sim_outputter_init(struct sim_outputter *out, enum sim_output_mode mode,
                        uint64_t output_interval) {
  out->mode = mode;
  out->output_interval = output_interval;
  // out->sampling_rate = sampling_rate;
}

// TODO remove io and instead use alg_w->nucleus->time?
void sim_outputter_print(struct sim_outputter *out, struct alg_wrapper *alg_w,
                         struct sim_stats_struct *sim_stats, int64_t io,
                         bool complete) {
  struct policy_stats stats;

  if (!complete && (out->output_interval == 0 || io % out->output_interval)) {
    return;
  }

  switch (out->mode) {
  case WATCHER:
    alg_wrapper_print(alg_w);
    break;
  default:
    stats_init(&stats);
    policy_get_stats(alg_w->nucleus, &stats);
    LOG_PRINT("%u %u %u %u %u %u %u %u %u %u %u", stats.hits, stats.misses,
              stats.filters, stats.promotions, stats.demotions,
              stats.private_stat, sim_stats->read_hits, sim_stats->read_misses,
              sim_stats->write_hits, sim_stats->write_misses,
              sim_stats->dirty_evicts);
    break;
  }
}

#endif /* SIM_EXT_SIM_OUTPUTTER_H */
