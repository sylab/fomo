# FOMO

Conventional caches are datapath caches due to each accessed item requiring that it must first reside in the cache before being consumed by the application. Non-datapath caches, being free of this requirement, demand entirely new techniques. Current non-datapath caches typically represented by flash- or NVM- based SSDs additionally have limited number of write cycles, motivating cache management strategies that minimize cache updates.
FOMO is a non-datapath cache admission policy that operates in two states: *insert* and *filter*, allowing cache insertions in the former state, and only selectively enabling insertions in the latter. Using two simple and inexpensive metrics, the *cache hit rate* and the *reuse rate of missed items*, FOMO makes judicious decisions about cache admission dynamically.


## Quick Start

 1. Run `make` from root directory of the project to compile `cache-sim`.
 2. Run `cache-sim` with `./cache-sim lru 10 basic -f example.trace`.


## NOTE

This repository contains work for a project called `cache nucleus` that has since not received updates.
It contains several different algorithms including __`FOMO`__ and whose simulator (`cache-sim`) was used to test algorithms against a series of traces.

The `dm-cache-policy` part of this project may not work and there are currently has no intentions to support this beyond the scope presented.

Below is the details for that project.


# cache nucleus

The idea of `cache nucleus` is to take knowledge from how `dm-cache-policy` works and applying that to help generalize how we write caching algorithms so that we can write a caching algorithm once and use said caching algorithm, unmodified, to build and run both simulations and kernel modules (such as `dm-cache`).

So far, the kernel module side of the code has been focused on the 3.13 Linux kernel.

---

## How to get kernel source code on Ubuntu

The following command with download and extract the Linux source code for your current kernel version: 

`apt-get source linux-image-$(uname -r)`

The Makefiles assume that the extracted source code in under the directory *linux*, so it is recommended to rename the extracted directory to *linux* (e.g. `mv linux-3.13.0 linux`).

---

## Algorithms

The algorithms within this project are either direct caching algorithms or wrappers that utilize the direct caching algorithms to manage the cache.

The list of supported algorithms are as follows:
 - FOMO (a wrapper around different algorithms)
 - m\* (a wrapper around different algorithms)
 - LRU
 - ARC
 - LARC
 - LIRS
 - mARC

The algorithms are identified with lower-case names for `cache-sim` and `dmcache-policy`, with the direct caching algorithm names would be: lru`/`arc`/`larc`/`lirs`/`marc`.
The wrapper algorithms require a prefix (`fomo\_`/`mstar\_`) which is then followed by the direct caching algorithm (e.g. `fomo\_arc` or `mstar\_lru`).

---

## Compiling

Compiling after all preparations is pretty simple: run `make` ___from the project's root folder!___

___NOTE:___ By default the compilation of the `dm-cache-policy` part of the project is disabled to aid in the simplification of compiling the `cache-nucleus` simulator (`cache-sim`) for testing. If you intend on compiling the `dm-cache-policy` part of the project, please refer to the __dmcache-policy__ section.

From there, scripts will generate the necessary algorithm structures for the different types of applications. Afterwards, the kernel-specific application(s) will try to compile (this is to enforce kernel-specific rules by halting compilation should any code not adhere to them), followed by the userspace application(s). All applications should then be found here in the root directory.

---

## cache-sim

```
Usage: ./cache-sim [ALGORITHM] [CACHE_SIZE] [TRACE_FORMAT] [OPTION]...
Simulate a cache of size CACHE_SIZE, running ALGORITHM over a TRACE_FORMAT.

  ALGORITHM        caching algorithm (such as lru)
  CACHE_SIZE       size of the cache in entries
  TRACE_FORMAT     format of the trace being processed

With no -f or --file OPTION, read standard input.

  -f, --file       file of TRACE_TYPE to open and simulate for
  -d, --duration   amount of the trace, based on time, that
                   is going to be processed
                   Supported time designations:
                     XXh   XX hours
                     XXd   XX days
  -m, --metadata-size
                   set the size of the metadata for the algorithm
                   should the algorithm support it
      --help       display this help and exit

Examples:
  ./cache-sim lru 10 basic
      Run lru cache (of size 10 entries) with standard input in
      basic trace format
  ./cache-sim lru 10 basic -f example.trace
      Run lru cache (of size 10 entries) with example.trace (which
      is a basic trace format)
```

---

## dmcache-policy

To compile the `dm-cache-policy`, beyond the setup of the linux kernel drivers, set the `COMPILE_DMCACHE` to `1` in the root `Makefile` and call `make`.

With `dmcache-policy` requiring changes to the Linux `dm-cache-target` and `dm-cache-policy`, we must compile these into `dm-cache` and insert our version to the kernel in order to have `dmcache-policy` work appropriately.

If you find yourself having issues with getting this part of the project to run, refer to `debug.txt` where one of the maintainers detailed the steps they took.

### Insertion/Removal From the Kernel

As `dmcache-policy` depends on `dm-cache`, it is advised to insert the `dm_cache` kernel module found in Linux prior to inserting `dmcache-policy` in order to load the required dependencies that our `dm-cache` version requires:
`modprobe -i dm-cache`
`rmmod dm_cache`
`insmod ./dm-cache.ko`

With `dm_cache` inserted into the kernel, we can now add `dmcache-policy`:
`insmod ./dmcache-policy.ko`

To remove `dmcache-policy`:
`rmmod dmcache_policy`

### Creating a dm-cache using an algorithm from dmcache-policy

With `dmcache-policy` inserted in the kernel, creating a dm-cache to run a policy from `dmcache-policy` only requires a few things:

- A block device for cache entry data
- A block device for cache entry metadata
- A block device that is the "origin" device for data

With each, the command to create the dm-cache would be:
`dmsetup create target-cache --table "0 [SIZE] cache [CACHE META DEVICE] [CACHE DATA DEVICE] [ORIGIN_DEVICE] 64 1 writeback [ALGORITHM] 0"`

For more details refer to your Linux kernel's [dm-cache documentation](https://elixir.bootlin.com/linux/v3.13.11/source/Documentation/device-mapper/cache.txt).

----

## How to write a policy for Cache Nucleus

The [lru\_policy](master/src/algs/lru) is a pretty simple template to build off of when writing a new policy.

### Creating example\_policy

1. Create a directory `example/` in `src/algs/`
2. Create `src/algs/example/example_policy.c`
  1. Write `struct example_policy` with `struct base_policy` in it
  2. Add functions required for `base_policy` to interact with `example_policy`
3. Create `src/algs/example/example_policy.h`
  1. Declare `example_create` function
     - Done for scripts, which assume that `example` directory contains an `example_policy.h` file with the function `example_create`
4. Add `example/example_policy.o` to `src/algs/Makefile`
5. Add `example/example_policy.o` to `libalgs.a` in `src/algs/Makefile` for static library linking
6. Add `example/example_policy.o` to `obj-y` in `src/algs/Makefile` for kernel compilation
  1. More complex algorithms could require additional object files to be included
7. On compilation, `example_policy` is now accessable in all applications under the name "example"

--- 

## Adding a new trace format

1. Write a new `trace_reader` header file in `src/trace_reader/`
  1. For example `example_trace.h`
  2. Write `struct trace_reader example_trace` along with appropriate `example_trace_init()` and `example_trace_read()` functions to point to
2. Add `#include "trace_reader/example_trace.h"` to `src/trace_reader/trace_reader.h`
3. Add `if (__trace_reader_names_match(name, "example")) { return &example_trace; }` to `find_trace_reader()`
4. On compilation, `example_trace` is now accessable in all applications that use the `trace_reader` under the case-insensitive name of "example"

---

## Working set size tool

When testing a cache size as a percentage of the total working set size of a trace, we used a tool program called `set-size`.

```
Usage: ./set-size [TRACE_FORMAT] [OPTION]...
Find the size of the trace's working set.
  TRACE_FORMAT     format of the trace being processed

With no -f or --file OPTION, read standard input.

  -f, --file       file of TRACE_TYPE to open and simulate for
  -d, --duration   amount of the trace, based on time, that
                   is going to be processed
                   Supported time designations:
                     XXh   XX hours
                     XXd   XX days
      --help       display this help and exit

Examples:
  ./set-size fiu
      Get working set size for FIU trace pass with standard input
  ./set-size msr -f example.trace
      Get working set size for MSR trace example.trace
```

