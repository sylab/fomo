#ifndef INCLUDE_KERNEL_COMMON_H
#define INCLUDE_KERNEL_COMMON_H

#ifndef __KERNEL__
#include <errno.h>
#include <stdlib.h>

#ifndef offsetof
#define offsetof(type, member) ((size_t)(&((type *)0)->member))
#endif

/**
 * container_of - cast a member of a structure out to the containing structure
 * @ptr:	the pointer to the member.
 * @type:	the type of the container struct this is embedded in.
 * @member:	the name of the member within the struct.
 *
 */
#define container_of(ptr, type, member)                                        \
  ({                                                                           \
    const typeof(((type *)0)->member) *__mptr = (ptr);                         \
    (type *)((char *)__mptr - offsetof(type, member));                         \
  })

static unsigned kstrtoul(const char *str, unsigned base, unsigned long *tmp) {
  errno = 0;
  *tmp = strtoul(str, NULL, base);
  return errno;
}

#undef max
static unsigned max(unsigned a, unsigned b) { return a > b ? a : b; }
#undef min
static unsigned min(unsigned a, unsigned b) { return a > b ? b : a; }
#endif /* !__KERNEL__ */

#endif /* INCLUDE_KERNEL_COMMON_H */
