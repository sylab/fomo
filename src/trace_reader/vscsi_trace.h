#ifndef TRACE_READER_VSCSI_TRACE_H
#define TRACE_READER_VSCSI_TRACE_H

#include "trace_reader/trace_reader_structs.h"
#include "types.h"
#include <stdio.h>
#include <stdlib.h>

/* The VSCSI traces use logical block address (lba) and size (based on blocks of
 * size 512 bytes). Since the VSCSI traces also are based on filesystem
 * accesses, the accesses are in page granularity (4096 bytes or 8 blocks) as
 * most Linux filesystems use 4KB pages as the base unit for accesses.
 *
 * Since the address and size are in blocks, we need to increment by pages (+8
 * blocks) in large size accesses to keep the block address in blocks.
 *
 * Format:
 * [Write or Read] [ts in s (float)] [lba] [size in 512 byte blocks]
 *
 * With time measurements included in the trace, VSCSI traces support the
 * duration feature. Calculations are complicated only by the time in the traces
 * being in the nanosecond scale.
 */

static const block_t VSCSI_BLOCK_SIZE = 512;
static const block_t VSCSI_PAGE_SIZE = 4096;
// changed to 8 instead of VSCSI_PAGE_SIZE / VSCSI_BLOCK_SIZE due to constant
// issues
static const block_t VSCSI_BLOCKS_PER_PAGE = 8;
// second -> minute -> hour
static const long long VSCSI_HOUR_LENGTH = 60 * 60;

/** vscsi_struct
 * Tracks trace information
 */
struct vscsi_struct {
  oblock_t addr;
  bool write;
  block_t pages_left;
  bool starting_time_set;
  double starting_time;
  double ending_time;
};

struct vscsi_struct vscsi_info = {
    .addr = 0,
    .write = false,
    .pages_left = 0,
};

static int vscsi_trace_init(FILE *file, unsigned duration_hrs);
static int vscsi_trace_read(struct trace_reader_result *result);

struct trace_reader vscsi_trace = {
    .features = {0},
    .file = NULL,
    .init = vscsi_trace_init,
    .read = vscsi_trace_read,
};

static int vscsi_trace_init(FILE *file, unsigned duration_hrs) {
  vscsi_trace.file = file;

  if (duration_hrs > 0) {
    vscsi_trace.features.use_duration = true;
    vscsi_trace.features.duration_hrs = duration_hrs;
  } else {
    vscsi_trace.features.use_duration = false;
  }

  vscsi_info.starting_time_set = false;
  return 0;
}

static int vscsi_trace_read(struct trace_reader_result *result) {
  FILE *file = vscsi_trace.file;

  if (vscsi_info.pages_left > 0) {
    vscsi_info.addr++;
    --vscsi_info.pages_left;
  } else {
    block_t size = 0;
    block_t align;
    char io[20];
    double ts;

    while (size == 0) {
      if (fscanf(file, "%s %lf %lu %lu\n", io, &ts, &vscsi_info.addr, &size) !=
          4) {
        if (feof(file)) {
          LOG_DEBUG("end of file reached");
        } else {
          LOG_DEBUG("couldn't read file properly. error %d", ferror(file));
        }
        return 1;
      }
    }

    vscsi_info.write = io[0] == 'W';
    vscsi_info.pages_left = size / VSCSI_BLOCK_SIZE;
    if (size % (VSCSI_BLOCK_SIZE) != 0) {
      ++vscsi_info.pages_left;
    }
    --vscsi_info.pages_left;

    if (vscsi_trace.features.use_duration) {
      if (!vscsi_info.starting_time_set) {
        vscsi_info.starting_time = ts;
        vscsi_info.ending_time =
            ts + (VSCSI_HOUR_LENGTH * vscsi_trace.features.duration_hrs);
        vscsi_info.starting_time_set = true;
      }
      if (ts > vscsi_info.ending_time) {
        LOG_DEBUG("end of duration reached");
        return 1;
      }
    }
  }

  result->oblock = vscsi_info.addr;
  result->write = vscsi_info.write;

  return 0;
}

#endif
