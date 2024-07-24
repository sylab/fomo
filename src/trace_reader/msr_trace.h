#ifndef TRACE_READER_MSR_TRACE_H
#define TRACE_READER_MSR_TRACE_H

#include "trace_reader/trace_reader_structs.h"
#include "types.h"
#include <stdio.h>
#include <stdlib.h>

/* The MSR traces have addresses and size in bytes.
 * We find with that the traces were originally collected in blocks, but later
 * converted to bytes for an unknown reason. The traces are in the block
 * granularity, unlike the FIU traces, which are at the page granularity. This
 * can be attributed to them collecting traces using a raw block device, as said
 * in their paper.
 *
 * Format:
 * Timestamp,Hostname,DiskNumber,Type,Offset,Size,ResponseTime
 *
 * The MSR traces support the duration feature. The time they have is described
 * as "Windows Filetime", which is essentially in the scale of 100 nanoseconds,
 * which is the only detail needed for support.
 *
 * Paper:
 * Write Off-Loading: Practical Power Management for Enterprise Storage
 * Dushyanth Narayanan, Austin Donnelly, and Antony Rowstron
 * Microsoft Research Ltd.
 * Proc. 6th USENIX Conference on File and Storage Technologies (FAST <92>08)
 * http://www.usenix.org/event/fast08/tech/narayanan.html
 */

static const block_t MSR_BLOCK_SIZE = 512;
// 100 nanosecond -> second -> minute -> hour
static const long long MSR_HOUR_LENGTH = 10000000L * 60 * 60;

struct msr_struct {
  oblock_t addr;
  bool write;
  block_t blocks_left;
  bool starting_time_set;
  uint64_t starting_time;
  uint64_t ending_time;
};

struct msr_struct msr_info = {
    .addr = 0,
    .write = false,
    .blocks_left = 0,
};

static int msr_trace_init(FILE *file, unsigned duration_hrs);
static int msr_trace_read(struct trace_reader_result *result);

struct trace_reader msr_trace = {
    .features = {0},
    .file = NULL,
    .init = msr_trace_init,
    .read = msr_trace_read,
};

static int msr_trace_init(FILE *file, unsigned duration_hrs) {
  msr_trace.file = file;

  if (duration_hrs > 0) {
    msr_trace.features.use_duration = true;
    msr_trace.features.duration_hrs = duration_hrs;
  } else {
    msr_trace.features.use_duration = false;
  }

  msr_info.starting_time_set = false;
  return 0;
}

static int msr_trace_read(struct trace_reader_result *result) {
  FILE *file = msr_trace.file;

  if (msr_info.blocks_left > 0) {
    msr_info.addr += MSR_BLOCK_SIZE;
    --msr_info.blocks_left;
  } else {
    block_t size = 0;
    block_t align;
    char io;
    uint64_t ts;

    while (size == 0) {
      if (fscanf(file, "%lu,%*[^,],%*[^,],%c%*[^,],%lu,%lu,%*s\n", &ts, &io,
                 &msr_info.addr, &size) != 4) {
        if (feof(file)) {
          LOG_DEBUG("end of file reached");
        } else {
          LOG_DEBUG("couldn't read file properly. error %d", ferror(file));
        }
        return 1;
      }
    }

    // align address
    align = msr_info.addr % MSR_BLOCK_SIZE;
    msr_info.addr -= align;
    size += align;

    msr_info.write = io == 'W';
    msr_info.blocks_left = (size / MSR_BLOCK_SIZE) - 1;
    if (size % MSR_BLOCK_SIZE != 0) {
      ++msr_info.blocks_left;
    }

    if (msr_trace.features.use_duration) {
      if (!msr_info.starting_time_set) {
        msr_info.starting_time = ts;
        msr_info.ending_time =
            ts + (MSR_HOUR_LENGTH * msr_trace.features.duration_hrs);
        msr_info.starting_time_set = true;
      }
      if (ts > msr_info.ending_time) {
        LOG_DEBUG("end of duration reached");
        return 1;
      }
    }
  }

  result->oblock = msr_info.addr;
  result->write = msr_info.write;

  return 0;
}

#endif
