#include <linux/module.h>	/* Needed by all modules */
#include <linux/kernel.h>	/* Needed for KERN_INFO */
#include <linux/init.h>		/* Needed for the macros */
#include <linux/gfp.h>
#include <linux/kallsyms.h>

#define TRAMPOLINE_LEN 1024
u32 trampolines[TRAMPOLINE_LEN] __aligned(PAGE_SIZE);
EXPORT_SYMBOL_GPL(trampolines);

static int __init trampolines_init(void)
{
	printk(KERN_INFO "Trampolines mod loaded\n");
	printk(KERN_INFO "sizeof trampolines: %ld\n", sizeof(trampolines));
	printk(KERN_INFO "trampolines addr1: %p\n", trampolines);
	printk(KERN_INFO "trampolines addr2: %p\n", &trampolines);
	return 0;

}

static void __exit trampolines_exit(void)
{
	free_page((unsigned long)trampolines);
	printk(KERN_INFO "Trampolines mod unloaded\n");
}

module_init(trampolines_init);
module_exit(trampolines_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Kuan");
MODULE_DESCRIPTION("Trampolines");
