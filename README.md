# Watchpoint 示例内核模块

编写一个在内核中注册 watchpoint 的内核模块，监视某个内存地址，一旦有内核线程对该内存进行写操作则进入 monitor 模式，执行注册的回调函数。

## 1. 注册 watchpoint 内核模块

watchpoint 机制对地址生效，但是本模块输入一个内核符号，再通过 __symbol_get() 获取该内核符号的内存地址，进而对地址进行监视。

在默认情况下，watchpoint 检测 trampolines 内核符号位于的内存地址。使用 modprobe ksym=<ksym_name> 可以指定要监视的内核符号。

```c
static char ksym_name[KSYM_NAME_LEN] = "trampolines";
module_param_string(ksym, ksym_name, KSYM_NAME_LEN, S_IRUGO);
```

watchpoint 内核模块初始化时首先定义一个 struct perf_event_attr 对象 attr。其中记录了受监视内存首地址 attr.bp_addr，受监视内存长度 attr.bp_len 受监视内存操作 attr.bp_type，以及受监视内存的屏蔽位 attr.bp_mask。struct perf_event_attr 对象并不一定是表示一个 breakpoint 或 watchpoint 事件，hw_breakpoint_init() 函数将其属性初始化为 breakpoint 事件。

```c
static int __init watchpoint_init(void)
{
	int ret;
	void *addr = __symbol_get(ksym_name);

    printk(KERN_INFO "ksym %s in %p", ksym_name, addr);
	if (!addr)
		return -ENXIO;

	hw_breakpoint_init(&attr);
	attr.bp_mask = 12;
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

## 5. 对内核的修改
在 arm64 架构下，并没有写入 wcr 控制寄存器的 mask 位，这导致 watchpoint 只能在 1-8 个字节的范围内生效。加入 mask 标志位：
```c
// arch/arm64/include/asm/hw_breakpoint.h
struct arch_hw_breakpoint_ctrl {
	u32 __reserved	: 19,
	mask            : 4,  // add
	len		        : 8,
	type	    	: 2,
	privilege	    : 2,
	enabled		    : 1;
};

static inline u32 encode_ctrl_reg(struct arch_hw_breakpoint_ctrl ctrl)
{
	// u32 val = (ctrl.len << 5) | (ctrl.type << 3) | 
	// 	(ctrl.privilege << 1) | ctrl.enabled;
	u32 val = (ctrl.mask << 24) | (ctrl.len << 5) | (ctrl.type << 3) | 
		(ctrl.privilege << 1) | ctrl.enabled;

	if (is_kernel_in_hyp_mode() && ctrl.privilege == AARCH64_BREAKPOINT_EL1)
		val |= DBG_HMC_HYP;

	return val;
}
```

根据 arm64 的架构参考手册，wcr 的 mask 标志位、 bas 标志位（即 attr->bp_len）和 wvr 的 va 位（即 attr->bp_mask）必须满足一定的关系，否则可能会产生未知的行为。所以在构造 arch_hw_breakpoint_ctrl 的值时检查：
```c
// arch/arm64/kernel/hw_breakpoint.c
	if (attr->bp_mask) {
		/*
		 * Mask
		 */
		if (attr->bp_mask < 3 || attr->bp_mask > 31) return -EINVAL;
		if (attr->bp_addr & ((1 << attr->bp_mask) - 1)) return -EINVAL;
		if (attr->bp_len != HW_BREAKPOINT_LEN_8) return -EINVAL;
		hw->ctrl.len = ARM_BREAKPOINT_LEN_8;
		hw->ctrl.mask = attr->bp_mask;
	}
```

## 实验记录
使用不同 mask 的 watchpoint 监视 trampolines 内核符号，此时 watchpoint 的首地址是 trampolines 数组的首地址，监视长度由 mask 决定。从 trampolines 的末端向首端遍历确认 watchpoint 监视的范围。

attr.bp_mask = 11 时，写 511 项（2^11 个字节）触发 watchpoint：
```
[    6.154637] 514th entry in trampolines 00000000ef6f0ac1
[    6.154804] 513th entry in trampolines 000000002b4fb64a
[    6.154980] 512th entry in trampolines 0000000014c4be8e
[    6.155152] 511th entry in trampolines 00000000f981f808
[    6.156205] trampolines value is changed
[    6.156394] Dump stack from watchpoint_handler
[    6.156826] CPU: 0 PID: 108 Comm: tl_writer Not tainted 5.10.0+ #3
[    6.157046] Hardware name: linux,dummy-virt (DT)
[    6.157459] Call trace:
[    6.158178]  dump_backtrace+0x0/0x1c0
[    6.158452]  show_stack+0x18/0x68
[    6.158616]  dump_stack+0xd4/0x130
[    6.159271]  watchpoint_handler+0x30/0x48 [watchpoint]
```
attr.bp_mask = 10 时，写 255 项（2^10 个字节）触发 watchpoint：
```
[    6.137330] 258th entry in trampolines 00000000b18bec2c
[    6.137511] 257th entry in trampolines 00000000e6b444da
[    6.137682] 256th entry in trampolines 000000006a693037
[    6.137868] 255th entry in trampolines 00000000278c1599
[    6.138854] trampolines value is changed
[    6.139017] Dump stack from watchpoint_handler
[    6.139463] CPU: 0 PID: 108 Comm: tl_writer Not tainted 5.10.0+ #3
[    6.139686] Hardware name: linux,dummy-virt (DT)
[    6.140097] Call trace:
[    6.140802]  dump_backtrace+0x0/0x1c0
[    6.141084]  show_stack+0x18/0x68
[    6.141255]  dump_stack+0xd4/0x130
[    6.141906]  watchpoint_handler+0x30/0x48 [watchpoint]
```
attr.bp_mask = 9 时，写 127 项（2^9 个字节）触发 watchpoint：
```
[    5.481447] 130th entry in trampolines 0000000051c5c5c7
[    5.481617] 129th entry in trampolines 00000000571d2b9a
[    5.481890] 128th entry in trampolines 000000009df087de
[    5.482076] 127th entry in trampolines 0000000050822d64
[    5.483028] trampolines value is changed
[    5.483203] Dump stack from watchpoint_handler
[    5.483642] CPU: 0 PID: 108 Comm: tl_writer Not tainted 5.10.0+ #3
[    5.483883] Hardware name: linux,dummy-virt (DT)
[    5.484286] Call trace:
[    5.484970]  dump_backtrace+0x0/0x1c0
[    5.485253]  show_stack+0x18/0x68
[    5.485404]  dump_stack+0xd4/0x130
[    5.486075]  watchpoint_handler+0x30/0x48 [watchpoint]
```
## 7. 问题

### 7.1 中断处理函数中不允许休眠
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

### 7.2 回调函数
另一个问题是一旦出现写 trampolines 的操作，就会一直调用回调函数 watchpoint_handler()。尚未确定原因。
