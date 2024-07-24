#ifndef WORKINGSET_SIZE_SET_SIZE_ARGS_H
#define WORKINGSET_SIZE_SET_SIZE_ARGS_H

#include "set_size_options.h"
#include "trace_reader/trace_reader.h"
#include <ctype.h>
#include <getopt.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>

void handle_optional_args(int argc, char **argv, struct set_size_options *options) {
  char c;

  static struct option long_options[] = {
      {"file", required_argument, 0, 'f'},
      {"help", no_argument, 0, '`'},
      {"duration", required_argument, 0, 'd'},
      {"sampling-rate", required_argument, 0, 's'},
      {0, 0, 0, 0},
  };
  int option_index = 0;
  char duration_time;

  while ((c = getopt_long(argc, argv, "d:f:s:", long_options,
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
          "Usage: ./set-size [TRACE_FORMAT] [OPTION]...\n"
          "Find the size of the trace's working set.\n"
          "  TRACE_FORMAT     format of the trace being processed\n\n"
          "With no -f or --file OPTION, read standard input.\n\n"
          "  -f, --file       file of TRACE_TYPE to open and simulate for\n"
          "  -d, --duration   amount of the trace, based on time, that\n"
          "                   is going to be processed\n"
          "                   Supported time designations:\n"
          "                     XXh   XX hours\n"
          "                     XXd   XX days\n"
          "      --help       display this help and exit\n\n"
          "Examples:\n"
          "  ./set-size fiu\n"
          "      Get working set size for FIU trace pass with standard input\n"
          "  ./set-size msr -f example.trace\n"
          "      Get working set size for MSR trace example.trace\n\n");
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

void handle_required_args(int argc, char **argv, struct set_size_options *options) {
  if (argc < 1) {
    LOG_FATAL("Missing non-optional arguments\n"
              "Try 'set-size --help' for more information.");
  }

  options->trace_name = argv[0];
  if (!find_trace_reader(options->trace_name)) {
    LOG_FATAL("Unknown trace type %s", options->trace_name);
  }
}

void handle_args(int argc, char **argv, struct set_size_options *options) {
  handle_optional_args(argc, argv, options);
  handle_required_args(argc - optind, argv + optind, options);
}

#endif /* WORKINGSET_SIZE_SET_SIZE_ARGS_H */
