#ifndef INCLUDE_COMMON_H
#define INCLUDE_COMMON_H

#include "kernel/common.h"
#include "tools/logs.h"
#include "types.h"

#ifdef __KERNEL__
#include <linux/vmalloc.h>
#else
#include <stdlib.h>
#endif

static void *mem_alloc(size_t size) {
#ifdef __KERNEL__
  return vzalloc(size);
#else
  return calloc(1, size);
#endif
}

static void mem_free(void *ptr) {
#ifdef __KERNEL__
  vfree(ptr);
#else
  free(ptr);
#endif
}

#endif
