#include "common.h"
#include "dmcache_policy/dmcache_policy_registry.h"
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>

// This part of the kernel module is very bare-bones as policy_registry_init()
// and policy_registry_exit() are generated in dmcache_policy_registry.h

static int __init dmcache_policy_init(void) { return policy_registry_init(); }

static void __exit dmcache_policy_exit(void) { policy_registry_exit(); }

module_init(dmcache_policy_init);
module_exit(dmcache_policy_exit);

MODULE_AUTHOR("Steven Lyons <slyon001@fiu.edu>");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("dmcache policy using base_policy");
