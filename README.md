## 1 Watchpoint 示例内核模块

编写一个在内核中注册 watchpoint 的内核模块，监视某个内存地址，一旦有内核线程对该内存进行写操作则进入 monitor 模式，执行注册的回调函数。

### 1.1 注册 watchpoint 内核模块

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

### 1.2 定义回调函数

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

### 1.3 启动内核线程修改受监视的内存区域

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

### 1.4 定义受监视内核符号

在 linux 内核中，可以将一个符号用 EXPORT_SYMBOL 宏导出为内核符号。此处将数组 trampolines 导出：

```c
u64 trampolines[TRAMPOLINE_LEN];
EXPORT_SYMBOL_GPL(trampolines);
```

这个内核符号必须在另一个内核模块中导出。如果在 watchpoint.ko 中定义并导出这个符号，watchpoint.ko 自己找不到这个符号。

并且这个内核模块必须先于 watchpoint.ko 加载。否则 watchpoint.ko 还是找不到 trampolines 这个符号。

### 1.5 对内核的修改

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

### 1.6 实验记录
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
### 1.7 问题

#### 1.7.1 中断处理函数中不允许休眠
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

#### 1.7.2 回调函数
另一个问题是一旦出现写 trampolines 的操作，就会一直调用回调函数 watchpoint_handler()。尚未确定原因。


## 2 生成 trampolines

trampolines 是可重随机函数的固定入口。其不被重随机，从而为内核调用或模块内调用提供接口。

### 2.1 内核编译选项设置

ARM64_MODULE_RERANDOMIZE 用于开启内核模块的重随机。

```c
# Reasons for dependencies
# KALLSYMS_ALL - For preserving symbol table
config ARM64_MODULE_RERANDOMIZE
	bool
	prompt "Enable ARM64 modules rerandomization"
	# depends on KALLSYMS_ALL && RANDOMIZE_BASE
	default y
	help
	  Allow runtime rerandomization of modules.
```

ARM64_PTR_AUTH 用于开启 ARMv8.3 的 PAC 特性，这是一种校验返回地址的安全机制。随机化可能会与之冲突。make menuconfig 中路径为 Kernel Features  --->  ARMv8.3 architectural features  --->  Enable support for pointer authentication。

```c
menu "ARMv8.3 architectural features"

config ARM64_PTR_AUTH
	bool "Enable support for pointer authentication"
	default y
	depends on (CC_HAS_SIGN_RETURN_ADDRESS || CC_HAS_BRANCH_PROT_PAC_RET) && AS_HAS_PAC
	# Modern compilers insert a .note.gnu.property section note for PAC
	# which is only understood by binutils starting with version 2.33.1.
	depends on LD_IS_LLD || LD_VERSION >= 233010000 || (CC_IS_GCC && GCC_VERSION < 90100)
	depends on !CC_IS_CLANG || AS_HAS_CFI_NEGATE_RA_STATE
	depends on (!FUNCTION_GRAPH_TRACER || DYNAMIC_FTRACE_WITH_REGS)
```

在 ARM64_PTR_AUTH 开启时，压栈保存时用 `paciasp`，基于 APIAKey 和 SP 对 LR 进行签名；出栈使用时用 `autiasp`，基于 APIAKey 和 SP 对 LR 进行校验。其中 APIAKey 是 ARM64 硬件提供的 5 个密钥之一。

```
0000000000000000 <random_function>:
   0:	d503233f 	paciasp
   4:	d10083ff 	sub	sp, sp, #0x20
   8:	a9017bfd 	stp	x29, x30, [sp, #16]
   c:	910043fd 	add	x29, sp, #0x10
  10:	94000000 	bl	0 <random_function>
  14:	b81fc3a0 	stur	w0, [x29, #-4]
  18:	a9417bfd 	ldp	x29, x30, [sp, #16]
  1c:	910083ff 	add	sp, sp, #0x20
  20:	d50323bf 	autiasp
  24:	d65f03c0 	ret
```

关闭后则取消这两条指令：

```
0000000000000000 <random_function>:
   0:	d10083ff 	sub	sp, sp, #0x20
   4:	a9017bfd 	stp	x29, x30, [sp, #16]
   8:	910043fd 	add	x29, sp, #0x10
   c:	94000000 	bl	0 <random_function>
  10:	b81fc3a0 	stur	w0, [x29, #-4]
  14:	a9417bfd 	ldp	x29, x30, [sp, #16]
  18:	910083ff 	add	sp, sp, #0x20
  1c:	d65f03c0 	ret
```

### 2.2 trampolines 表项内容

trampoline 表项内容目前包含：

1. 构造栈空间，并保存 lr 寄存器的值。

2. 在栈中保存函数调用时通过寄存器 x0 到 x7 传入的参数。

3. 使用 bl 指令跳转到 add_pause_list() 函数，获取当前线程 pid，用于重随机时暂停内核线程。

4. 恢复寄存器 x0 至 x7 参数。

5. 使用 bl 指令跳转到目的函数。

6. 在栈中保存目的函数的返回值 x0。

7. 使用 bl 指令跳转到 remove_pause_list() 函数，将当前线程从暂停列表中移除，表示重随机时不再需要暂停该线程。

8. 将目的函数的返回值恢复到 x0 寄存器中。

9. 栈平衡并恢复 lr 寄存器。

函数参数与返回值由寄存器传递。

```
Disassembly of section .trampolines.text.random_function:

0000000000000000 <random_function>:
   0:	d10143ff 	sub	sp, sp, #0x50           ;开辟栈空间
   4:	a9047bfd 	stp	x29, x30, [sp, #64]     ;保存fp和lr寄存器
   8:	a90307e0 	stp	x0, x1, [sp, #48]       ;保存x0-x7参数寄存器
   c:	a9020fe2 	stp	x2, x3, [sp, #32]
  10:	a90117e4 	stp	x4, x5, [sp, #16]
  14:	a9001fe6 	stp	x6, x7, [sp]
  18:	910003fd 	mov	x29, sp                 ;将fp更新为sp
  1c:	94000000 	bl	0 <add_pause_list>      ;跳转至add_pause_list函数
  20:	a94307e0 	ldp	x0, x1, [sp, #48]       ;返回后恢复x0-x7参数寄存器
  24:	a9420fe2 	ldp	x2, x3, [sp, #32]
  28:	a94117e4 	ldp	x4, x5, [sp, #16]
  2c:	a9401fe6 	ldp	x6, x7, [sp]
  30:	94000000 	bl	0 <random_function>     ;跳转至random_function
  34:	f9001fe0 	str	x0, [sp, #56]           ;保存返回值寄存器x0
  38:	94000000 	bl	0 <remove_pause_list>   ;跳转至remove_pause_list函数
  3c:	a9447bfd 	ldp	x29, x30, [sp, #64]     ;读取最开始保存的fp和lr寄存器
  40:	f9401fe0 	ldr	x0, [sp, #56]           ;读取x0寄存器，恢复返回值
  44:	910143ff 	add	sp, sp, #0x50           ;恢复sp寄存器，完成栈平衡
  48:	d65f03c0 	ret
```

### 2.3 重定位相关信息

bl 指令跳转目的地址在重随机时由重定位表项计算。每个 trampoline 表项拥有自己的重定位表，内容为：

```
RELOCATION RECORDS FOR [.trampolines.text.random_function]:
OFFSET           TYPE              VALUE 
000000000000001c R_AARCH64_CALL26  add_pause_list
0000000000000030 R_AARCH64_CALL26  random_function_real
0000000000000038 R_AARCH64_CALL26  remove_pause_list
```

其中记载了 bl 指令在 trampoline 中的偏移 offset，重定位类型 type 以及重定位目的地址的符号 value。在编译时修改符号表内容，将原函数的符号定义位置改为 trampoline 表项位置，并新增可重随机函数的符号定义位置，内容为：

```
SYMBOL TABLE:
OFFSET					 TYPE SECTION                           SIZE             VIS     NAME
0000000000000000 F    .trampoline.text.random_function	0000000000000020         random_function
0000000000000000 F    .text.random_function_real	      0000000000000034 .hidden random_function_real
```

### 2.4 函数节

基于 -ffunction-sections 这一编译选项进行编译，每个函数的代码段均单独为一个 section。在此基础上，为每个函数节增加一个 trampoline 节。内核范围内所有对本函数的调用均指向本函数的 trampoline，再由 trampoline 跳转到函数的原代码段。

以 random_function 函数为例。由于符号表的修改，所有对 random_function 的调用均指向 trampoline 节 .trampolines.text.random_function。该节内的 bl 指令目的符号为 random_funciton_real，为重定位符号。在重随机过程中，根据重定表和符号表中的内容更新目的地址。

**模块补丁实现**

原函数：

```c
SPECIAL_FUNCTION(void, random_function, void) {
// void random_function(void) {
    printk("%s %d random function addr:%lx\n", __FUNCTION__, __LINE__, &random_function);
}
```

宏定义：

```c
#define _ASM_BL(f)		"bl " #f

#define SPECIAL_FUNCTION_PROTO(ret, name, args...)  \
	noinline ret __attribute__ ((section (".trampoline.text." #name))) __attribute__((naked)) name(args)
#define SPECIAL_FUNCTION(ret, name, args...) \
_Pragma("GCC diagnostic push") \
_Pragma("GCC diagnostic ignored \"-Wreturn-type\"") \
_Pragma("GCC diagnostic ignored \"-Wattributes\"") \
ret __attribute__ ((visibility("hidden"))) name## _ ##real(args);\
SPECIAL_FUNCTION_PROTO(ret, name, args) {              \
	asm ("sub sp, sp, #80");                           \
	asm ("stp x29, x30, [sp, #64]");                   \
	asm ("stp x0, x1, [sp, #48]");                     \
	asm ("stp x2, x3, [sp, #32]");                     \
	asm ("stp x4, x5, [sp, #16]");                     \
	asm ("stp x6, x7, [sp]");                          \
	asm ("mov x29, sp");                               \
	asm (_ASM_BL(add_pause_list));                     \
	asm ("ldp x0, x1, [sp, #48]");                     \
	asm ("ldp x2, x3, [sp, #32]");                     \
	asm ("ldp x4, x5, [sp, #16]");                     \
	asm ("ldp x6, x7, [sp]");                          \
	asm (_ASM_BL(name## _ ##real));                    \
	asm ("str x0, [sp, #56]");                         \
	asm (_ASM_BL(remove_pause_list));                  \
	asm ("ldp x29, x30, [sp, #64]");                   \
	asm ("ldr x0, [sp, #56]");                         \
	asm ("add sp, sp, #80");                           \
} \
_Pragma("GCC diagnostic pop") \
ret name## _ ##real(args)
```

## 3 测试内核模块

不直接在 bfq.ko 进行测试有三个原因：

1. bfq 函数众多，不便手动加宏。
2. 不容易验证 bfq 模块正常运行。
3. bfq 中不一定包含了所有待测试情景。

因此我使用了之前测试代码引用编写过的内核模块 test_dirver，该内核模块放在了 block/test 中。

这一内核模块的 init 部分如下：

```c
/* --------- block/test/test_driver.c:135 ---------*/
struct Rerandom_Driver rerandom_driver_struct = {
};

extern void register_rerandom_driver(struct Rerandom_Driver* driver);
static int __init random_test_driver_init(void) {
    rerandom_driver_struct.name = "rerandom_test";
    rerandom_driver_struct.init_entry = &init_entry;
    rerandom_driver_struct.check_entry = &check_entry;

    register_rerandom_driver(&rerandom_driver_struct);
    return 0;
}
module_init(random_test_driver_init);
```

其中 `struct Rerandom_Driver` 是一个**新增的**定义在内核中的结构体，在 test_driver 的初始化部分被实例化为一个“局部变量”（虽然是该文件中的全局变量，但其实是弱符号，把它的定义写进 init 函数成为真正的局部变量也是一样的）。这一结构体有如下定义：

```c
/* --------- include/linux/pci.h:46 ---------*/
struct Rerandom_Driver {
	const char		   *name;
	void (*init_entry)(void);
	int (*check_entry)(int, char*, time64_t);
};
```

可以看到 `struct Rerandom_Driver` 中包含了两个函数指针 init_entry 和 check_entry。这两个函数指针在 test_driver 的初始化函数中被初始化了。接着，random_test_driver_init 调用了一个外部函数 register_rerandom_driver，这个外部函数负责使用传入参数为全局变量 rerandom_driver 赋值。

```c
/* --------- mm/vmscan:4336 ---------*/
struct Rerandom_Driver rerandom_driver = {
	.name = default_name,
	.init_entry = empty_func,
	.check_entry = empty_check_func,
};

void register_rerandom_driver(const struct Rerandom_Driver *rerandom_driver_struct) {
	rerandom_driver.name        = rerandom_driver_struct->name;
	rerandom_driver.init_entry  = rerandom_driver_struct->init_entry;
	rerandom_driver.check_entry = rerandom_driver_struct->check_entry;
}
EXPORT_SYMBOL_GPL(register_rerandom_driver);
```

这部分代码模拟了驱动模块向内核结构体挂载接口的过程。挂载完成后，我使用了一个内核线程不断运行这些挂在的函数：

```c
/* --------- mm/vmscan:4349 ---------*/
static int my_kthread(void *p)
{
	time64_t time = ktime_get_seconds();
	int test_ret = -1;
	printk("My kthread: kthread started.");
	for ( ; ; ) {
		msleep(1000);
		/* Periodically print the statistics */
		if (time < ktime_get_seconds()) {
			time = ktime_get_seconds() + 5;
			printk("\n************************");
			printk("**Jump into test func.**");
			printk("************************");
			rerandom_driver.init_entry();
			test_ret = rerandom_driver.check_entry(11, "chifan", time);
		    printk("test_ret = %d\n", test_ret);
		    printk("Out Function init_entry Address: %px\n", rerandom_driver.init_entry);
		    printk("Out Function check_entry Address: %px\n", rerandom_driver.check_entry);
		}
	}
	return 0;
}
```

这个内核线程会随 vmscan 启动的 kswapd 线程一起启动，可以认为它始终在内核中运行。挂载之前，rerandom_driver 结构体中的函数指针指向两个默认函数：

```c
char default_name[] = "default_name";
void empty_func(void) {
	return;
}
int empty_check_func(int a, char* c, time64_t time) {
	printk("This is an empty func.");
	return 0;
}
```

初始化了 test_driver 内核模块后，这两个函数就会被更新为 test_driver.c 中定义的函数。