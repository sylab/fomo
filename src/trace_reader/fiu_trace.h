#ifndef TRACE_READER_FIU_TRACE_H
#define TRACE_READER_FIU_TRACE_H

#include "trace_reader/trace_reader_structs.h"
#include "types.h"
#include <stdio.h>
#include <stdlib.h>

/* The FIU traces use logical block address (lba) and size (based on blocks of
 * size 512 bytes). Since the FIU traces also are based on filesystem accesses,
 * the accesses are in page granularity (4096 bytes or 8 blocks) as most Linux
 * filesystems use 4KB pages as the base unit for accesses.
 *
 * Since the address and size are in blocks, we need to increment by pages (+8
 * blocks) in large size accesses to keep the block address in blocks.
 *
 * Format:
 * [ts in ns] [pid] [process] [lba] [size in 512 byte blocks] [Write or Read]
 * [major device number] [minor device number] [MD5 per 512/4096 bytes]
 *
 * With time measurements included in the trace, FIU traces support the duration
 * feature. Calculations are complicated only by the time in the traces being in
 * the nanosecond scale.
 */

static const block_t FIU_BLOCK_SIZE = 512;
static const block_t FIU_PAGE_SIZE = 4096;
// changed to 8 instead of FIU_PAGE_SIZE / FIU_BLOCK_SIZE due to constant issues
static const block_t FIU_BLOCKS_PER_PAGE = 8;
// nanosecond -> second -> minute -> hour
static const long long FIU_HOUR_LENGTH = 1000000000L * 60 * 60;

/** fiu_struct
 * Tracks trace information
 */
struct fiu_struct {
  oblock_t addr;
  bool write;
  block_t pages_left;
  bool starting_time_set;
  uint64_t starting_time;
  uint64_t ending_time;
};

struct fiu_struct fiu_info = {
    .addr = 0,
    .write = false,
    .pages_left = 0,
};

static int fiu_trace_init(FILE *file, unsigned duration_hrs);
static int fiu_trace_read(struct trace_reader_result *result);

struct trace_reader fiu_trace = {
    .features = {0},
    .file = NULL,
    .init = fiu_trace_init,
    .read = fiu_trace_read,
};

static int fiu_trace_init(FILE *file, unsigned duration_hrs) {
  fiu_trace.file = file;

  if (duration_hrs > 0) {
    fiu_trace.features.use_duration = true;
    fiu_trace.features.duration_hrs = duration_hrs;
  } else {
    fiu_trace.features.use_duration = false;
  }

  fiu_info.starting_time_set = false;
  return 0;
}

static int fiu_trace_read(struct trace_reader_result *result) {
  FILE *file = fiu_trace.file;

  if (fiu_info.pages_left > 0) {
    fiu_info.addr += FIU_BLOCKS_PER_PAGE;
    --fiu_info.pages_left;
  } else {
    block_t size = 0;
    block_t align;
    char io[20];
    uint64_t ts;

    while (size == 0) {
      if (fscanf(file, "%lu %*d %*s %lu %lu %s %*d %*d %*s\n", &ts,
                 &fiu_info.addr, &size, io) != 4) {
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
    align = fiu_info.addr % FIU_BLOCKS_PER_PAGE;
    fiu_info.addr -= align;
    size += align;

    fiu_info.write = io[0] == 'W';
    fiu_info.pages_left = (size / FIU_BLOCKS_PER_PAGE) - 1;
    if (size % FIU_BLOCKS_PER_PAGE != 0) {
      ++fiu_info.pages_left;
    }

    if (fiu_trace.features.use_duration) {
      if (!fiu_info.starting_time_set) {
        fiu_info.starting_time = ts;
        fiu_info.ending_time =
            ts + (FIU_HOUR_LENGTH * fiu_trace.features.duration_hrs);
        fiu_info.starting_time_set = true;
      }
      if (ts > fiu_info.ending_time) {
        LOG_DEBUG("end of duration reached");
        return 1;
      }
    }
  }

  result->oblock = fiu_info.addr;
  result->write = fiu_info.write;

  return 0;
}

#endif
