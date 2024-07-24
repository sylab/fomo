#ifndef TRACE_READER_TRACE_READER_STRUCTS_H
#define TRACE_READER_TRACE_READER_STRUCTS_H

#include "types.h"
#include <stdio.h>

/* trace_reader and trace_reader_result are in a separate header file from
 * trace_reader.h due to circular dependencies caused by the find_trace_reader
 * function, where the traces such as basic_trace depend on these structs, but
 * find_trace_reader depends on basic_trace, which leads to a circular
 * dependency. So, to sidestep it, the structs are defined seperately.
 */

/** Result of a trace_reader read call
 */
struct trace_reader_result {
  oblock_t oblock; ///< The origin block device address to access
  bool write;      ///< Is the access a write? (If not, it's a read)
};

/** trace_reader features support
 * use_duration - Whether to use duration_hrs or not
 * duration_hrs - amount of the trace, based on hours, that is going to be
 *                processed if supported by the trace format
 */
struct trace_reader_features {
  bool use_duration;
  unsigned duration_hrs;
};

/** A trace_reader to be customized to read a trace
 *
 * A custom trace_reader is intended to be created for each unique trace format,
 * to be indicated by the user.
 *
 * trace_reader includes the FILE pointer since there's only one instance/trace
 * being read at one time, so it should be fine.
 *
 * With init(), this is intended to be extended to support any extra features
 * that may appear as the project continues. With duration_hrs, since hours are
 * the smallest supported unit of time for cache-sim.
 * If duration_hrs is 0, it is assumed that duration is not to be used.
 */
struct trace_reader {
  struct trace_reader_features features; ///< Features of a trace_reader
  FILE *file;                            ///< File being read

  int (*init)(FILE *file,
              unsigned duration_hrs); ///< Function to init the reader
  int (*read)(
      struct trace_reader_result *result); ///< Function to read from the trace
};

#endif
