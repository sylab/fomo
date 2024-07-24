#ifndef SIM_SIM_ARGS_H
#define SIM_SIM_ARGS_H

#include "sim_options.h"
#include "trace_reader/trace_reader.h"
#include <ctype.h>
#include <getopt.h>
#include <stdio.h>
#include <unistd.h>

void handle_optional_args(int argc, char **argv, struct sim_options *options) {
  char c;

  static struct option long_options[] = {
      {"file", required_argument, 0, 'f'},
      {"help", no_argument, 0, '`'},
      {"duration", required_argument, 0, 'd'},
      {"metadata-size", required_argument, 0, 'm'},
      {"window-size", required_argument, 0, 'w'},
      {"freq-count", required_argument, 0, 'c'},
      {"hoard-rate", no_argument, 0, 'h'},
      {"recency-classifier", no_argument, 0, 'r'},
      {"sampling-rate", required_argument, 0, 's'},
      {"lir-hir-count", no_argument, 0, 'l'},
      {"migration-delay", required_argument, 0, ','},
      {"remove-rate", required_argument, 0, '<'},
      {"watch", required_argument, 0, '>'},
      {0, 0, 0, 0},
  };
  int option_index = 0;
  char duration_time;

  while ((c = getopt_long(argc, argv, "d:f:m:w:c:s:hrl", long_options,
                          &option_index)) != -1) {
    switch (c) {
    case 'f':
      options->fp = fopen(optarg, "r");
      if (!options->fp) {
        LOG_FATAL("File %s could not be opened. Errno = %d", optarg, errno);
      }
      break;
    case '`':
      LOG_PRINT(
          "Usage: ./cache-sim [ALGORITHM] [CACHE_SIZE] [TRACE_FORMAT] "
          "[OPTION]...\n"
          "Simulate a cache of size CACHE_SIZE, running ALGORITHM over a "
          "TRACE_FORMAT.\n\n"
          "  ALGORITHM        caching algorithm (such as lru)\n"
          "  CACHE_SIZE       size of the cache in entries\n"
          "  TRACE_FORMAT     format of the trace being processed\n\n"
          "With no -f or --file OPTION, read standard input.\n\n"
          "  -f, --file       file of TRACE_TYPE to open and simulate for\n"
          "  -d, --duration   amount of the trace, based on time, that\n"
          "                   is going to be processed\n"
          "                   Supported time designations:\n"
          "                     XXh   XX hours\n"
          "                     XXd   XX days\n"
          "  -m, --metadata-size\n"
          "                   set the size of the metadata for the algorithm\n"
          "                   should the algorithm support it\n"
          "  -w, --window-size\n"
          "                   set the size of the window that regulates how\n"
          "                   often stats are printed.\n"
          "                   Default of 0 means print stats when run ends.\n"
          "  -c, --freq-count\n"
          "                   instead of outputting the default stats, output\n"
          "                   the count of how many entries in cache have a\n"
          "                   frequency greater than or equal to X\n"
          "  -h, --hoard-rate\n"
          "                   instead of outputting the default stats, output\n"
          "                   the count of how many entries in the cache are\n"
          "                   being \"hoarded\"\n"
          "  -s, --sampling-rate\n"
          "                   use a sampling rate to approximate results\n"
          "                   will also decrease cache size appropriately\n"
          "  -l, --lir-hit-count\n"
          "                   count the number of lir and hir entries in\n"
          "                   last N (cache size)  unique accesses\n"
          "                   output: [HIR count] [LIR count]\n"
          "      --migration-delay\n"
          "                   delay the completion of entry migration by\n"
          "                   the given number of accesses\n"
          "      --remove-rate\n"
          "                   remove the accessed entry at a particular rate.\n"
          "                   If > 0, it will remove every x accesses.\n"
          "                   If = 0, it will be disabled.\n"
          "                   If < 0, random removal will be enabled,\n"
          "                   where there is a 1/x chance to remove.\n"
          "      --help       display this help and exit\n\n"
          "Examples:\n"
          "  ./cache-sim lru 10 basic\n"
          "      Run lru cache (of size 10 entries) with standard input in\n"
          "      basic trace format\n"
          "  ./cache-sim lru 10 basic -f example.trace\n"
          "      Run lru cache (of size 10 entries) with example.trace (which\n"
          "      is a basic trace format)\n\n");
      exit(0);
      break;
    case 'd':
      sscanf(optarg, "%lu%c", &options->duration_hrs, &duration_time);
      switch (duration_time) {
      case 'd':
        options->duration_hrs *= 24;
      case 'h':
        break;
      default:
        LOG_FATAL("Unknown duration time %c", duration_time);
      }
      break;
    case 'm':
      sscanf(optarg, "%ld", &options->metadata_size);
      LOG_ASSERT(options->metadata_size >= 0);
      break;
    case 'w':
      sscanf(optarg, "%lu", &options->window_size);
      break;
    case 's':
      sscanf(optarg, "%lu", &options->sampling_rate);
      LOG_ASSERT(options->sampling_rate > 0u);
      break;
    case ',':
      sscanf(optarg, "%lu", &options->migration_delay);
      break;
    case '<':
      sscanf(optarg, "%ld", &options->remove_rate);
      break;
    case '>':
      options->watch_str = optarg;
      options->output_mode = WATCHER;
      break;
    case '?':
      if (isprint(optopt)) {
        LOG_FATAL("Unknown option `-%c`", optopt);
      } else {
        LOG_FATAL("Unknown option character `\\x%x`", optopt);
      }
    default:
      LOG_FATAL("Should've expected the unknown unknown");
    }
  }
}

void handle_required_args(int argc, char **argv, struct sim_options *options) {
  if (argc < 3) {
    LOG_FATAL("Missing non-optional arguments\n"
              "Try 'cache-sim --help' for more information.");
  }

  options->policy_name = argv[0];
  if (find_policy(options->policy_name) == NULL) {
    LOG_FATAL("Unknown caching algorithm %s", options->policy_name);
  }

  if (sscanf(argv[1], "%u", &options->cache_size) != 1) {
    LOG_FATAL("Cache size given was not a number `%s`", argv[1]);
  }

  if (options->metadata_size == -1) {
    options->metadata_size = options->cache_size;
  }

  options->trace_name = argv[2];
  if (!find_trace_reader(options->trace_name)) {
    LOG_FATAL("Unknown trace type %s", options->trace_name);
  }
}

void handle_args(int argc, char **argv, struct sim_options *options) {
  handle_optional_args(argc, argv, options);
  handle_required_args(argc - optind, argv + optind, options);
}

#endif /* SIM_SIM_ARGS_H */
