/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __ASM_MCS_LOCK_H
#define __ASM_MCS_LOCK_H

#ifdef CONFIG_SMP
#include <asm/spinlock.h>

/* MCS spin-locking. */
#define arch_mcs_spin_wait(lock)					\
do {									\
	/* Ensure prior stores are observed before we enter wfe. */	\
	smp_mb();							\
	while (!(smp_load_acquire(lock)))				\
		wfe();							\
} while (0)								\

#define arch_mcs_lock_handoff(lock, val)				\
do {									\
	smp_store_release((lock), (val));				\
	dsb_sev();							\
} while (0)

#endif	/* CONFIG_SMP */
#endif	/* __ASM_MCS_LOCK_H */
