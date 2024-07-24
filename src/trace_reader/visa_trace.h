#ifndef TRACE_READER_VISA_TRACE_H
#define TRACE_READER_VISA_TRACE_H

#include "trace_reader/trace_reader_structs.h"
#include "types.h"
#include <stdio.h>
#include <stdlib.h>

/* The VISA traces are another set of traces collected in FIU and in the vps
 * cloud They use logical block address (lba) and size (based on blocks of size
 * 512 bytes). Since the FIU traces also are based on filesystem accesses, the
 * accesses are in page granularity (4096 bytes or 8 blocks) as most Linux
 * filesystems use 4KB pages as the base unit for accesses.
 *
 * Since the address and size are in blocks, we need to increment by pages (+8
 * blocks) in large size accesses to keep the block address in blocks.
 *
 * Format:
 * [ts in sec] [pid] [cpu] [process] [lba or starting addr] [seq number]
 * [size in 512 byte blocks] [Write or Read]
 * [major device number] [minor device number]
 *
 * With time measurements included in the trace, FIU traces support the duration
 * feature. Calculations are complicated only by the time in the traces being in
 * the nanosecond scale.
 */

static const block_t VISA_BLOCK_SIZE = 512;
static const block_t VISA_PAGE_SIZE = 4096;
static const block_t VISA_BLOCKS_PER_PAGE = 8;
// second -> minute -> hour
static const long long VISA_HOUR_LENGTH = 60 * 60;

/** visa_struct
 * Tracks trace information
 */
struct visa_struct {
  oblock_t addr;
  bool write;
  block_t pages_left;
  bool starting_time_set;
  uint64_t starting_time;
  uint64_t ending_time;
};

struct visa_struct visa_info = {
    .addr = 0,
    .write = false,
    .pages_left = 0,
};

static int visa_trace_init(FILE *file, unsigned duration_hrs);
static int visa_trace_read(struct trace_reader_result *result);

struct trace_reader visa_trace = {
    .features = {0},
    .file = NULL,
    .init = visa_trace_init,
    .read = visa_trace_read,
};

static int visa_trace_init(FILE *file, unsigned duration_hrs) {
  visa_trace.file = file;

  if (duration_hrs > 0) {
    visa_trace.features.use_duration = true;
    visa_trace.features.duration_hrs = duration_hrs;
  } else {
    visa_trace.features.use_duration = false;
  }

  visa_info.starting_time_set = false;
  return 0;
}

static int visa_trace_read(struct trace_reader_result *result) {
  FILE *file = visa_trace.file;

  if (visa_info.pages_left > 0) {
    visa_info.addr += VISA_BLOCKS_PER_PAGE;
    --visa_info.pages_left;
  } else {
    block_t size = 0;
    block_t align;
    char io[20];
    float ts;

    while (size == 0) {
      if (fscanf(file, "%f %*d %*d %*s %lu %lu %s %*d %*d\n", &ts,
                 &visa_info.addr, &size, io) != 4) {
        if (feof(file)) {
          LOG_DEBUG("end of file reached");
        } else {
          LOG_DEBUG("couldn't read file properly. error %d", ferror(file));
        }
        return 1;
      }
    }

    // align block address
    // remainder blocks from realignment added to size
    align = visa_info.addr % VISA_BLOCKS_PER_PAGE;
    visa_info.addr -= align;
    size += align;

    visa_info.write = io[0] == 'W';
    visa_info.pages_left = (size / VISA_BLOCKS_PER_PAGE) - 1;
    if (size % VISA_BLOCKS_PER_PAGE != 0) {
      ++visa_info.pages_left;
    }

    if (visa_trace.features.use_duration) {
      if (!visa_info.starting_time_set) {
        visa_info.starting_time = ts;
        visa_info.ending_time =
            ts + (VISA_HOUR_LENGTH * visa_trace.features.duration_hrs);
        visa_info.starting_time_set = true;
      }
      if (ts > visa_info.ending_time) {
        LOG_DEBUG("end of duration reached");
        return 1;
      }
    }
  }

  result->oblock = visa_info.addr;
  result->write = visa_info.write;

  return 0;
}

#endif
