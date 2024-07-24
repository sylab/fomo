#ifndef TRACE_READER_BASIC_TRACE_H
#define TRACE_READER_BASIC_TRACE_H

#include "trace_reader/trace_reader_structs.h"
#include <stdio.h>
#include <stdlib.h>

/* A basic trace reader, with a format consisting *only* of oblock addresses
 * accessed. As it has no sense of time, each access is simply one after
 * another, it cannot support the duration_hrs feature.
 */

static int basic_trace_init(FILE *file, unsigned duration_hrs);
static int basic_trace_read(struct trace_reader_result *result);

struct trace_reader basic_trace = {
    .features = {0},
    .file = NULL,
    .init = basic_trace_init,
    .read = basic_trace_read,
};

static int basic_trace_init(FILE *file, unsigned duration_hrs) {
  basic_trace.file = file;
  basic_trace.features.use_duration = false; // not supported
  return 0;
}

static int basic_trace_read(struct trace_reader_result *result) {
  FILE *file = basic_trace.file;
  oblock_t oblock;

  if (fscanf(file, "%lu", &oblock) != 1) {
    if (feof(file)) {
      LOG_DEBUG("end of file reached");
    } else {
      LOG_DEBUG("couldn't read file properly. error %d", ferror(file));
    }
    return 1;
  }

  result->oblock = oblock;

  return 0;
}

#endif
