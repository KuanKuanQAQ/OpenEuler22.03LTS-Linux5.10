// SPDX-License-Identifier: GPL-2.0
/*
 * sw64 KGDB support
 *
 * Based on arch/arm64/kernel/kgdb.c
 *
 * Copyright (C) Xia Bin
 * Author: Xia Bin
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <linux/kdebug.h>
#include <linux/kgdb.h>

struct dbg_reg_def_t dbg_reg_def[DBG_MAX_REG_NUM] = {
	{ "r0", 8, offsetof(struct pt_regs, r0)},
	{ "r1", 8, offsetof(struct pt_regs, r1)},
	{ "r2", 8, offsetof(struct pt_regs, r2)},
	{ "r3", 8, offsetof(struct pt_regs, r3)},
	{ "r4", 8, offsetof(struct pt_regs, r4)},
	{ "r5", 8, offsetof(struct pt_regs, r5)},
	{ "r6", 8, offsetof(struct pt_regs, r6)},
	{ "r7", 8, offsetof(struct pt_regs, r7)},
	{ "r8", 8, offsetof(struct pt_regs, r8)},

	{ "r9",  8, -1 },
	{ "r10", 8, -1 },
	{ "r11", 8, -1 },
	{ "r12", 8, -1 },
	{ "r13", 8, -1 },
	{ "r14", 8, -1 },
	{ "r15", 8, -1 },

	{ "r16", 8, offsetof(struct pt_regs, r16)},
	{ "r17", 8, offsetof(struct pt_regs, r17)},
	{ "r18", 8, offsetof(struct pt_regs, r18)},

	{ "r19", 8, offsetof(struct pt_regs, r19)},
	{ "r20", 8, offsetof(struct pt_regs, r20)},
	{ "r21", 8, offsetof(struct pt_regs, r21)},
	{ "r22", 8, offsetof(struct pt_regs, r22)},
	{ "r23", 8, offsetof(struct pt_regs, r23)},
	{ "r24", 8, offsetof(struct pt_regs, r24)},
	{ "r25", 8, offsetof(struct pt_regs, r25)},
	{ "r26", 8, offsetof(struct pt_regs, r26)},
	{ "r27", 8, offsetof(struct pt_regs, r27)},
	{ "at", 8, offsetof(struct pt_regs, r28)},
	{ "gp", 8, offsetof(struct pt_regs, gp)},
	{ "sp", 8, -1 },
	{ "zero", 8, -1 },

	{ "f0", 8, -1 },
	{ "f1", 8, -1 },
	{ "f2", 8, -1 },
	{ "f3", 8, -1 },
	{ "f4", 8, -1 },
	{ "f5", 8, -1 },
	{ "f6", 8, -1 },
	{ "f7", 8, -1 },
	{ "f8", 8, -1 },
	{ "f9", 8, -1 },
	{ "f10", 8, -1 },
	{ "f11", 8, -1 },
	{ "f12", 8, -1 },
	{ "f13", 8, -1 },
	{ "f14", 8, -1 },
	{ "f15", 8, -1 },
	{ "f16", 8, -1 },
	{ "f17", 8, -1 },
	{ "f18", 8, -1 },
	{ "f19", 8, -1 },
	{ "f20", 8, -1 },
	{ "f21", 8, -1 },
	{ "f22", 8, -1 },
	{ "f23", 8, -1 },
	{ "f24", 8, -1 },
	{ "f25", 8, -1 },
	{ "f26", 8, -1 },
	{ "f27", 8, -1 },
	{ "f28", 8, -1 },
	{ "f29", 8, -1 },
	{ "f30", 8, -1 },
	{ "fpcr", 8, -1 },

	{ "pc", 8, offsetof(struct pt_regs, pc)},
	{ "", 8, -1 },
	{ "unique", 8, -1},
};

char *dbg_get_reg(int regno, void *mem, struct pt_regs *regs)
{
	if (regno >= DBG_MAX_REG_NUM || regno < 0)
		return NULL;

	if (dbg_reg_def[regno].offset != -1)
		memcpy(mem, (void *)regs + dbg_reg_def[regno].offset,
				dbg_reg_def[regno].size);
	else
		memset(mem, 0, dbg_reg_def[regno].size);
	return dbg_reg_def[regno].name;
}

int dbg_set_reg(int regno, void *mem, struct pt_regs *regs)
{
	if (regno >= DBG_MAX_REG_NUM || regno < 0)
		return -EINVAL;

	if (dbg_reg_def[regno].offset != -1)
		memcpy((void *)regs + dbg_reg_def[regno].offset, mem,
				dbg_reg_def[regno].size);
	return 0;
}

void
sleeping_thread_to_gdb_regs(unsigned long *gdb_regs, struct task_struct *task)
{
	int i;
	/* Initialize to zero */
	memset((char *)gdb_regs, 0, NUMREGBYTES);
	for (i = 0; i < DBG_MAX_REG_NUM; i++)
		gdb_regs[i] = get_reg(task, i);
}

void kgdb_arch_set_pc(struct pt_regs *regs, unsigned long pc)
{
	pr_info("BEFORE SET PC WITH %lx\n", pc);
	instruction_pointer(regs) = pc;
	pr_info("AFTER SET PC IS %lx\n", instruction_pointer(regs));
}

void kgdb_call_nmi_hook(void *ignored)
{
	kgdb_nmicallback(raw_smp_processor_id(), NULL);
}

void kgdb_roundup_cpus(void)
{
	local_irq_enable();
	smp_call_function(kgdb_call_nmi_hook, NULL, 0);
	local_irq_disable();
}

int kgdb_arch_handle_exception(int exception_vector, int signo,
			       int err_code, char *remcom_in_buffer,
			       char *remcom_out_buffer,
			       struct pt_regs *linux_regs)
{
	char *ptr;
	unsigned long address = -1;

	switch (remcom_in_buffer[0]) {
	case 'c':
		ptr = &remcom_in_buffer[1];
		if (kgdb_hex2long(&ptr, &address))
			kgdb_arch_set_pc(linux_regs, address);
		return 0;
	}
	return -1;
}

static int __kgdb_notify(struct die_args *args, unsigned long cmd)
{
	struct pt_regs *regs = args->regs;

	/* Userspace events, ignore. */
	if (user_mode(regs))
		return NOTIFY_DONE;

	if (kgdb_handle_exception(1, args->signr, cmd, regs))
		return  NOTIFY_DONE;

	return NOTIFY_STOP;
}

static int
kgdb_notify(struct notifier_block *self, unsigned long cmd, void *ptr)
{
	unsigned long flags;
	int ret;

	local_irq_save(flags);
	ret = __kgdb_notify(ptr, cmd);
	local_irq_restore(flags);

	return ret;
}

static struct notifier_block kgdb_notifier = {
	.notifier_call  = kgdb_notify,
};

/*
 * kgdb_arch_init - Perform any architecture specific initalization.
 * This function will handle the initalization of any architecture
 * specific callbacks.
 */
int kgdb_arch_init(void)
{
	int ret = register_die_notifier(&kgdb_notifier);

	if (ret != 0)
		return ret;
	return 0;
}

/*
 * kgdb_arch_exit - Perform any architecture specific uninitalization.
 * This function will handle the uninitalization of any architecture
 * specific callbacks, for dynamic registration and unregistration.
 */
void kgdb_arch_exit(void)
{
	unregister_die_notifier(&kgdb_notifier);
}

/*
 * sw64 instructions are always in LE.
 * Break instruction is encoded in LE format
 */
const struct kgdb_arch arch_kgdb_ops = {
	.gdb_bpt_instr = {0x80, 00, 00, 00}
};
