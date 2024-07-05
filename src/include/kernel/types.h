#ifndef KERNEL_TYPES_H
#define KERNEL_TYPES_H

#ifdef __KERNEL__
#include "dm-cache-policy.h"
#include <linux/types.h>
#else /* !__KERNEL__ */
#include <stdbool.h>
#include <stdint.h>

typedef uint64_t dm_block_t;
typedef dm_block_t dm_oblock_t;
typedef uint32_t dm_cblock_t;

typedef uint32_t u32;
typedef uint64_t u64;
typedef u64 sector_t;

static dm_oblock_t to_oblock(dm_block_t val) { return (dm_oblock_t)val; }

static dm_block_t from_oblock(dm_oblock_t val) { return (dm_block_t)val; }

static dm_cblock_t to_cblock(uint32_t val) { return (dm_cblock_t)val; }

static uint32_t from_cblock(dm_cblock_t val) { return (uint32_t)val; }
#endif /* !__KERNEL__ */

#endif /* KERNEL_TYPES_H */
