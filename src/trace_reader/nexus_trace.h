#ifndef TRACE_READER_NEXUS_TRACE_H
#define TRACE_READER_NEXUS_TRACE_H

#include "trace_reader/trace_reader_structs.h"
#include "types.h"
#include <stdio.h>
#include <stdlib.h>

/* The Nexus traces use starting address in sectors (lba) and access size in
 * sectors or blocks.
 *
 * Since the address and size are in blocks, we need to increment by pages (+8
 * blocks or 4 KB) to keep the block address in blocks as in the FIU traces.
 *
 * Format:
 * [access start addr] [size in sectors/blocks of 512 bytes] [size in bytes]
 * [access type and waiting] [request generate time] [request process time]
 * [submitted request to hw] [request finish time]
 *
 * Nexus traces include a request generate time, which is the one use in this
 * reader to support the duration feature.
 */

static const block_t NEXUS_BLOCK_SIZE = 512;
static const block_t NEXUS_PAGE_SIZE = 4096;
static const block_t NEXUS_BLOCKS_PER_PAGE = 8;
// nanosecond -> second -> minute -> hour
static const long long NEXUS_HOUR_LENGTH = 1000000000L * 60 * 60;

/** nexus_struct
 * Tracks trace information
 */
struct nexus_struct {
  oblock_t addr;
  bool write;
  block_t pages_left;
  bool starting_time_set;
  uint64_t starting_time;
  uint64_t ending_time;
};

struct nexus_struct nexus_info = {
    .addr = 0,
    .write = false,
    .pages_left = 0,
};

static int nexus_trace_init(FILE *file, unsigned duration_hrs);
static int nexus_trace_read(struct trace_reader_result *result);

struct trace_reader nexus_trace = {
    .features = {0},
    .file = NULL,
    .init = nexus_trace_init,
    .read = nexus_trace_read,
};

static int nexus_trace_init(FILE *file, unsigned duration_hrs) {
  nexus_trace.file = file;

  if (duration_hrs > 0) {
    nexus_trace.features.use_duration = true;
    nexus_trace.features.duration_hrs = duration_hrs;
  } else {
    nexus_trace.features.use_duration = false;
  }

  nexus_info.starting_time_set = false;
  return 0;
}

static int nexus_trace_read(struct trace_reader_result *result) {
  FILE *file = nexus_trace.file;

  if (nexus_info.pages_left > 0) {
    nexus_info.addr += NEXUS_BLOCKS_PER_PAGE;
    --nexus_info.pages_left;
  } else {
    block_t size = 0;
    block_t align;
    unsigned int write = 0;
    float ts;

    while (size == 0) {
      if (fscanf(file, "%lu\t\t%lu\t\t%*u\t\t%u\t\t%f\t\t%*f\t\t%*f\t\t%*f\n",
                 &nexus_info.addr, &size, &write, &ts) != 4) {
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
    align = nexus_info.addr % NEXUS_BLOCKS_PER_PAGE;
    nexus_info.addr -= align;
    size += align;

    if (write == 5 || write == 3) {
      nexus_info.write = true;
    }
    nexus_info.pages_left = (size / NEXUS_BLOCKS_PER_PAGE) - 1;

    // Note: we commented this out since the size in nexus traces has an
    // adittional sector
    //      added by the MMC driver that we should not consider
    // if (size % NEXUS_BLOCKS_PER_PAGE != 0) {
    //  ++nexus_info.pages_left;
    //}

    if (nexus_trace.features.use_duration) {
      if (!nexus_info.starting_time_set) {
        nexus_info.starting_time = ts;
        nexus_info.ending_time =
            ts + (NEXUS_HOUR_LENGTH * nexus_trace.features.duration_hrs);
        nexus_info.starting_time_set = true;
      }
      if (ts > nexus_info.ending_time) {
        LOG_DEBUG("end of duration reached");
        return 1;
      }
    }
  }

  result->oblock = nexus_info.addr;
  result->write = nexus_info.write;

  return 0;
}

#endif
