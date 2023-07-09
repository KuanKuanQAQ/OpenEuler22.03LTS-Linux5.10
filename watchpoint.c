#include <linux/module.h>	/* Needed by all modules */
#include <linux/kernel.h>	/* Needed for KERN_INFO */
#include <linux/init.h>		/* Needed for the macros */
#include <linux/kallsyms.h>
#include <linux/delay.h>
#include <linux/random.h>
#include <linux/kthread.h>

#include <linux/perf_event.h>
#include <linux/hw_breakpoint.h>

#define TRAMPOLINE_LEN 10
#define RANDPERIOD 2000

static struct task_struct *kthread = NULL;

struct perf_event * __percpu *wp;
struct perf_event_attr attr;

// static char ksym_name[KSYM_NAME_LEN] = "jiffies";
static char ksym_name[KSYM_NAME_LEN] = "trampolines";
module_param_string(ksym, ksym_name, KSYM_NAME_LEN, S_IRUGO);

static void watchpoint_handler(struct perf_event *bp,
			       struct perf_sample_data *data,
			       struct pt_regs *regs)
{
	printk(KERN_INFO "%s value is changed\n", ksym_name);
	printk(KERN_INFO "Dump stack from watchpoint_handler\n");
	dump_stack();
}

int work_func(void *trampoline)
{
	unsigned int random_num = 0;
	u64 *trampolines = (u64 *)trampoline;
	int cpu = get_cpu();
	int cnt = 3;
	ktime_t time = ktime_get();

	printk(KERN_INFO "Write in trampolines\n");
	do{
		msleep(RANDPERIOD);
		
		unregister_hw_breakpoint(per_cpu(*wp, cpu));
		random_num = prandom_u32_max(TRAMPOLINE_LEN);
		printk(KERN_INFO "%dth entry in trampolines %p\n", random_num, &trampolines[random_num]);
		trampolines[random_num] = 0xd61f0000;
		// random_num += 1;
		wp = register_wide_hw_breakpoint(&attr, watchpoint_handler, NULL);

	// }while(!kthread_should_stop());
	}while(--cnt);
	printk(KERN_INFO "Total time: %lld\n", ktime_get() - time);
	
	return 0;
}


static int __init watchpoint_init(void)
{
	int ret;
	void *addr = __symbol_get(ksym_name);

    printk(KERN_INFO "ksym %s in %p", ksym_name, addr);
	if (!addr)
		return -ENXIO;

	hw_breakpoint_init(&attr);
	attr.bp_addr = (unsigned long)addr;
	attr.bp_len = HW_BREAKPOINT_LEN_8;
	attr.bp_type = HW_BREAKPOINT_W;

	wp = register_wide_hw_breakpoint(&attr, watchpoint_handler, NULL);
	if (IS_ERR((void __force *)wp)) {
		ret = PTR_ERR((void __force *)wp);
        printk(KERN_INFO "Watchpoint registration failed\n");
        return ret;
	}

	printk(KERN_INFO "Watchpoint for %s write installed %p\n", ksym_name, addr);

	printk(KERN_INFO "Kernel thread start\n");
	/* Start worker kthread */
	kthread = kthread_run(work_func, (void *)addr, "tl_writer");
	if(kthread == ERR_PTR(-ENOMEM)){
		pr_err("Could not run kthread\n");
		return -1;
	}
	return 0;

}

static void __exit watchpoint_exit(void)
{
	unregister_wide_hw_breakpoint(wp);
	symbol_put(ksym_name);
	printk(KERN_INFO "Watchpoint for %s write uninstalled\n", ksym_name);
}

module_init(watchpoint_init);
module_exit(watchpoint_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Kuan");
MODULE_DESCRIPTION("Watchpoint");
