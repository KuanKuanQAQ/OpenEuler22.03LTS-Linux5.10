# Watchpoint 示例内核模块

编写一个在内核中注册 watchpoint 的内核模块，监视某个内存地址，一旦有内核线程对该内存进行写操作则进入 monitor 模式，执行注册的回调函数。

## 1. 注册 watchpoint 内核模块

watchpoint 机制对地址生效，但是本模块输入一个内核符号，再通过 __symbol_get() 获取该内核符号的内存地址，进而对地址进行监视。

在默认情况下，watchpoint 检测 trampolines 内核符号位于的内存地址。使用 modprobe ksym=<ksym_name> 可以指定要监视的内核符号。

```c
static char ksym_name[KSYM_NAME_LEN] = "trampolines";
module_param_string(ksym, ksym_name, KSYM_NAME_LEN, S_IRUGO);
```

watchpoint 内核模块初始化时首先定义一个 struct perf_event_attr 对象 attr。其中记载了受监视内存首地址 attr.bp_addr，受监视内存长度 attr.bp_len 以及受监视内存操作 attr.bp_type。struct perf_event_attr 对象并不一定是表示一个 breakpoint 或 watchpoint 事件，hw_breakpoint_init() 函数将其属性初始化为 breakpoint 事件。

```c
static int __init watchpoint_init(void)
{
	int ret;
	struct perf_event_attr attr;
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

	return 0;

}
```

注册 watchpoint 内核模块的核心是 register_wide_hw_breakpoint() 函数。开启 CONFIG_HAVE_HW_BREAKPOINT 的情况下，在  <linux/hw_breakpoint.h> 头文件中声明了，register_wide_hw_breakpoint_cpu()，register_wide_hw_breakpoint()，register_perf_hw_breakpoint() 三个用于注册 watchpoint 的函数。其中第一个函数只有声明没有定义。第二个函数是在所有 cpu 上注册监视该内存地址，本内核模块采用第二个函数。

## 2. 定义回调函数

回调函数 watchpoint_handler 在 register_wide_hw_breakpoint() 被注册到异常处理程序中。用于产生 watchpoint 异常时调用，本模块的回调函数定义如下：

```c
static void watchpoint_handler(struct perf_event *bp,
			       struct perf_sample_data *data,
			       struct pt_regs *regs)
{
	printk(KERN_INFO "%s value is changed\n", ksym_name);
	printk(KERN_INFO "Dump stack from watchpoint_handler\n");
	dump_stack();
}
```

其中 dump_stack() 函数用于打印异常发生时的函数调用栈。

 ## 3. 启动内核线程修改受监视的内存区域

本实验中被修改的区域为 trampolines，这是一个 u64 数组。watchpoint 禁止对其的任何写操作。

在 watchpoint.ko 中启动一个内核线程，定期修改这块内存：

```c
int work_func(void *trampoline)
{
	unsigned int random_num = 0;
	u64 *trampolines = (u64 *)trampoline;
	int cpu = get_cpu();
	printk(KERN_INFO "Write in trampolines\n");
	do{
		msleep(RANDPERIOD);
		
		unregister_hw_breakpoint(per_cpu(*wp, cpu));
		random_num = prandom_u32_max(TRAMPOLINE_LEN);
		printk(KERN_INFO "%dth entry in trampolines %p\n", random_num, &trampolines[random_num]);
		trampolines[random_num] = 0xd61f0000;
		// random_num += 1;
		wp = register_wide_hw_breakpoint(&attr, watchpoint_handler, NULL);

	}while(!kthread_should_stop());

	return 0;
}
```

注意，在修改前后分别使用 unregister_hw_breakpoint() 和 register_wide_hw_breakpoint() 暂时关闭线程所在 cpu 对于 trampolines 的 watchpoint，在修改后重新打开。

此处的修改是随机进行的。也可以改成其他修改方式。

内核线程启动时告知 trampolines 符号的地址：

```c
	printk(KERN_INFO "Kernel thread start\n");
	/* Start worker kthread */
	kthread = kthread_run(work_func, (void *)addr, "tl_writer");
	if(kthread == ERR_PTR(-ENOMEM)){
		pr_err("Could not run kthread\n");
		return -1;
	}
```

## 4. 定义受监视内核符号

在 linux 内核中，可以将一个符号用 EXPORT_SYMBOL 宏导出为内核符号。此处将数组 trampolines 导出：

```c
u64 trampolines[TRAMPOLINE_LEN];
EXPORT_SYMBOL_GPL(trampolines);
```

这个内核符号必须在另一个内核模块中导出。如果在 watchpoint.ko 中定义并导出这个符号，watchpoint.ko 自己找不到这个符号。

并且这个内核模块必须先于 watchpoint.ko 加载。否则 watchpoint.ko 还是找不到 trampolines 这个符号。

## 5. 问题

第 3 节中创建的内核线程在两次写 trampolines 之间会睡眠一定时间，这引起了内核错误（删除 msleep 睡眠就不会报错）：

```
[    3.633378] BUG: scheduling while atomic: tl_writer/108/0x00000002
[    3.633646] Modules linked in: watchpoint trampolines
[    3.634488] CPU: 0 PID: 108 Comm: tl_writer Not tainted 5.10.0 #8
[    3.634740] Hardware name: linux,dummy-virt (DT)
[    3.635201] Call trace:
[    3.635979]  dump_backtrace+0x0/0x1c0
[    3.636279]  show_stack+0x18/0x68
[    3.636455]  dump_stack+0xd4/0x130
[    3.636593]  __schedule_bug+0x54/0x78
[    3.636739]  __schedule+0x618/0x650
[    3.636883]  schedule+0x70/0x108
[    3.637037]  schedule_timeout+0x188/0x288
[    3.637195]  msleep+0x2c/0x40
[    3.637890]  work_func+0x88/0x118 [watchpoint]
[    3.638063]  kthread+0x158/0x160
[    3.638191]  ret_from_fork+0x10/0x30
```

这是由于在中断处理函数中调用了休眠函数。而 linux 内核要求在中断处理的时候，不允许系统调度，不允许抢占，要等到中断处理完成才能做其他事情。因此，要充分考虑中断处理的时间，一定不能太久。

但是这个内核线程为什么会被认为是中断处理函数呢？

另一个问题是一旦出现写 trampolines 的操作，就会一直调用回调函数 watchpoint_handler()。尚未确定原因。
