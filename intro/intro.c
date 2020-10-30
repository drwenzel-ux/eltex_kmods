#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>

MODULE_DESCRIPTION("Test kernel module");
MODULE_AUTHOR("Wenzel");
MODULE_LICENSE("GPL");

static int mod_init(void) {
  pr_info("init\n");
  return 0;
}

static void mod_exit(void) { pr_info("exit\n"); }

module_init(mod_init);
module_exit(mod_exit);