// SPDX-License-Identifier: GPL-2.0
/*
 * arch/sw_64/kernel/traps.c
 *
 * (C) Copyright 1994 Linus Torvalds
 */

/*
 * This file initializes the trap entry points
 */

#include <linux/extable.h>
#include <linux/perf_event.h>
#include <linux/kdebug.h>
#include <linux/kexec.h>

#include <asm/gentrap.h>
#include <asm/mmu_context.h>
#include <asm/fpu.h>
#include <asm/kprobes.h>
#include <asm/uprobes.h>

#include "proto.h"

void
dik_show_regs(struct pt_regs *regs, unsigned long *r9_15)
{
	printk("pc = [<%016lx>]  ra = [<%016lx>]  ps = %04lx    %s\n",
	       regs->pc, regs->r26, regs->ps, print_tainted());
	printk("pc is at %pSR\n", (void *)regs->pc);
	printk("ra is at %pSR\n", (void *)regs->r26);
	printk("v0 = %016lx  t0 = %016lx  t1 = %016lx\n",
	       regs->r0, regs->r1, regs->r2);
	printk("t2 = %016lx  t3 = %016lx  t4 = %016lx\n",
	       regs->r3, regs->r4, regs->r5);
	printk("t5 = %016lx  t6 = %016lx  t7 = %016lx\n",
	       regs->r6, regs->r7, regs->r8);

	if (r9_15) {
		printk("s0 = %016lx  s1 = %016lx  s2 = %016lx\n",
		       r9_15[9], r9_15[10], r9_15[11]);
		printk("s3 = %016lx  s4 = %016lx  s5 = %016lx\n",
		       r9_15[12], r9_15[13], r9_15[14]);
		printk("s6 = %016lx\n", r9_15[15]);
	}

	printk("a0 = %016lx  a1 = %016lx  a2 = %016lx\n",
	       regs->r16, regs->r17, regs->r18);
	printk("a3 = %016lx  a4 = %016lx  a5 = %016lx\n",
	       regs->r19, regs->r20, regs->r21);
	printk("t8 = %016lx  t9 = %016lx  t10 = %016lx\n",
	       regs->r22, regs->r23, regs->r24);
	printk("t11= %016lx  pv = %016lx  at = %016lx\n",
	       regs->r25, regs->r27, regs->r28);
	printk("gp = %016lx  sp = %p\n", regs->gp, regs+1);
}

static void
dik_show_code(unsigned int *pc)
{
	long i;
	unsigned int insn;

	printk("Code:");
	for (i = -6; i < 2; i++) {
		if (__get_user(insn, (unsigned int __user *)pc + i))
			break;
		printk("%c%08x%c", i ? ' ' : '<', insn, i ? ' ' : '>');
	}
	printk("\n");
}

static void
dik_show_trace(unsigned long *sp, const char *loglvl)
{
	long i = 0;
	unsigned long tmp;

	printk("%sTrace:\n", loglvl);
	while (0x1ff8 & (unsigned long)sp) {
		tmp = *sp;
		sp++;
		if (!__kernel_text_address(tmp))
			continue;
		printk("%s[<%lx>] %pSR\n", loglvl, tmp, (void *)tmp);
		if (i > 40) {
			printk("%s ...", loglvl);
			break;
		}
	}
	printk("\n");
}

static int kstack_depth_to_print = 24;

void show_stack(struct task_struct *task, unsigned long *sp, const char *loglvl)
{
	unsigned long *stack;
	int i;

	/*
	 * debugging aid: "show_stack(NULL, NULL, KERN_EMERG);" prints the
	 * back trace for this cpu.
	 */
	if (sp == NULL)
		sp = (unsigned long *)&sp;

	stack = sp;
	for (i = 0; i < kstack_depth_to_print; i++) {
		if (((long) stack & (THREAD_SIZE-1)) == 0)
			break;
		if (i && ((i % 4) == 0))
			printk("%s       ", loglvl);
		printk("%016lx ", *stack++);
	}
	printk("\n");
	dik_show_trace(sp, loglvl);
}

void
die_if_kernel(char *str, struct pt_regs *regs, long err, unsigned long *r9_15)
{
	if (regs->ps & 8)
		return;
#ifdef CONFIG_SMP
	printk("CPU %d ", hard_smp_processor_id());
#endif
	printk("%s(%d): %s %ld\n", current->comm, task_pid_nr(current), str, err);
	dik_show_regs(regs, r9_15);
	add_taint(TAINT_DIE, LOCKDEP_NOW_UNRELIABLE);
	dik_show_trace((unsigned long *)(regs+1), KERN_DEFAULT);
	dik_show_code((unsigned int *)regs->pc);

	if (test_and_set_thread_flag(TIF_DIE_IF_KERNEL)) {
		printk("die_if_kernel recursion detected.\n");
		local_irq_enable();
		while (1)
			asm("nop");
	}

	if (kexec_should_crash(current))
		crash_kexec(regs);

	if (panic_on_oops)
		panic("Fatal exception");

	do_exit(SIGSEGV);
}

#ifndef CONFIG_MATHEMU
static long dummy_emul(void)
{
	return 0;
}

long (*sw64_fp_emul_imprecise)(struct pt_regs *regs, unsigned long writemask) = (void *)dummy_emul;
EXPORT_SYMBOL_GPL(sw64_fp_emul_imprecise);

long (*sw64_fp_emul)(unsigned long pc) = (void *)dummy_emul;
EXPORT_SYMBOL_GPL(sw64_fp_emul);
#else
long sw64_fp_emul_imprecise(struct pt_regs *regs, unsigned long writemask);
long sw64_fp_emul(unsigned long pc);
#endif

asmlinkage void
do_entArith(unsigned long summary, unsigned long write_mask,
		struct pt_regs *regs)
{
	long si_code = FPE_FLTINV;

	if (summary & 1) {
		/* Software-completion summary bit is set, so try to
		 * emulate the instruction.  If the processor supports
		 * precise exceptions, we don't have to search.
		 */
		si_code = sw64_fp_emul(regs->pc - 4);
		if (si_code == 0)
			return;
	}
	die_if_kernel("Arithmetic fault", regs, 0, NULL);

	force_sig_fault(SIGFPE, si_code, (void __user *)regs->pc, 0);
}

asmlinkage void
do_entIF(unsigned long inst_type, struct pt_regs *regs)
{
	int signo, code;
	unsigned int inst, type;

	type = inst_type & 0xffffffff;
	inst = inst_type >> 32;

	if ((regs->ps & ~IPL_MAX) == 0 && type != 4) {
		if (type == 1) {
			const unsigned int *data
				= (const unsigned int *) regs->pc;
			printk("Kernel bug at %s:%d\n",
				(const char *)(data[1] | (long)data[2] << 32),
				data[0]);
		} else if (type == 0) {
			/* support kgdb */
			notify_die(0, "kgdb trap", regs, 0, 0, SIGTRAP);
			return;
		}
		die_if_kernel((type == 1 ? "Kernel Bug" : "Instruction fault"),
				regs, type, NULL);
	}

	switch (type) {
	case 0: /* breakpoint */
		if (ptrace_cancel_bpt(current))
			regs->pc -= 4;	/* make pc point to former bpt */

		force_sig_fault(SIGTRAP, TRAP_BRKPT, (void __user *)regs->pc, 0);
		return;

	case 1: /* bugcheck */
		force_sig_fault(SIGTRAP, TRAP_UNK, (void __user *)regs->pc, 0);
		return;

	case 2: /* gentrap */
		switch ((long)regs->r16) {
		case GEN_INTOVF:
			signo = SIGFPE;
			code = FPE_INTOVF;
			break;
		case GEN_INTDIV:
			signo = SIGFPE;
			code = FPE_INTDIV;
			break;
		case GEN_FLTOVF:
			signo = SIGFPE;
			code = FPE_FLTOVF;
			break;
		case GEN_FLTDIV:
			signo = SIGFPE;
			code = FPE_FLTDIV;
			break;
		case GEN_FLTUND:
			signo = SIGFPE;
			code = FPE_FLTUND;
			break;
		case GEN_FLTINV:
			signo = SIGFPE;
			code = FPE_FLTINV;
			break;
		case GEN_FLTINE:
			signo = SIGFPE;
			code = FPE_FLTRES;
			break;
		case GEN_ROPRAND:
			signo = SIGFPE;
			code = FPE_FLTUNK;
			break;

		case GEN_DECOVF:
		case GEN_DECDIV:
		case GEN_DECINV:
		case GEN_ASSERTERR:
		case GEN_NULPTRERR:
		case GEN_STKOVF:
		case GEN_STRLENERR:
		case GEN_SUBSTRERR:
		case GEN_RANGERR:
		case GEN_SUBRNG:
		case GEN_SUBRNG1:
		case GEN_SUBRNG2:
		case GEN_SUBRNG3:
		case GEN_SUBRNG4:
		case GEN_SUBRNG5:
		case GEN_SUBRNG6:
		case GEN_SUBRNG7:
		default:
			signo = SIGTRAP;
			code = TRAP_UNK;
			break;
		}

		force_sig_fault(signo, code, (void __user *)regs->pc, regs->r16);
		return;

	case 4: /* opDEC */
		switch (inst) {
		case BREAK_KPROBE:
			if (notify_die(DIE_BREAK, "kprobe", regs, 0, 0, SIGTRAP) == NOTIFY_STOP)
				return;
		case BREAK_KPROBE_SS:
			if (notify_die(DIE_SSTEPBP, "single_step", regs, 0, 0, SIGTRAP) == NOTIFY_STOP)
				return;
		case UPROBE_BRK_UPROBE:
			if (notify_die(DIE_UPROBE, "uprobe", regs, 0, 0, SIGTRAP) == NOTIFY_STOP)
				return;
		case UPROBE_BRK_UPROBE_XOL:
			if (notify_die(DIE_UPROBE_XOL, "uprobe_xol", regs, 0, 0, SIGTRAP) == NOTIFY_STOP)
				return;
		}
		if ((regs->ps & ~IPL_MAX) == 0)
			die_if_kernel("Instruction fault", regs, type, NULL);
		break;

	case 3: /* FEN fault */
		/*
		 * Irritating users can call HMC_clrfen to disable the
		 * FPU for the process. The kernel will then trap in
		 * do_switch_stack and undo_switch_stack when we try
		 * to save and restore the FP registers.

		 * Given that GCC by default generates code that uses the
		 * FP registers, HMC_clrfen is not useful except for DoS
		 * attacks. So turn the bleeding FPU back on and be done
		 * with it.
		 */
		current_thread_info()->pcb.flags |= 1;
		__reload_thread(&current_thread_info()->pcb);
		return;

	case 5: /* illoc */
	default: /* unexpected instruction-fault type */
		break;
	}

	force_sig_fault(SIGILL, ILL_ILLOPC, (void __user *)regs->pc, 0);
}

/*
 * entUna has a different register layout to be reasonably simple. It
 * needs access to all the integer registers (the kernel doesn't use
 * fp-regs), and it needs to have them in order for simpler access.
 *
 * Due to the non-standard register layout (and because we don't want
 * to handle floating-point regs), user-mode unaligned accesses are
 * handled separately by do_entUnaUser below.
 *
 * Oh, btw, we don't handle the "gp" register correctly, but if we fault
 * on a gp-register unaligned load/store, something is _very_ wrong
 * in the kernel anyway..
 */
struct allregs {
	unsigned long regs[32];
	unsigned long ps, pc, gp, a0, a1, a2;
};

struct unaligned_stat {
	unsigned long count, va, pc;
} unaligned[2];


/* Macro for exception fixup code to access integer registers. */
#define una_reg(r) (_regs[(r) >= 16 && (r) <= 18 ? (r) + 19 : (r)])


asmlinkage void
do_entUna(void *va, unsigned long opcode, unsigned long reg,
	  struct allregs *regs)
{
	long error;
	unsigned long tmp1, tmp2, tmp3, tmp4, tmp5, tmp6, tmp7, tmp8;
	unsigned long pc = regs->pc - 4;
	unsigned long *_regs = regs->regs;
	const struct exception_table_entry *fixup;

	unaligned[0].count++;
	unaligned[0].va = (unsigned long) va;
	unaligned[0].pc = pc;

	/*
	 * We don't want to use the generic get/put unaligned macros as
	 * we want to trap exceptions. Only if we actually get an
	 * exception will we decide whether we should have caught it.
	 */

	switch (opcode) {
	case 0x21:
		__asm__ __volatile__(
		"1:	ldl_u	%1, 0(%3)\n"
		"2:	ldl_u	%2, 1(%3)\n"
		"	extlh	%1, %3, %1\n"
		"	exthh	%2, %3, %2\n"
		"3:\n"
		".section __ex_table,\"a\"\n"
		"	.long	1b - .\n"
		"	ldi	%1, 3b-1b(%0)\n"
		"	.long	2b - .\n"
		"	ldi	%2, 3b-2b(%0)\n"
		".previous"
		: "=r"(error), "=&r"(tmp1), "=&r"(tmp2)
		: "r"(va), "0"(0));

		if (error)
			goto got_exception;
		una_reg(reg) = tmp1 | tmp2;
		return;

	case 0x22:
		__asm__ __volatile__(
		"1:	ldl_u	%1,0(%3)\n"
		"2:	ldl_u	%2,3(%3)\n"
		"	extlw	%1,%3,%1\n"
		"	exthw	%2,%3,%2\n"
		"3:\n"
		".section __ex_table, \"a\"\n"
		"	.long	1b - .\n"
		"	ldi	%1, 3b-1b(%0)\n"
		"	.long	2b - .\n"
		"	ldi	%2, 3b-2b(%0)\n"
		".previous"
		: "=r"(error), "=&r"(tmp1), "=&r"(tmp2)
		: "r"(va), "0"(0));

		if (error)
			goto got_exception;
		una_reg(reg) = (int)(tmp1 | tmp2);
		return;

	case 0x23: /* ldl */
		__asm__ __volatile__(
		"1:	ldl_u	%1, 0(%3)\n"
		"2:	ldl_u	%2, 7(%3)\n"
		"	extll	%1, %3, %1\n"
		"	exthl	%2, %3, %2\n"
		"3:\n"
		".section __ex_table, \"a\"\n"
		"	.long	1b - .\n"
		"	ldi	%1, 3b-1b(%0)\n"
		"	.long	2b - .\n"
		"	ldi	%2, 3b-2b(%0)\n"
		".previous"
		: "=r"(error), "=&r"(tmp1), "=&r"(tmp2)
		: "r"(va), "0"(0));

		if (error)
			goto got_exception;
		una_reg(reg) = tmp1 | tmp2;
		return;

	case 0x29: /* sth */
		__asm__ __volatile__(
		"	zap	%6, 2, %1\n"
		"	srl	%6, 8, %2\n"
		"1:	stb	%1, 0x0(%5)\n"
		"2:	stb	%2, 0x1(%5)\n"
		"3:\n"
		".section __ex_table, \"a\"\n"
		"	.long	1b - .\n"
		"	ldi	%2, 3b-1b(%0)\n"
		"	.long	2b - .\n"
		"	ldi	%1, 3b-2b(%0)\n"
		".previous"
		: "=r"(error), "=&r"(tmp1), "=&r"(tmp2),
		"=&r"(tmp3), "=&r"(tmp4)
		: "r"(va), "r"(una_reg(reg)), "0"(0));

		if (error)
			goto got_exception;
		return;

	case 0x2a: /* stw */
		__asm__ __volatile__(
		"	zapnot	%6, 0x1, %1\n"
		"	srl	%6, 8, %2\n"
		"	zapnot	%2, 0x1,%2\n"
		"	srl	%6, 16, %3\n"
		"	zapnot	%3, 0x1, %3\n"
		"	srl	%6, 24, %4\n"
		"	zapnot	%4, 0x1, %4\n"
		"1:	stb	%1, 0x0(%5)\n"
		"2:	stb	%2, 0x1(%5)\n"
		"3:	stb	%3, 0x2(%5)\n"
		"4:	stb	%4, 0x3(%5)\n"
		"5:\n"
		".section __ex_table, \"a\"\n"
		"	.long	1b - .\n"
		"	ldi	$31, 5b-1b(%0)\n"
		"	.long	2b - .\n"
		"	ldi	$31, 5b-2b(%0)\n"
		"	.long	3b - .\n"
		"	ldi	$31, 5b-3b(%0)\n"
		"	.long	4b - .\n"
		"	ldi	$31, 5b-4b(%0)\n"
		".previous"
		: "=r"(error), "=&r"(tmp1), "=&r"(tmp2),
		  "=&r"(tmp3), "=&r"(tmp4)
		: "r"(va), "r"(una_reg(reg)), "0"(0));

		if (error)
			goto got_exception;
		return;

	case 0x2b: /* stl */
		__asm__ __volatile__(
		"	zapnot	%10, 0x1, %1\n"
		"	srl	%10, 8, %2\n"
		"	zapnot	%2, 0x1, %2\n"
		"	srl	%10, 16, %3\n"
		"	zapnot	%3, 0x1, %3\n"
		"	srl	%10, 24, %4\n"
		"	zapnot	%4, 0x1, %4\n"
		"	srl	%10, 32, %5\n"
		"	zapnot	%5, 0x1, %5\n"
		"	srl	%10, 40, %6\n"
		"	zapnot	%6, 0x1, %6\n"
		"	srl	%10, 48, %7\n"
		"	zapnot	%7, 0x1, %7\n"
		"	srl	%10, 56, %8\n"
		"	zapnot	%8, 0x1, %8\n"
		"1:	stb	%1, 0(%9)\n"
		"2:	stb	%2, 1(%9)\n"
		"3:	stb	%3, 2(%9)\n"
		"4:	stb	%4, 3(%9)\n"
		"5:	stb	%5, 4(%9)\n"
		"6:	stb	%6, 5(%9)\n"
		"7:	stb	%7, 6(%9)\n"
		"8:	stb	%8, 7(%9)\n"
		"9:\n"
		".section __ex_table, \"a\"\n\t"
		"	.long	1b - .\n"
		"	ldi	$31, 9b-1b(%0)\n"
		"	.long	2b - .\n"
		"	ldi	$31, 9b-2b(%0)\n"
		"	.long	3b - .\n"
		"	ldi	$31, 9b-3b(%0)\n"
		"	.long	4b - .\n"
		"	ldi	$31, 9b-4b(%0)\n"
		"	.long	5b - .\n"
		"	ldi	$31, 9b-5b(%0)\n"
		"	.long	6b - .\n"
		"	ldi	$31, 9b-6b(%0)\n"
		"	.long	7b - .\n"
		"	ldi	$31, 9b-7b(%0)\n"
		"	.long	8b - .\n"
		"	ldi	$31, 9b-8b(%0)\n"
		".previous"
		: "=r"(error), "=&r"(tmp1), "=&r"(tmp2), "=&r"(tmp3),
		"=&r"(tmp4), "=&r"(tmp5), "=&r"(tmp6), "=&r"(tmp7), "=&r"(tmp8)
		: "r"(va), "r"(una_reg(reg)), "0"(0));

		if (error)
			goto got_exception;
		return;
	}

	printk("Bad unaligned kernel access at %016lx: %p %lx %lu\n",
		pc, va, opcode, reg);
	do_exit(SIGSEGV);

got_exception:
	/* Ok, we caught the exception, but we don't want it. Is there
	 * someone to pass it along to?
	 */
	fixup = search_exception_tables(pc);
	if (fixup != 0) {
		unsigned long newpc;

		newpc = fixup_exception(una_reg, fixup, pc);
		printk("Forwarding unaligned exception at %lx (%lx)\n",
		       pc, newpc);

		regs->pc = newpc;
		return;
	}

	/*
	 * Yikes!  No one to forward the exception to.
	 * Since the registers are in a weird format, dump them ourselves.
	 */

	printk("%s(%d): unhandled unaligned exception\n",
	       current->comm, task_pid_nr(current));

	printk("pc = [<%016lx>]  ra = [<%016lx>]  ps = %04lx\n",
	       pc, una_reg(26), regs->ps);
	printk("r0 = %016lx  r1 = %016lx  r2 = %016lx\n",
	       una_reg(0), una_reg(1), una_reg(2));
	printk("r3 = %016lx  r4 = %016lx  r5 = %016lx\n",
	       una_reg(3), una_reg(4), una_reg(5));
	printk("r6 = %016lx  r7 = %016lx  r8 = %016lx\n",
	       una_reg(6), una_reg(7), una_reg(8));
	printk("r9 = %016lx  r10= %016lx  r11= %016lx\n",
	       una_reg(9), una_reg(10), una_reg(11));
	printk("r12= %016lx  r13= %016lx  r14= %016lx\n",
	       una_reg(12), una_reg(13), una_reg(14));
	printk("r15= %016lx\n", una_reg(15));
	printk("r16= %016lx  r17= %016lx  r18= %016lx\n",
	       una_reg(16), una_reg(17), una_reg(18));
	printk("r19= %016lx  r20= %016lx  r21= %016lx\n",
	       una_reg(19), una_reg(20), una_reg(21));
	printk("r22= %016lx  r23= %016lx  r24= %016lx\n",
	       una_reg(22), una_reg(23), una_reg(24));
	printk("r25= %016lx  r27= %016lx  r28= %016lx\n",
	       una_reg(25), una_reg(27), una_reg(28));
	printk("gp = %016lx  sp = %p\n", regs->gp, regs+1);

	dik_show_code((unsigned int *)pc);
	dik_show_trace((unsigned long *)(regs+1), KERN_DEFAULT);

	if (test_and_set_thread_flag(TIF_DIE_IF_KERNEL)) {
		printk("die_if_kernel recursion detected.\n");
		local_irq_enable();
		while (1)
			asm("nop");
	}
	do_exit(SIGSEGV);

}

/*
 * Convert an s-floating point value in memory format to the
 * corresponding value in register format. The exponent
 * needs to be remapped to preserve non-finite values
 * (infinities, not-a-numbers, denormals).
 */
static inline unsigned long
s_mem_to_reg(unsigned long s_mem)
{
	unsigned long frac = (s_mem >> 0) & 0x7fffff;
	unsigned long sign = (s_mem >> 31) & 0x1;
	unsigned long exp_msb = (s_mem >> 30) & 0x1;
	unsigned long exp_low = (s_mem >> 23) & 0x7f;
	unsigned long exp;

	exp = (exp_msb << 10) | exp_low;	/* common case */
	if (exp_msb) {
		if (exp_low == 0x7f)
			exp = 0x7ff;
	} else {
		if (exp_low == 0x00)
			exp = 0x000;
		else
			exp |= (0x7 << 7);
	}
	return (sign << 63) | (exp << 52) | (frac << 29);
}

/*
 * Convert an s-floating point value in register format to the
 * corresponding value in memory format.
 */
static inline unsigned long
s_reg_to_mem(unsigned long s_reg)
{
	return ((s_reg >> 62) << 30) | ((s_reg << 5) >> 34);
}

/*
 * Handle user-level unaligned fault. Handling user-level unaligned
 * faults is *extremely* slow and produces nasty messages. A user
 * program *should* fix unaligned faults ASAP.
 *
 * Notice that we have (almost) the regular kernel stack layout here,
 * so finding the appropriate registers is a little more difficult
 * than in the kernel case.
 *
 * Finally, we handle regular integer load/stores only. In
 * particular, load-linked/store-conditionally and floating point
 * load/stores are not supported. The former make no sense with
 * unaligned faults (they are guaranteed to fail) and I don't think
 * the latter will occur in any decent program.
 *
 * Sigh. We *do* have to handle some FP operations, because GCC will
 * uses them as temporary storage for integer memory to memory copies.
 * However, we need to deal with stt/ldt and sts/lds only.
 */
#define OP_INT_MASK	(1L << 0x22 | 1L << 0x2a | /* ldw stw */	\
			 1L << 0x23 | 1L << 0x2b | /* ldl stl */	\
			 1L << 0x21 | 1L << 0x29 | /* ldhu sth */	\
			 1L << 0x20 | 1L << 0x28)  /* ldbu stb */

#define OP_WRITE_MASK	(1L << 0x26 | 1L << 0x27 | /* fsts fstd */	\
			 1L << 0x2c | 1L << 0x2d | /* stw stl */	\
			 1L << 0x0d | 1L << 0x0e)  /* sth stb */

#define R(x)	((size_t) &((struct pt_regs *)0)->x)

static int unauser_reg_offsets[32] = {
	R(r0), R(r1), R(r2), R(r3), R(r4), R(r5), R(r6), R(r7), R(r8),
	/* r9 ... r15 are stored in front of regs. */
	-56, -48, -40, -32, -24, -16, -8,
	R(r16), R(r17), R(r18),
	R(r19), R(r20), R(r21), R(r22), R(r23), R(r24), R(r25), R(r26),
	R(r27), R(r28), R(gp),
	0, 0
};

#undef R

asmlinkage void
do_entUnaUser(void __user *va, unsigned long opcode,
	      unsigned long reg, struct pt_regs *regs)
{
#ifdef CONFIG_UNA_PRINT
	static DEFINE_RATELIMIT_STATE(ratelimit, 5 * HZ, 5);
#endif

	unsigned long tmp1, tmp2, tmp3, tmp4;
	unsigned long fake_reg, *reg_addr = &fake_reg;
	int si_code;
	long error;
	unsigned long tmp, tmp5, tmp6, tmp7, tmp8, vb;
	unsigned long fp[4];
	unsigned long instr, instr_op, value;

	/* Check the UAC bits to decide what the user wants us to do
	 * with the unaliged access.
	 */
	perf_sw_event(PERF_COUNT_SW_ALIGNMENT_FAULTS,
			1, regs, regs->pc - 4);

#ifdef CONFIG_UNA_PRINT
	if (!(current_thread_info()->status & TS_UAC_NOPRINT)) {
		if (__ratelimit(&ratelimit)) {
			printk("%s(%d): unaligned trap at %016lx: %p %lx %ld\n",
			       current->comm, task_pid_nr(current),
			       regs->pc - 4, va, opcode, reg);
		}
	}
#endif
	if ((current_thread_info()->status & TS_UAC_SIGBUS))
		goto give_sigbus;
	/* Not sure why you'd want to use this, but... */
	if ((current_thread_info()->status & TS_UAC_NOFIX))
		return;

	/* Don't bother reading ds in the access check since we already
	 * know that this came from the user. Also rely on the fact that
	 * the page at TASK_SIZE is unmapped and so can't be touched anyway.
	 */
	if ((unsigned long)va >= TASK_SIZE)
		goto give_sigsegv;

	++unaligned[1].count;
	unaligned[1].va = (unsigned long)va;
	unaligned[1].pc = regs->pc - 4;

	if ((1L << opcode) & OP_INT_MASK) {
		/* it's an integer load/store */
		if (reg < 30) {
			reg_addr = (unsigned long *)
				((char *)regs + unauser_reg_offsets[reg]);
		} else if (reg == 30) {
			/* usp in HMCODE regs */
			fake_reg = rdusp();
		} else {
			/* zero "register" */
			fake_reg = 0;
		}
	}

	get_user(instr, (__u32 *)(regs->pc - 4));
	instr_op = (instr >> 26) & 0x3f;

	get_user(value, (__u64 *)va);

	switch (instr_op) {

	case 0x0c:  /* vlds */
		if ((unsigned long)va<<61 == 0) {
			__asm__ __volatile__(
			"1:	ldl	%1, 0(%5)\n"
			"2:	ldl	%2, 8(%5)\n"
			"3:\n"
			".section __ex_table, \"a\"\n"
			"	.long	1b - .\n"
			"	ldi	%1, 3b-1b(%0)\n"
			"	.long	2b - .\n"
			"	ldi	%2, 3b-2b(%0)\n"
			".previous"
			: "=r"(error), "=&r"(tmp1), "=&r"(tmp2), "=&r"(tmp3), "=&r"(tmp4)
			: "r"(va), "0"(0));

			if (error)
				goto give_sigsegv;

			sw64_write_simd_fp_reg_s(reg, tmp1, tmp2);

			return;
		} else {
			__asm__ __volatile__(
			"1:	ldl_u	%1, 0(%6)\n"
			"2:	ldl_u	%2, 7(%6)\n"
			"3:	ldl_u	%3, 15(%6)\n"
			"	extll	%1, %6, %1\n"
			"	extll	%2, %6, %5\n"
			"	exthl	%2, %6, %4\n"
			"	exthl	%3, %6, %3\n"
			"4:\n"
			".section __ex_table, \"a\"\n"
			"	.long	1b - .\n"
			"	ldi	%1, 4b-1b(%0)\n"
			"	.long	2b - .\n"
			"	ldi	%2, 4b-2b(%0)\n"
			"	.long	3b - .\n"
			"	ldi	%3, 4b-3b(%0)\n"
			".previous"
			: "=r"(error), "=&r"(tmp1), "=&r"(tmp2), "=&r"(tmp3),
			  "=&r"(tmp4), "=&r"(tmp5)
			: "r"(va), "0"(0));

			if (error)
				goto give_sigsegv;

			tmp1 = tmp1 | tmp4;
			tmp2 = tmp5 | tmp3;

			sw64_write_simd_fp_reg_s(reg, tmp1, tmp2);

			return;
		}
	case 0x0a: /* ldse */
		__asm__ __volatile__(
		"1:	ldl_u	%1, 0(%3)\n"
		"2:	ldl_u	%2, 3(%3)\n"
		"	extlw	%1, %3, %1\n"
		"	exthw	%2, %3, %2\n"
		"3:\n"
		".section __ex_table, \"a\"\n"
		"	.long	1b - .\n"
		"	ldi	%1, 3b-1b(%0)\n"
		"	.long	2b - .\n"
		"	ldi	%2, 3b-2b(%0)\n"
		".previous"
		: "=r"(error), "=&r"(tmp1), "=&r"(tmp2)
		: "r"(va), "0"(0));

		if (error)
			goto give_sigsegv;

		tmp = tmp1 | tmp2;
		tmp = tmp | (tmp<<32);

		sw64_write_simd_fp_reg_s(reg, tmp, tmp);

		return;

	case 0x0d: /* vldd */
		if ((unsigned long)va<<61 == 0) {
			__asm__ __volatile__(
			"1:	ldl	%1, 0(%5)\n"
			"2:	ldl	%2, 8(%5)\n"
			"3:	ldl	%3, 16(%5)\n"
			"4:	ldl	%4, 24(%5)\n"
			"5:\n"
			".section __ex_table, \"a\"\n"
			"	.long	1b - .\n"
			"	ldi	%1, 5b-1b(%0)\n"
			"	.long	2b - .\n"
			"	ldi	%2, 5b-2b(%0)\n"
			"	.long	3b - .\n"
			"	ldi	%3, 5b-3b(%0)\n"
			"	.long	4b - .\n"
			"	ldi	%4, 5b-4b(%0)\n"
			".previous"
			: "=r"(error), "=&r"(tmp1), "=&r"(tmp2), "=&r"(tmp3), "=&r"(tmp4)
			: "r"(va), "0"(0));

			if (error)
				goto give_sigsegv;

			sw64_write_simd_fp_reg_d(reg, tmp1, tmp2, tmp3, tmp4);

			return;
		} else {
			__asm__ __volatile__(
			"1:	ldl_u	%1, 0(%6)\n"
			"2:	ldl_u	%2, 7(%6)\n"
			"3:	ldl_u	%3, 15(%6)\n"
			"	extll	%1, %6, %1\n"
			"	extll	%2, %6, %5\n"
			"	exthl	%2, %6, %4\n"
			"	exthl	%3, %6, %3\n"
			"4:\n"
			".section __ex_table, \"a\"\n"
			"	.long	1b - .\n"
			"	ldi	%1, 4b-1b(%0)\n"
			"	.long	2b - .\n"
			"	ldi	%2, 4b-2b(%0)\n"
			"	.long	3b - .\n"
			"	ldi	%3, 4b-3b(%0)\n"
			".previous"
			: "=r"(error), "=&r"(tmp1), "=&r"(tmp2), "=&r"(tmp3),
			  "=&r"(tmp4), "=&r"(tmp5)
			: "r"(va), "0"(0));

			if (error)
				goto give_sigsegv;

			tmp7 = tmp1 | tmp4;		//f0
			tmp8 = tmp5 | tmp3;		//f1

			vb = ((unsigned long)(va))+16;

			__asm__ __volatile__(
			"1:	ldl_u	%1, 0(%6)\n"
			"2:	ldl_u	%2, 7(%6)\n"
			"3:	ldl_u	%3, 15(%6)\n"
			"	extll	%1, %6, %1\n"
			"	extll	%2, %6, %5\n"
			"	exthl	%2, %6, %4\n"
			"	exthl	%3, %6, %3\n"
			"4:\n"
			".section __ex_table, \"a\"\n"
			"	.long	1b - .\n"
			"	ldi	%1, 4b-1b(%0)\n"
			"	.long	2b - .\n"
			"	ldi	%2, 4b-2b(%0)\n"
			"	.long	3b - .\n"
			"	ldi	%3, 4b-3b(%0)\n"
			".previous"
			: "=r"(error), "=&r"(tmp1), "=&r"(tmp2), "=&r"(tmp3),
			  "=&r"(tmp4), "=&r"(tmp5)
			: "r"(vb), "0"(0));

			if (error)
				goto give_sigsegv;

			tmp = tmp1 | tmp4;			// f2
			tmp2 = tmp5 | tmp3;			// f3

			sw64_write_simd_fp_reg_d(reg, tmp7, tmp8, tmp, tmp2);
			return;
		}

	case 0x0b: /* ldde */
		__asm__ __volatile__(
		"1:	ldl_u	%1, 0(%3)\n"
		"2:	ldl_u	%2, 7(%3)\n"
		"	extll	%1, %3, %1\n"
		"	exthl	%2, %3, %2\n"
		"3:\n"
		".section __ex_table, \"a\"\n"
		"	.long	1b - .\n"
		"	ldi	%1, 3b-1b(%0)\n"
		"	.long	2b - .\n"
		"	ldi	%2, 3b-2b(%0)\n"
		".previous"
			: "=r"(error), "=&r"(tmp1), "=&r"(tmp2)
			: "r"(va), "0"(0));

		if (error)
			goto give_sigsegv;

		tmp = tmp1 | tmp2;

		sw64_write_simd_fp_reg_d(reg, tmp, tmp, tmp, tmp);
		return;

	case 0x09: /* ldwe */
		__asm__ __volatile__(
		"1:	ldl_u	%1, 0(%3)\n"
		"2:	ldl_u	%2, 3(%3)\n"
		"	extlw	%1, %3, %1\n"
		"	exthw	%2, %3, %2\n"
		"3:\n"
		".section __ex_table, \"a\"\n"
		"	.long	1b - .\n"
		"	ldi	%1, 3b-1b(%0)\n"
		"	.long	2b - .\n"
		"	ldi	%2, 3b-2b(%0)\n"
		".previous"
		: "=r"(error), "=&r"(tmp1), "=&r"(tmp2)
		: "r"(va), "0"(0));

		if (error)
			goto give_sigsegv;

		sw64_write_simd_fp_reg_ldwe(reg, (int)(tmp1 | tmp2));

		return;

	case 0x0e: /* vsts */
		sw64_read_simd_fp_m_s(reg, fp);
		if ((unsigned long)va<<61 == 0) {
			__asm__ __volatile__(
			"	bis	%4, %4, %1\n"
			"	bis	%5, %5, %2\n"
			"1:	stl	%1, 0(%3)\n"
			"2:	stl	%2, 8(%3)\n"
			"3:\n"
			".section __ex_table, \"a\"\n\t"
			"	.long	1b - .\n"
			"	ldi	$31, 3b-1b(%0)\n"
			"	.long	2b - .\n"
			"	ldi	$31, 3b-2b(%0)\n"
			".previous"
			: "=r"(error), "=&r"(tmp1), "=&r"(tmp2)
			: "r"(va), "r"(fp[0]), "r"(fp[1]), "0"(0));

			if (error)
				goto give_sigsegv;

			return;
		} else {
			__asm__ __volatile__(
			"	zapnot	%10, 0x1, %1\n"
			"	srl	%10, 8, %2\n"
			"	zapnot	%2, 0x1, %2\n"
			"	srl	%10, 16, %3\n"
			"	zapnot	%3, 0x1, %3\n"
			"	srl	%10, 24, %4\n"
			"	zapnot	%4, 0x1, %4\n"
			"	srl	%10, 32, %5\n"
			"	zapnot	%5, 0x1, %5\n"
			"	srl	%10, 40, %6\n"
			"	zapnot	%6, 0x1, %6\n"
			"	srl	%10, 48, %7\n"
			"	zapnot	%7, 0x1, %7\n"
			"	srl	%10, 56, %8\n"
			"	zapnot	%8, 0x1, %8\n"
			"1:	stb	%1, 0(%9)\n"
			"2:	stb	%2, 1(%9)\n"
			"3:	stb	%3, 2(%9)\n"
			"4:	stb	%4, 3(%9)\n"
			"5:	stb	%5, 4(%9)\n"
			"6:	stb	%6, 5(%9)\n"
			"7:	stb	%7, 6(%9)\n"
			"8:	stb	%8, 7(%9)\n"
			"9:\n"
			".section __ex_table, \"a\"\n\t"
			"	.long	1b - .\n"
			"	ldi	$31, 9b-1b(%0)\n"
			"	.long	2b - .\n"
			"	ldi	$31, 9b-2b(%0)\n"
			"	.long	3b - .\n"
			"	ldi	$31, 9b-3b(%0)\n"
			"	.long	4b - .\n"
			"	ldi	$31, 9b-4b(%0)\n"
			"	.long	5b - .\n"
			"	ldi	$31, 9b-5b(%0)\n"
			"	.long	6b - .\n"
			"	ldi	$31, 9b-6b(%0)\n"
			"	.long	7b - .\n"
			"	ldi	$31, 9b-7b(%0)\n"
			"	.long	8b - .\n"
			"	ldi	$31, 9b-8b(%0)\n"
			".previous"
			: "=r"(error), "=&r"(tmp1), "=&r"(tmp2), "=&r"(tmp3),
			  "=&r"(tmp4), "=&r"(tmp5), "=&r"(tmp6), "=&r"(tmp7), "=&r"(tmp8)
			: "r"(va), "r"(fp[0]), "0"(0));

			if (error)
				goto give_sigsegv;


			vb = ((unsigned long)va) + 8;

			__asm__ __volatile__(
			"	zapnot	%10, 0x1, %1\n"
			"	srl	%10, 8, %2\n"
			"	zapnot	%2, 0x1, %2\n"
			"	srl	%10, 16, %3\n"
			"	zapnot	%3, 0x1, %3\n"
			"	srl	%10, 24, %4\n"
			"	zapnot	%4, 0x1, %4\n"
			"	srl	%10, 32, %5\n"
			"	zapnot	%5, 0x1, %5\n"
			"	srl	%10, 40, %6\n"
			"	zapnot	%6, 0x1, %6\n"
			"	srl	%10, 48, %7\n"
			"	zapnot	%7, 0x1, %7\n"
			"	srl	%10, 56, %8\n"
			"	zapnot	%8, 0x1, %8\n"
			"1:	stb	%1, 0(%9)\n"
			"2:	stb	%2, 1(%9)\n"
			"3:	stb	%3, 2(%9)\n"
			"4:	stb	%4, 3(%9)\n"
			"5:	stb	%5, 4(%9)\n"
			"6:	stb	%6, 5(%9)\n"
			"7:	stb	%7, 6(%9)\n"
			"8:	stb	%8, 7(%9)\n"
			"9:\n"
			".section __ex_table, \"a\"\n\t"
			"	.long	1b - .\n"
			"	ldi	$31, 9b-1b(%0)\n"
			"	.long	2b - .\n"
			"	ldi	$31, 9b-2b(%0)\n"
			"	.long	3b - .\n"
			"	ldi	$31, 9b-3b(%0)\n"
			"	.long	4b - .\n"
			"	ldi	$31, 9b-4b(%0)\n"
			"	.long	5b - .\n"
			"	ldi	$31, 9b-5b(%0)\n"
			"	.long	6b - .\n"
			"	ldi	$31, 9b-6b(%0)\n"
			"	.long	7b - .\n"
			"	ldi	$31, 9b-7b(%0)\n"
			"	.long	8b - .\n"
			"	ldi	$31, 9b-8b(%0)\n"
			".previous"
			: "=r"(error), "=&r"(tmp1), "=&r"(tmp2), "=&r"(tmp3),
			  "=&r"(tmp4), "=&r"(tmp5), "=&r"(tmp6), "=&r"(tmp7), "=&r"(tmp8)
			: "r"(vb), "r"(fp[1]), "0"(0));

			if (error)
				goto give_sigsegv;

			return;
		}

	case 0x0f: /* vstd */
		sw64_read_simd_fp_m_d(reg, fp);
		if ((unsigned long)va<<61 == 0) {
			__asm__ __volatile__(
			"	bis	%4, %4, %1\n"
			"	bis	%5, %5, %2\n"
			"1:	stl	%1, 0(%3)\n"
			"2:	stl	%2, 8(%3)\n"
			"3:\n"
			".section __ex_table, \"a\"\n\t"
			"	.long	1b - .\n"
			"	ldi	$31, 3b-1b(%0)\n"
			"	.long	2b - .\n"
			"	ldi	$31, 3b-2b(%0)\n"
			".previous"
			: "=r"(error), "=&r"(tmp1), "=&r"(tmp2)
			: "r"(va), "r"(fp[0]), "r"(fp[1]), "0"(0));

			if (error)
				goto give_sigsegv;

			vb = ((unsigned long)va)+16;


			__asm__ __volatile__(
			"	bis	%4, %4, %1\n"
			"	bis	%5, %5, %2\n"
			"1:	stl	%1, 0(%3)\n"
			"2:	stl	%2, 8(%3)\n"
			"3:\n"
			".section __ex_table, \"a\"\n\t"
			"	.long	1b - .\n"
			"	ldi	$31, 3b-1b(%0)\n"
			"	.long	2b - .\n"
			"	ldi	$31, 3b-2b(%0)\n"
			".previous"
			: "=r"(error), "=&r"(tmp1), "=&r"(tmp2)
			: "r"(vb), "r"(fp[2]), "r"(fp[3]), "0"(0));

			if (error)
				goto give_sigsegv;

			return;
		} else {
			__asm__ __volatile__(
			"	zapnot	%10, 0x1, %1\n"
			"	srl	%10, 8, %2\n"
			"	zapnot	%2, 0x1, %2\n"
			"	srl	%10, 16, %3\n"
			"	zapnot	%3, 0x1, %3\n"
			"	srl	%10, 24, %4\n"
			"	zapnot	%4, 0x1, %4\n"
			"	srl	%10, 32, %5\n"
			"	zapnot	%5, 0x1, %5\n"
			"	srl	%10, 40, %6\n"
			"	zapnot	%6, 0x1, %6\n"
			"	srl	%10, 48, %7\n"
			"	zapnot	%7, 0x1, %7\n"
			"	srl	%10, 56, %8\n"
			"	zapnot	%8, 0x1, %8\n"
			"1:	stb	%1, 0(%9)\n"
			"2:	stb	%2, 1(%9)\n"
			"3:	stb	%3, 2(%9)\n"
			"4:	stb	%4, 3(%9)\n"
			"5:	stb	%5, 4(%9)\n"
			"6:	stb	%6, 5(%9)\n"
			"7:	stb	%7, 6(%9)\n"
			"8:	stb	%8, 7(%9)\n"
			"9:\n"
			".section __ex_table, \"a\"\n\t"
			"	.long	1b - .\n"
			"	ldi	$31, 9b-1b(%0)\n"
			"	.long	2b - .\n"
			"	ldi	$31, 9b-2b(%0)\n"
			"	.long	3b - .\n"
			"	ldi	$31, 9b-3b(%0)\n"
			"	.long	4b - .\n"
			"	ldi	$31, 9b-4b(%0)\n"
			"	.long	5b - .\n"
			"	ldi	$31, 9b-5b(%0)\n"
			"	.long	6b - .\n"
			"	ldi	$31, 9b-6b(%0)\n"
			"	.long	7b - .\n"
			"	ldi	$31, 9b-7b(%0)\n"
			"	.long	8b - .\n"
			"	ldi	$31, 9b-8b(%0)\n"
			".previous"
			: "=r"(error), "=&r"(tmp1), "=&r"(tmp2), "=&r"(tmp3),
			  "=&r"(tmp4), "=&r"(tmp5), "=&r"(tmp6), "=&r"(tmp7), "=&r"(tmp8)
			: "r"(va), "r"(fp[0]), "0"(0));

			if (error)
				goto give_sigsegv;

			vb = ((unsigned long)va) + 8;

			__asm__ __volatile__(
			"	zapnot	%10, 0x1, %1\n"
			"	srl	%10, 8, %2\n"
			"	zapnot	%2, 0x1, %2\n"
			"	srl	%10, 16, %3\n"
			"	zapnot	%3, 0x1, %3\n"
			"	srl	%10, 24, %4\n"
			"	zapnot	%4, 0x1, %4\n"
			"	srl	%10, 32, %5\n"
			"	zapnot	%5, 0x1, %5\n"
			"	srl	%10, 40, %6\n"
			"	zapnot	%6, 0x1, %6\n"
			"	srl	%10, 48, %7\n"
			"	zapnot	%7, 0x1, %7\n"
			"	srl	%10, 56, %8\n"
			"	zapnot	%8, 0x1, %8\n"
			"1:	stb	%1, 0(%9)\n"
			"2:	stb	%2, 1(%9)\n"
			"3:	stb	%3, 2(%9)\n"
			"4:	stb	%4, 3(%9)\n"
			"5:	stb	%5, 4(%9)\n"
			"6:	stb	%6, 5(%9)\n"
			"7:	stb	%7, 6(%9)\n"
			"8:	stb	%8, 7(%9)\n"
			"9:\n"
			".section __ex_table, \"a\"\n\t"
			"	.long	1b - .\n"
			"	ldi	$31, 9b-1b(%0)\n"
			"	.long	2b - .\n"
			"	ldi	$31, 9b-2b(%0)\n"
			"	.long	3b - .\n"
			"	ldi	$31, 9b-3b(%0)\n"
			"	.long	4b - .\n"
			"	ldi	$31, 9b-4b(%0)\n"
			"	.long	5b - .\n"
			"	ldi	$31, 9b-5b(%0)\n"
			"	.long	6b - .\n"
			"	ldi	$31, 9b-6b(%0)\n"
			"	.long	7b - .\n"
			"	ldi	$31, 9b-7b(%0)\n"
			"	.long	8b - .\n"
			"	ldi	$31, 9b-8b(%0)\n"
			".previous"
			: "=r"(error), "=&r"(tmp1), "=&r"(tmp2), "=&r"(tmp3),
			  "=&r"(tmp4), "=&r"(tmp5), "=&r"(tmp6), "=&r"(tmp7), "=&r"(tmp8)
			: "r"(vb), "r"(fp[1]), "0"(0));

			if (error)
				goto give_sigsegv;

			vb = vb + 8;

			__asm__ __volatile__(
			"	zapnot	%10, 0x1, %1\n"
			"	srl	%10, 8, %2\n"
			"	zapnot	%2, 0x1, %2\n"
			"	srl	%10, 16, %3\n"
			"	zapnot	%3, 0x1, %3\n"
			"	srl	%10, 24, %4\n"
			"	zapnot	%4, 0x1, %4\n"
			"	srl	%10, 32, %5\n"
			"	zapnot	%5, 0x1, %5\n"
			"	srl	%10, 40, %6\n"
			"	zapnot	%6, 0x1, %6\n"
			"	srl	%10, 48, %7\n"
			"	zapnot	%7, 0x1, %7\n"
			"	srl	%10, 56, %8\n"
			"	zapnot	%8, 0x1, %8\n"
			"1:	stb	%1, 0(%9)\n"
			"2:	stb	%2, 1(%9)\n"
			"3:	stb	%3, 2(%9)\n"
			"4:	stb	%4, 3(%9)\n"
			"5:	stb	%5, 4(%9)\n"
			"6:	stb	%6, 5(%9)\n"
			"7:	stb	%7, 6(%9)\n"
			"8:	stb	%8, 7(%9)\n"
			"9:\n"
			".section __ex_table, \"a\"\n\t"
			"	.long	1b - .\n"
			"	ldi	$31, 9b-1b(%0)\n"
			"	.long	2b - .\n"
			"	ldi	$31, 9b-2b(%0)\n"
			"	.long	3b - .\n"
			"	ldi	$31, 9b-3b(%0)\n"
			"	.long	4b - .\n"
			"	ldi	$31, 9b-4b(%0)\n"
			"	.long	5b - .\n"
			"	ldi	$31, 9b-5b(%0)\n"
			"	.long	6b - .\n"
			"	ldi	$31, 9b-6b(%0)\n"
			"	.long	7b - .\n"
			"	ldi	$31, 9b-7b(%0)\n"
			"	.long	8b - .\n"
			"	ldi	$31, 9b-8b(%0)\n"
			".previous"
			: "=r"(error), "=&r"(tmp1), "=&r"(tmp2), "=&r"(tmp3),
			  "=&r"(tmp4), "=&r"(tmp5), "=&r"(tmp6), "=&r"(tmp7), "=&r"(tmp8)
			: "r"(vb), "r"(fp[2]), "0"(0));

			if (error)
				goto give_sigsegv;

			vb = vb + 8;

			__asm__ __volatile__(
			"	zapnot	%10, 0x1, %1\n"
			"	srl	%10, 8, %2\n"
			"	zapnot	%2, 0x1, %2\n"
			"	srl	%10, 16, %3\n"
			"	zapnot	%3, 0x1, %3\n"
			"	srl	%10, 24, %4\n"
			"	zapnot	%4, 0x1, %4\n"
			"	srl	%10, 32, %5\n"
			"	zapnot	%5, 0x1, %5\n"
			"	srl	%10, 40, %6\n"
			"	zapnot	%6, 0x1, %6\n"
			"	srl	%10, 48, %7\n"
			"	zapnot	%7, 0x1, %7\n"
			"	srl	%10, 56, %8\n"
			"	zapnot	%8, 0x1, %8\n"
			"1:	stb	%1, 0(%9)\n"
			"2:	stb	%2, 1(%9)\n"
			"3:	stb	%3, 2(%9)\n"
			"4:	stb	%4, 3(%9)\n"
			"5:	stb	%5, 4(%9)\n"
			"6:	stb	%6, 5(%9)\n"
			"7:	stb	%7, 6(%9)\n"
			"8:	stb	%8, 7(%9)\n"
			"9:\n"
			".section __ex_table, \"a\"\n\t"
			"	.long	1b - .\n"
			"	ldi	$31, 9b-1b(%0)\n"
			"	.long	2b - .\n"
			"	ldi	$31, 9b-2b(%0)\n"
			"	.long	3b - .\n"
			"	ldi	$31, 9b-3b(%0)\n"
			"	.long	4b - .\n"
			"	ldi	$31, 9b-4b(%0)\n"
			"	.long	5b - .\n"
			"	ldi	$31, 9b-5b(%0)\n"
			"	.long	6b - .\n"
			"	ldi	$31, 9b-6b(%0)\n"
			"	.long	7b - .\n"
			"	ldi	$31, 9b-7b(%0)\n"
			"	.long	8b - .\n"
			"	ldi	$31, 9b-8b(%0)\n"
			".previous"
			: "=r"(error), "=&r"(tmp1), "=&r"(tmp2), "=&r"(tmp3),
			  "=&r"(tmp4), "=&r"(tmp5), "=&r"(tmp6), "=&r"(tmp7), "=&r"(tmp8)
			: "r"(vb), "r"(fp[3]), "0"(0));

			if (error)
				goto give_sigsegv;

			return;
		}
	}
	switch (opcode) {
	case 0x21: /* ldhu */
		__asm__ __volatile__(
		"1:	ldl_u	%1, 0(%3)\n"
		"2:	ldl_u	%2, 1(%3)\n"
		"	extlh	%1, %3, %1\n"
		"	exthh	%2, %3, %2\n"
		"3:\n"
		".section __ex_table, \"a\"\n"
		"	.long	1b - .\n"
		"	ldi	%1, 3b-1b(%0)\n"
		"	.long	2b - .\n"
		"	ldi	%2, 3b-2b(%0)\n"
		".previous"
		: "=r"(error), "=&r"(tmp1), "=&r"(tmp2)
		: "r"(va), "0"(0));
		if (error)
			goto give_sigsegv;
		*reg_addr = tmp1 | tmp2;
		break;

	case 0x26: /* flds */
		__asm__ __volatile__(
		"1:	ldl_u	%1, 0(%3)\n"
		"2:	ldl_u	%2, 3(%3)\n"
		"	extlw	%1, %3, %1\n"
		"	exthw	%2, %3, %2\n"
		"3:\n"
		".section __ex_table, \"a\"\n"
		"	.long	1b - .\n"
		"	ldi	%1, 3b-1b(%0)\n"
		"	.long	2b - .\n"
		"	ldi	%2, 3b-2b(%0)\n"
		".previous"
		: "=r"(error), "=&r"(tmp1), "=&r"(tmp2)
		: "r"(va), "0"(0));
		if (error)
			goto give_sigsegv;
		sw64_write_fp_reg(reg, s_mem_to_reg((int)(tmp1 | tmp2)));
		return;

	case 0x27: /* fldd */
		__asm__ __volatile__(
		"1:	ldl_u	%1, 0(%3)\n"
		"2:	ldl_u	%2, 7(%3)\n"
		"	extll	%1, %3, %1\n"
		"	exthl	%2, %3, %2\n"
		"3:\n"
		".section __ex_table, \"a\"\n"
		"	.long	1b - .\n"
		"	ldi	%1, 3b-1b(%0)\n"
		"	.long	2b - .\n"
		"	ldi	%2, 3b-2b(%0)\n"
		".previous"
		: "=r"(error), "=&r"(tmp1), "=&r"(tmp2)
		: "r"(va), "0"(0));
		if (error)
			goto give_sigsegv;
		sw64_write_fp_reg(reg, tmp1 | tmp2);
		return;

	case 0x22: /* ldw */
		__asm__ __volatile__(
		"1:	ldl_u	%1, 0(%3)\n"
		"2:	ldl_u	%2, 3(%3)\n"
		"	extlw	%1, %3, %1\n"
		"	exthw	%2, %3, %2\n"
		"3:\n"
		".section __ex_table, \"a\"\n"
		"	.long	1b - .\n"
		"	ldi	%1, 3b-1b(%0)\n"
		"	.long	2b - .\n"
		"	ldi	%2, 3b-2b(%0)\n"
		".previous"
		: "=r"(error), "=&r"(tmp1), "=&r"(tmp2)
		: "r"(va), "0"(0));
		if (error)
			goto give_sigsegv;
		*reg_addr = (int)(tmp1 | tmp2);
		break;

	case 0x23: /* ldl */
		__asm__ __volatile__(
		"1:	ldl_u	%1, 0(%3)\n"
		"2:	ldl_u	%2, 7(%3)\n"
		"	extll	%1, %3, %1\n"
		"	exthl	%2, %3, %2\n"
		"3:\n"
		".section __ex_table, \"a\"\n"
		"	.long	1b - .\n"
		"	ldi	%1, 3b-1b(%0)\n"
		"	.long	2b - .\n"
		"	ldi	%2, 3b-2b(%0)\n"
		".previous"
		: "=r"(error), "=&r"(tmp1), "=&r"(tmp2)
		: "r"(va), "0"(0));
		if (error)
			goto give_sigsegv;
		*reg_addr = tmp1 | tmp2;
		break;

	/* Note that the store sequences do not indicate that they change
	 * memory because it _should_ be affecting nothing in this context.
	 * (Otherwise we have other, much larger, problems.)
	 */
	case 0x29: /* sth with stb */
		__asm__ __volatile__(
		"	zap	%6, 2, %1\n"
		"	srl	%6, 8, %2\n"
		"1:	stb	%1, 0x0(%5)\n"
		"2:	stb	%2, 0x1(%5)\n"
		"3:\n"
		".section __ex_table, \"a\"\n"
		"	.long	1b - .\n"
		"	ldi	%2, 3b-1b(%0)\n"
		"	.long	2b - .\n"
		"	ldi	%1, 3b-2b(%0)\n"
		".previous"
		: "=r"(error), "=&r"(tmp1), "=&r"(tmp2),
		  "=&r"(tmp3), "=&r"(tmp4)
		: "r"(va), "r"(*reg_addr), "0"(0));

		if (error)
			goto give_sigsegv;
		return;

	case 0x2e: /* fsts*/
		fake_reg = s_reg_to_mem(sw64_read_fp_reg(reg));
		/* FALLTHRU */

	case 0x2a: /* stw with stb*/
		__asm__ __volatile__(
		"	zapnot	%6, 0x1, %1\n"
		"	srl	%6, 8, %2\n"
		"	zapnot	%2, 0x1, %2\n"
		"	srl	%6, 16, %3\n"
		"	zapnot	%3, 0x1, %3\n"
		"	srl	%6, 24, %4\n"
		"	zapnot	%4, 0x1, %4\n"
		"1:	stb  %1, 0x0(%5)\n"
		"2:	stb  %2, 0x1(%5)\n"
		"3:	stb  %3, 0x2(%5)\n"
		"4:	stb  %4, 0x3(%5)\n"
		"5:\n"
		".section __ex_table, \"a\"\n"
		"	.long	1b - .\n"
		"	ldi	$31, 5b-1b(%0)\n"
		"	.long	2b - .\n"
		"	ldi	$31, 5b-2b(%0)\n"
		"	.long	3b - .\n"
		"	ldi	$31, 5b-3b(%0)\n"
		"	.long	4b - .\n"
		"	ldi	$31, 5b-4b(%0)\n"
		".previous"
		: "=r"(error), "=&r"(tmp1), "=&r"(tmp2),
		  "=&r"(tmp3), "=&r"(tmp4)
		: "r"(va), "r"(*reg_addr), "0"(0));

		if (error)
			goto give_sigsegv;
		return;

	case 0x2f: /* fstd */
		fake_reg = sw64_read_fp_reg(reg);
		/* FALLTHRU */

	case 0x2b: /* stl */
		__asm__ __volatile__(
		"	zapnot	%10, 0x1, %1\n"
		"	srl	%10, 8, %2\n"
		"	zapnot	%2, 0x1, %2\n"
		"	srl	%10, 16, %3\n"
		"	zapnot	%3, 0x1, %3\n"
		"	srl	%10, 24, %4\n"
		"	zapnot	%4, 0x1, %4\n"
		"	srl	%10, 32, %5\n"
		"	zapnot	%5, 0x1, %5\n"
		"	srl	%10, 40, %6\n"
		"	zapnot	%6, 0x1, %6\n"
		"	srl	%10, 48, %7\n"
		"	zapnot	%7, 0x1, %7\n"
		"	srl	%10, 56, %8\n"
		"	zapnot	%8, 0x1, %8\n"
		"1:	stb	%1, 0(%9)\n"
		"2:	stb	%2, 1(%9)\n"
		"3:	stb	%3, 2(%9)\n"
		"4:	stb	%4, 3(%9)\n"
		"5:	stb	%5, 4(%9)\n"
		"6:	stb	%6, 5(%9)\n"
		"7:	stb	%7, 6(%9)\n"
		"8:	stb	%8, 7(%9)\n"
		"9:\n"
		".section __ex_table, \"a\"\n\t"
		"	.long	1b - .\n"
		"	ldi	$31, 9b-1b(%0)\n"
		"	.long	2b - .\n"
		"	ldi	$31, 9b-2b(%0)\n"
		"	.long	3b - .\n"
		"	ldi	$31, 9b-3b(%0)\n"
		"	.long	4b - .\n"
		"	ldi	$31, 9b-4b(%0)\n"
		"	.long	5b - .\n"
		"	ldi	$31, 9b-5b(%0)\n"
		"	.long	6b - .\n"
		"	ldi	$31, 9b-6b(%0)\n"
		"	.long	7b - .\n"
		"	ldi	$31, 9b-7b(%0)\n"
		"	.long	8b - .\n"
		"	ldi	$31, 9b-8b(%0)\n"
		".previous"
		: "=r"(error), "=&r"(tmp1), "=&r"(tmp2), "=&r"(tmp3),
		  "=&r"(tmp4), "=&r"(tmp5), "=&r"(tmp6), "=&r"(tmp7), "=&r"(tmp8)
		: "r"(va), "r"(*reg_addr), "0"(0));

		if (error)
			goto give_sigsegv;
		return;

	default:
		/* What instruction were you trying to use, exactly? */
		goto give_sigbus;
	}

	/* Only integer loads should get here; everyone else returns early. */
	if (reg == 30)
		wrusp(fake_reg);
	return;

give_sigsegv:
	regs->pc -= 4;  /* make pc point to faulting insn */

	/* We need to replicate some of the logic in mm/fault.c,
	 * since we don't have access to the fault code in the
	 * exception handling return path.
	 */
	if ((unsigned long)va >= TASK_SIZE)
		si_code = SEGV_ACCERR;
	else {
		struct mm_struct *mm = current->mm;

		down_read(&mm->mmap_lock);
		if (find_vma(mm, (unsigned long)va))
			si_code = SEGV_ACCERR;
		else
			si_code = SEGV_MAPERR;
		up_read(&mm->mmap_lock);
	}
	force_sig_fault(SIGSEGV, si_code, va, 0);
	return;

give_sigbus:
	regs->pc -= 4;
	force_sig_fault(SIGBUS, BUS_ADRALN, va, 0);
}

void
trap_init(void)
{
	/* Tell HMcode what global pointer we want in the kernel. */
	register unsigned long gptr __asm__("$29");
	wrkgp(gptr);

	wrent(entArith, 1);
	wrent(entMM, 2);
	wrent(entIF, 3);
	wrent(entUna, 4);
	wrent(entSys, 5);
}
