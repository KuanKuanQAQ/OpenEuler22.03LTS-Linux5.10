/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (C) 2012 ARM Ltd.
 */
#ifndef __ASM_MODULE_H
#define __ASM_MODULE_H

#include <asm-generic/module.h>

#ifdef CONFIG_ARM64_MODULE_PLTS
struct mod_plt_sec {
	int			plt_shndx;
	int			plt_num_entries;
	int			plt_max_entries;
};

struct mod_arch_specific {
	struct mod_plt_sec	core;
	struct mod_plt_sec	init;

	/* for CONFIG_DYNAMIC_FTRACE */
	struct plt_entry	*ftrace_trampolines;
};
#endif

void add_pause_list(void);

void remove_pause_list(void);

#ifdef CONFIG_ARM64_MODULE_RERANDOMIZE

#define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
#define printp(x) printk("(%s.%03d): " #x " = 0x%lx\n", __FILENAME__, __LINE__, (unsigned long)(x))
#define INC_BY_DELTA(x, delta) ( x = (typeof((x))) ((unsigned long)(x) + (unsigned long)(delta)) )
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
#else /* !CONFIG_X86_MODULE_RERANDOMIZE */
#define SPECIAL_FUNCTION_PROTO(ret, name, args...) ret name (args)
#define SPECIAL_FUNCTION(ret, name, args...) ret name (args)
#endif /* CONFIG_X86_MODULE_RERANDOMIZE */

u64 module_emit_plt_entry(struct module *mod, Elf64_Shdr *sechdrs,
			  void *loc, const Elf64_Rela *rela,
			  Elf64_Sym *sym);

u64 module_emit_veneer_for_adrp(struct module *mod, Elf64_Shdr *sechdrs,
				void *loc, u64 val);

#ifdef CONFIG_RANDOMIZE_BASE
extern u64 module_alloc_base;
#else
#define module_alloc_base	((u64)_etext - MODULES_VSIZE)
#endif

struct plt_entry {
	/*
	 * A program that conforms to the AArch64 Procedure Call Standard
	 * (AAPCS64) must assume that a veneer that alters IP0 (x16) and/or
	 * IP1 (x17) may be inserted at any branch instruction that is
	 * exposed to a relocation that supports long branches. Since that
	 * is exactly what we are dealing with here, we are free to use x16
	 * as a scratch register in the PLT veneers.
	 */
	__le32	adrp;	/* adrp	x16, ....			*/
	__le32	add;	/* add	x16, x16, #0x....		*/
	__le32	br;	/* br	x16				*/
};

static inline bool is_forbidden_offset_for_adrp(void *place)
{
	return IS_ENABLED(CONFIG_ARM64_ERRATUM_843419) &&
	       cpus_have_const_cap(ARM64_WORKAROUND_843419) &&
	       ((u64)place & 0xfff) >= 0xff8;
}

struct plt_entry get_plt_entry(u64 dst, void *pc);
bool plt_entries_equal(const struct plt_entry *a, const struct plt_entry *b);

static inline bool plt_entry_is_initialized(const struct plt_entry *e)
{
	return e->adrp || e->add || e->br;
}

#endif /* __ASM_MODULE_H */
