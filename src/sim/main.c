#include "ext/sim_outputter.h"
#include "kernel/hash.h"
#include "policy_registry/policy_registry.h"
#include "policy_registry/wrapper_registry.h"
#include "sim_args.h"
#include "sim_freq_count.h"
#include "sim_hoard_count.h"
#include "sim_lir_hir_count.h"
#include "sim_migration_tracker.h"
#include "sim_options.h"
#include "sim_policy.h"
#include "sim_recency_classifier.h"
#include "sim_stats_struct.h"
#include "tools/logs.h"
#include "tools/random.h"
#include "trace_reader/trace_reader.h"

struct sim_outputter sim_outputter = {
    .mode = DEFAULT,
    .output_interval = 0,
};

struct sim_stats_struct sim_stats = {
    .write_hits = 0,
    .write_misses = 0,
    .read_hits = 0,
    .read_misses = 0,
    .dirty_evicts = 0,
};

struct sim_options options = {
    .fp = NULL,
    .policy_name = "",
    .cache_size = 0,
    .trace_name = "",
    .duration_hrs = 0,
    .metadata_size = -1,
    .window_size = 0,
    .output_mode = DEFAULT,
    .freq_count_min = 0,
    .sampling_rate = 1,
    .migration_delay = 0,
    .watch_str = "",
};

// TODO do I add these features back in?
// struct freq_tracker f_tracker;
// struct hoard_tracker h_tracker;
// struct recency_classifier r_class;
// struct lir_hir_tracker lh_tracker;
struct migration_tracker m_tracker;

struct random_state random_remove;

bool remove_now(int time) {
  if (options.remove_rate == 0) {
    return false;
  }

  if (options.remove_rate > 0) {
    return time % options.remove_rate == 0;
  }

  return random_int(&random_remove) % (-options.remove_rate) == 0;
}

void sim_prep(int argc, char **argv) {
  unsigned i;

  options.fp = stdin;

  handle_args(argc, argv, &options);

  m_tracker.delay = options.migration_delay;
  if (options.migration_delay > 0 &&
      migration_tracker_init(&m_tracker, options.migration_delay)) {
    LOG_FATAL("unable to allocate for migration_tracker");
  }

  // TODO rename window_size to output_interval?
  uint64_t output_interval = options.window_size;
  sim_outputter_init(&sim_outputter, options.output_mode, output_interval);

  // TODO make sure this is gonna be okay to do
  prandom_init_seed(&random_remove, 123);
}

struct trace_reader *trace_prep() {
  struct trace_reader *reader = find_trace_reader(options.trace_name);
  reader->init(options.fp, options.duration_hrs);
  return reader;
}

struct alg_wrapper *policy_prep() {
  struct alg_wrapper *alg_w = create_wrapper(
      options.policy_name, options.cache_size / options.sampling_rate,
      options.metadata_size / options.sampling_rate);
  if (!alg_w) {
    LOG_FATAL("Base policy wrapper creation failed");
  }
  alg_wrapper_init(alg_w, options.watch_str);
  return alg_w;
}

void sim_map(struct cache_nucleus *bp, struct trace_reader_result *read_result,
             struct cache_nucleus_result *result) {
  struct entry *e = policy_cache_lookup(bp, read_result->oblock);
  if (e != NULL && e->migrating) {
    result->op == CACHE_NUCLEUS_HIT;
    if (read_result->write) {
      ++sim_stats.write_hits;
    } else {
      ++sim_stats.read_hits;
    }
    return;
  }

  if (sim_policy_map(bp, read_result->oblock, read_result->write, result)) {
    LOG_FATAL("Error occurred while processing entry");
  }

  if (result->op == CACHE_NUCLEUS_HIT) {
    if (read_result->write) {
      ++sim_stats.write_hits;
    } else {
      ++sim_stats.read_hits;
    }
  } else {
    if (read_result->write) {
      ++sim_stats.write_misses;
    } else {
      ++sim_stats.read_misses;
    }
  }

  if (result->dirty_eviction) {
    ++sim_stats.dirty_evicts;
  }
}

int sim_access(struct trace_reader *reader, struct cache_nucleus *bp,
               unsigned time) {
  struct cache_nucleus_result result = {0};
  struct trace_reader_result read_result = {0};
  int r;
  int i;

  if (options.sampling_rate == 1) {
    r = reader->read(&read_result);
    if (r) {
      return r;
    }
  } else {
    unsigned P = (2 << 30);
    unsigned T = P / options.sampling_rate;

    do {
      r = reader->read(&read_result);
      if (r) {
        return r;
      }
      read_result.oblock = hash_64(read_result.oblock, ffs(P) - 1);
    } while (read_result.oblock > T);
  }

  sim_policy_set_time(bp, time);
  sim_map(bp, &read_result, &result);

  if (options.migration_delay == 0) {
    if (result.op == CACHE_NUCLEUS_NEW || result.op == CACHE_NUCLEUS_REPLACE) {
      sim_policy_migrated(bp, read_result.oblock);
    }
  } else {
    oblock_t migrated;
    if (migration_next(&m_tracker, time, &migrated)) {
      struct entry *e = policy_cache_lookup(bp, migrated);
      if (e != NULL) {
        sim_policy_migrated(bp, migrated);
      }
    }

    if (result.op == CACHE_NUCLEUS_NEW || result.op == CACHE_NUCLEUS_REPLACE) {
      migration_add(&m_tracker, time, read_result.oblock);
    }
  }

  if (remove_now(time)) {
    struct entry *e = policy_cache_lookup(bp, read_result.oblock);
    if (e != NULL) {
      migration_remove(&m_tracker, read_result.oblock);
      policy_remove(bp, read_result.oblock, true);
    }
  }

  return 0;
}

void sim_exit(struct cache_nucleus *bp) {
  sim_policy_destroy(bp);
  fclose(options.fp);
}

int main(int argc, char **argv) {
  struct trace_reader *reader;
  struct alg_wrapper *alg_w;
  struct cache_nucleus *bp;

  unsigned time = 1;

  sim_prep(argc, argv);
  reader = trace_prep();
  alg_w = policy_prep();
  bp = alg_w->nucleus;

  while (!sim_access(reader, bp, time)) {
    sim_outputter_print(&sim_outputter, alg_w, &sim_stats, time, false);

    ++time;
  }

  sim_outputter_print(&sim_outputter, alg_w, &sim_stats, time, true);

  sim_exit(bp);

  return 0;
}
