#include "common.h"
#include "mstar_phase.h"

int window_stats_init(struct window_stats *stats, unsigned size) {
  if (size > 0) {
    stats->values = mem_alloc(sizeof(*stats->values) * size);
    if (stats->values == NULL) {
      return -ENOMEM;
    }
  }

  stats->sum = 0;
  stats->start = 0;
  stats->end = 0;
  stats->len = 0;
  stats->size = size;

  return 0;
}

void window_stats_exit(struct window_stats *stats) { mem_free(stats->values); }

void window_stats_clear(struct window_stats *stats) {
  stats->sum = 0;
  stats->start = 0;
  stats->end = 0;
  stats->len = 0;
}

// NOTE: REQUIRES to->size == from->size
void window_stats_copy(struct window_stats *to, struct window_stats *from) {
  unsigned i;
  for (i = 0; i < from->size; i++) {
    to->values[i] = from->values[i];
  }
  to->sum = from->sum;
  to->start = from->start;
  to->end = from->end;
  to->len = from->len;
}

// NOTE: window_stats_pop does nothing if size == 0
//       since there's no history to "pop"
void window_stats_pop(struct window_stats *stats) {
  unsigned len = stats->len;
  unsigned size = stats->size;
  unsigned start = stats->start;

  if (len > 0 && size > 0) {
    if (len == size) {
      stats->sum -= stats->values[start];
    }
    stats->start = (start + 1) % size;
    stats->len--;
  }
}

void window_stats_push(struct window_stats *stats, unsigned value) {
  if (stats->size > 0) {
    if (stats->len == stats->size) {
      window_stats_pop(stats);
    }
    stats->values[stats->end] = value;
    stats->end = (stats->end + 1) % stats->size;
  }
  stats->sum += value;
  stats->len++;
}

unsigned window_stats_average(struct window_stats *stats) {
  if (stats->len == 0) {
    return 0;
  }
  return stats->sum / stats->len;
}

unsigned window_stats_average_hundredths(struct window_stats *stats) {
  if (stats->len == 0) {
    return 0;
  }
  return (100 * stats->sum) / stats->len;
}

int phase_stats_init(struct phase_stats *stats, unsigned size) {
  int r;
  r = window_stats_init(&stats->cache_stats, size);
  if (r) {
    goto cache_stats_init_fail;
  }
  r = window_stats_init(&stats->ghost_stats, size);
  if (r) {
    goto ghost_stats_init_fail;
  }

  return 0;

ghost_stats_init_fail:
  window_stats_exit(&stats->cache_stats);
cache_stats_init_fail:
  return r;
}

void phase_stats_exit(struct phase_stats *stats) {
  window_stats_exit(&stats->cache_stats);
  window_stats_exit(&stats->ghost_stats);
}

// NOTE: hitrate in hundredths
unsigned phase_cache_hitrate(struct phase_stats *stats) {
  return window_stats_average_hundredths(&stats->cache_stats);
}

// NOTE: hitrate in hundredths
unsigned phase_ghost_hitrate(struct phase_stats *stats) {
  return window_stats_average_hundredths(&stats->ghost_stats);
}

void phase_stats_clear(struct phase_stats *stats) {
  window_stats_clear(&stats->cache_stats);
  window_stats_clear(&stats->ghost_stats);
}

void phase_stats_copy(struct phase_stats *to, struct phase_stats *from) {
  window_stats_copy(&to->cache_stats, &from->cache_stats);
  window_stats_copy(&to->ghost_stats, &from->ghost_stats);
}
