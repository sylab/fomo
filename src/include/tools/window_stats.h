#ifndef INCLUDE_TOOLS_WINDOW_STATS_H
#define INCLUDE_TOOLS_WINDOW_STATS_H

#include "common.h"

struct window_stats {
  uint64_t *values;
  uint64_t sum;
  unsigned start;
  unsigned end;
  unsigned len;
  unsigned size;
};

static void window_stats_clear(struct window_stats *stats) {
  stats->sum = 0;
  stats->start = 0;
  stats->end = 0;
  stats->len = 0;
}

static int window_stats_init(struct window_stats *stats, unsigned size) {
  stats->values = mem_alloc(size * sizeof(*(stats->values)));
  if (stats->values == NULL) {
    return -ENOSPC;
  }

  window_stats_clear(stats);

  stats->size = size;

  return 0;
}

static void window_stats_exit(struct window_stats *stats) {
  if (stats->values != NULL) {
    mem_free(stats->values);
  }
}

static uint64_t window_stats_peek(struct window_stats *stats) {
  if (stats->len < stats->size) {
    return 0;
  }
  return stats->values[stats->start];
}

static void window_stats_pop(struct window_stats *stats) {
  if (stats->len == 0) {
    return;
  }

  stats->sum -= window_stats_peek(stats);
  stats->start = (stats->start + 1) % stats->size;
  stats->len--;
}

static void window_stats_push(struct window_stats *stats, uint64_t value) {
  if (stats->len == stats->size) {
    window_stats_pop(stats);
  }

  stats->values[stats->end] = value;
  stats->sum += value;
  stats->end = (stats->end + 1) % stats->size;
  stats->len++;
}

static bool window_stats_full(struct window_stats *stats) {
  return stats->len == stats->size;
}

#endif /* INCLUDE_TOOLS_WINDOW_STATS_H */
