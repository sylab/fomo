#ifndef SIM_SIM_OPTIONS_H
#define SIM_SIM_OPTIONS_H

#include "ext/sim_outputter.h"
#include "types.h"
#include <stdio.h>

struct sim_options {
  FILE *fp;
  char *policy_name;
  cblock_t cache_size;
  char *trace_name;
  uint64_t duration_hrs;
  int64_t metadata_size;
  uint64_t window_size;
  enum sim_output_mode output_mode;
  uint64_t freq_count_min;
  uint64_t sampling_rate;
  uint64_t migration_delay;
  int64_t remove_rate;
  char *watch_str;
};

#endif /* SIM_SIM_OPTIONS_H */
