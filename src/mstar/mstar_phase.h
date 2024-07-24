#ifndef MSTAR_PHASE_H
#define MSTAR_PHASE_H

// TODO move windows_stats to include/tools directory
struct window_stats {
  unsigned *values;
  unsigned sum;
  unsigned start;
  unsigned end;
  unsigned len;
  unsigned size;
};

int window_stats_init(struct window_stats *stats, unsigned size);
void window_stats_exit(struct window_stats *stats);
void window_stats_clear(struct window_stats *stats);
// NOTE: REQUIRES to->size == from->size
void window_stats_copy(struct window_stats *to, struct window_stats *from);
// NOTE: window_stats_pop does nothing if size == 0
//       since there's no history to "pop"
void window_stats_pop(struct window_stats *stats);
void window_stats_push(struct window_stats *stats, unsigned value);
unsigned window_stats_average(struct window_stats *stats);
unsigned window_stats_average_hundredths(struct window_stats *stats);

struct phase_stats {
  struct window_stats cache_stats;
  struct window_stats ghost_stats;
};

int phase_stats_init(struct phase_stats *stats, unsigned size);
void phase_stats_exit(struct phase_stats *stats);
// NOTE: hitrate in hundredths
unsigned phase_cache_hitrate(struct phase_stats *stats);
// NOTE: hitrate in hundredths
unsigned phase_ghost_hitrate(struct phase_stats *stats);
void phase_stats_clear(struct phase_stats *stats);
void phase_stats_copy(struct phase_stats *to, struct phase_stats *from);

#endif
