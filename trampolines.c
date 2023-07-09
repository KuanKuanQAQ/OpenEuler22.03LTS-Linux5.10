#include <linux/module.h>	/* Needed by all modules */
#include <linux/kernel.h>	/* Needed for KERN_INFO */
#include <linux/init.h>		/* Needed for the macros */
#include <linux/kallsyms.h>

#define TRAMPOLINE_LEN 10
u64 trampolines[TRAMPOLINE_LEN];
EXPORT_SYMBOL_GPL(trampolines);

static int __init trampolines_init(void)
{
	
	printk(KERN_INFO "Trampolines mod loaded\n");
	return 0;

}

static void __exit trampolines_exit(void)
{
	printk(KERN_INFO "Trampolines mod unloaded\n");
}

module_init(trampolines_init);
module_exit(trampolines_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Kuan");
MODULE_DESCRIPTION("Trampolines");
