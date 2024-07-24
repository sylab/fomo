#ifndef SIM_SIM_STATS_STRUCT_H
#define SIM_SIM_STATS_STRUCT_H

struct sim_stats_struct {
  unsigned write_hits;
  unsigned write_misses;

  unsigned read_hits;
  unsigned read_misses;

  unsigned dirty_evicts;
};

#endif /* SIM_SIM_STATS_STRUCT_H */
