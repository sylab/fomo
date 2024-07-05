#ifndef INCLUDE_TOOLS_LOGS_H
#define INCLUDE_TOOLS_LOGS_H

/** filename without directory
 */
#define __FILENAME__                                                           \
  (__builtin_strrchr(__FILE__, '/') ? __builtin_strrchr(__FILE__, '/') + 1     \
                                    : __FILE__)

#ifdef __KERNEL__
#define LOG_STDOUT KERN_INFO
#define LOG_DEBUG_OUT KERN_DEBUG
#define LOG_WARNING_OUT KERN_WARNING
#define LOG_FATAL_OUT KERN_EMERG
#define LOG_PRINT_F(file, fmt, args...)                                        \
  do {                                                                         \
    printk(file fmt, ##args);                                                  \
  } while (0)
#define LOG_ABORT_F()                                                          \
  do {                                                                         \
    BUG_ON(1);                                                                 \
  } while (0)
#else
#include <stdio.h>
#include <stdlib.h>
#define LOG_STDOUT stdout
#define LOG_DEBUG_OUT stdout
#define LOG_WARNING_OUT stderr
#define LOG_FATAL_OUT stderr
#define LOG_PRINT_F(file, fmt, args...)                                        \
  do {                                                                         \
    fprintf(file, fmt, ##args);                                                \
    fflush(file);                                                              \
  } while (0)
#define LOG_ABORT_F()                                                          \
  do {                                                                         \
    abort();                                                                   \
  } while (0)
#endif

// make macros and log levels
// levels: LOG(INFO)
//         LOG(DEBUG)
//         LOG(WARNING)
//         LOG(FATAL)

#define LOG_PRINT(fmt, args...)                                                \
  do {                                                                         \
    LOG_PRINT_F(LOG_STDOUT, fmt "\n", ##args);                                 \
  } while (0)

#define LOG(file, prefix, fmt, args...)                                        \
  do {                                                                         \
    LOG_PRINT_F(file, prefix " %s %d :: " fmt "\n", __FILENAME__, __LINE__,    \
                ##args);                                                       \
  } while (0)

/** LOG_INFO:
 * Print to stdout or KERN_INFO with format
 * INFO: example.c 15 :: Example message!
 */
#define LOG_INFO(fmt, args...) LOG(LOG_STDOUT, "INFO:", fmt, ##args)

/** LOG_DEBUG:
 * If DEBUG flag is defined, print to stdout or KERN_DEBUG with format
 * DEBUG: example.c 15 :: Example message!
 */
#ifdef DEBUG
#define LOG_DEBUG(fmt, args...) LOG(LOG_DEBUG_OUT, "DEBUG:", fmt, ##args)
#else
#define LOG_DEBUG(fmt, args...)                                                \
  do {                                                                         \
  } while (0)
#endif

/** LOG_WARNING:
 * Print to stderr or KERN_WARNING with format
 * WARNING: example.c 15 :: Example message!
 */
#define LOG_WARNING(fmt, args...) LOG(LOG_WARNING_OUT, "WARNING:", fmt, ##args)

/** LOG_FATA:
 * Print to stderr or KERN_EMERG with format
 * FATAL: example.c 15 :: Example message!
 * And abort/crash the program to prevent further issues
 */
#define LOG_FATAL(fmt, args...)                                                \
  do {                                                                         \
    LOG(LOG_FATAL_OUT, "FATAL:", fmt, ##args);                                 \
    LOG_ABORT_F();                                                             \
  } while (0)

/** LOG_ASSERT:
 * If expression fails, print to stderr or KERN_EMERG with format
 * FATAL: example.c 15 :: Assertion a > 5 failed.
 * And abort/crash the program to prevent further issues
 */
#define LOG_ASSERT(expr)                                                       \
  do {                                                                         \
    if (!(expr)) {                                                             \
      LOG_FATAL("Assertion %s failed.", #expr);                                \
    }                                                                          \
  } while (0)

#define LOG_PRINT_ONCE(fmt, args...)                                           \
  do {                                                                         \
    static int __counter__ = 0;                                                \
    if (__counter__ < 1) {                                                     \
      LOG_PRINT(fmt##args);                                                    \
      ++__counter__;                                                           \
    }                                                                          \
  } while (0)

#define LOG_ONCE(file, prefix, fmt, args...)                                   \
  do {                                                                         \
    static int __counter__ = 0;                                                \
    if (__counter__ < 1) {                                                     \
      LOG(file, prefix, fmt, ##args);                                          \
      ++__counter__;                                                           \
    }                                                                          \
  } while (0)

#define LOG_INFO_ONCE(fmt, args...) LOG_ONCE(LOG_STDOUT, "INFO:", fmt, ##args)

#ifdef DEBUG
#define LOG_DEBUG_ONCE(fmt, args...)                                           \
  LOG_ONCE(LOG_DEBUG_OUT, "DEBUG:", fmt, ##args)
#else
#define LOG_DEBUG_ONCE(fmt, args...)                                           \
  do {                                                                         \
  } while (0)
#endif

#define LOG_WARNING_ONCE(fmt, args...)                                         \
  LOG_ONCE(LOG_WARNING_OUT, "WARNING:", fmt, ##args)

#endif /* INCLUDE_TOOLS_LOGS_H */
