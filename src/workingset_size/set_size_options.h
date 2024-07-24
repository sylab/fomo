#ifndef WORKINGSET_SIZE_SET_SIZE_OPTIONS_H
#define WORKINGSET_SIZE_SET_SIZE_OPTIONS_H

#include "types.h"
#include <stdio.h>

struct set_size_options {
  FILE *fp;
  char *trace_name;
  uint64_t duration_hrs;
};

#endif /* WORKINGSET_SIZE_SET_SIZE_OPTIONS_H */
