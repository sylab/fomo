#ifndef TRACE_READER_TRACE_READER_H
#define TRACE_READER_TRACE_READER_H

#include "trace_reader/trace_reader_structs.h"
#include <string.h>

// List of trace readers
#include "trace_reader/basic_trace.h"
#include "trace_reader/fiu_trace.h"
#include "trace_reader/msr_trace.h"
#include "trace_reader/nexus_trace.h"
#include "trace_reader/visa_trace.h"
#include "trace_reader/vscsi_trace.h"

#define TRACE_READER_NAME_MAX_LENGTH 20

/** Checks whether the two trace reader name strings match, ignoring case.
 *
 * NOTES:
 *   1) Name size limited for safety purposes to TRACE_READER_NAME_MAX_LENGTH
 *   2) Is meant to be private to trace_reader, hence the naming scheme where
 *      the function name starts with two underscores.
 */
static bool __trace_reader_names_match(const char *name_a, const char *name_b) {
  return strncasecmp(name_a, name_b, TRACE_READER_NAME_MAX_LENGTH) == 0;
}

static struct trace_reader *find_trace_reader(const char *name) {
  if (__trace_reader_names_match(name, "basic")) {
    return &basic_trace;
  }
  if (__trace_reader_names_match(name, "fiu")) {
    return &fiu_trace;
  }
  if (__trace_reader_names_match(name, "msr")) {
    return &msr_trace;
  }
  if (__trace_reader_names_match(name, "nexus")) {
    return &nexus_trace;
  }
  if (__trace_reader_names_match(name, "visa")) {
    return &visa_trace;
  }
  if (__trace_reader_names_match(name, "vscsi")) {
    return &vscsi_trace;
  }
  return NULL;
}

#endif /* TRACE_READER_TRACE_READER_H */
