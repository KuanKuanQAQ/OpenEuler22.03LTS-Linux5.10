/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _ASM_SW64_EFI_H
#define _ASM_SW64_EFI_H

#include <asm/io.h>
#include <asm/early_ioremap.h>
#ifdef CONFIG_EFI
extern void efi_init(void);
#else
#define efi_init()
#define efi_idmap_init()
#endif

#define arch_efi_call_virt_setup()
#define arch_efi_call_virt_teardown()

#define arch_efi_call_virt(p, f, args...)				\
({									\
	efi_##f##_t * __f;						\
	__f = p->f;							\
	__f(args);							\
})

#define	ARCH_EFI_IRQ_FLAGS_MASK		0x00000001

/* arch specific definitions used by the stub code */

/*
 * AArch64 requires the DTB to be 8-byte aligned in the first 512MiB from
 * start of kernel and may not cross a 2MiB boundary. We set alignment to
 * 2MiB so we know it won't cross a 2MiB boundary.
 */
#define EFI_FDT_ALIGN	SZ_2M   /* used by allocate_new_fdt_and_exit_boot() */
#define MAX_FDT_OFFSET	SZ_512M

#define efi_call_early(f, ...) sys_table_arg->boottime->f(__VA_ARGS__)

#endif /* _ASM_SW64_EFI_H */
