#ifndef INCLUDE_TOOLS_ARGS_H
#define INCLUDE_TOOLS_ARGS_H

#include "common.h"
#include "tools/logs.h"
#include <string.h>

static char *optarg = NULL;
static int optind = -1;

struct arg_option {
  char *id;
  bool following_argument;
  char short_id;
};

// Return 1 if short arg, 2 if long arg, 0 or more if not either
// If needed, move to enum to add further support for odd argument types.
// Thinking of adding a +arg type for algorithms eventually
static int __get_arg_type(char *arg) {
  int count = 0;
  while (*arg != '\0' && *arg == '-') {
    count++;
    arg++;
  }
  return count;
}

static char __get_short_args(int argc, char **argv, struct arg_option *args,
                             int *index) {
  char *arg = argv[*index] + 1;
  char c = *arg;

  for (struct arg_option *opt = args; opt->id != NULL; opt++) {
    if (opt->short_id == c) {
      if (opt->following_argument) {
        if ((*index) + 1 >= optind) {
          // TODO consider new signal for no following argument
          return -1;
        }
        optarg = argv[*index + 1];
        *index = *index + 2;
      } else {
        if (*(arg + 1) == '\0') {
          (*index)++;
        } else {
          // overwrite arg with new one without c
          char *p = arg + 1;
          while (*arg != '\0') {
            *arg++ = *p++;
          }
        }
      }
      return c;
    }
  }

  // no match found
  (*index)++;
  return '?';
}

static char __get_long_arg(int argc, char **argv, struct arg_option *args,
                           int *index) {
  char *arg = argv[*index] + 2;
  char *s = arg;

  for (struct arg_option *opt = args; opt->id != NULL; opt++) {
    if (strcmp(opt->id, s) == 0) {
      if (opt->following_argument) {
        if ((*index) + 1 >= optind) {
          // TODO consider new signal for no following argument
          return -1;
        }
        optarg = argv[*index + 1];
        *index = (*index) + 2;
      }
      return opt->short_id;
    }
  }

  // no match found
  (*index)++;
  return '?';
}

static char get_optional_arg(int argc, char **argv, struct arg_option *args,
                             int *index) {
  // only initialize once
  if (optind == -1) {
    optind = argc;
  }

  if (args == NULL) {
    return -1;
  }

  for (; *index < optind; (*index)++) {
    char *arg = argv[*index];

    // Determine which argument type arg is:
    //       -abc == 1 == short_args a, b, and c
    //  --example == 2 == long_arg example
    int arg_type = __get_arg_type(arg);

    if (arg_type == 0) {
      // bubble required argument to top
      // then decrement optind
      for (int i = (*index); i < argc - 1; i++) {
        char *tmp = argv[i];
        argv[i] = argv[i + 1];
        argv[i + 1] = tmp;
      }
      optind--;
      (*index)--;
    } else if (arg_type == 1) {
      return __get_short_args(argc, argv, args, index);
    } else if (arg_type == 2) {
      return __get_long_arg(argc, argv, args, index);
    }
  }

  return -1;
}

#endif /* INCLUDE_TOOLS_ARGS_H */
