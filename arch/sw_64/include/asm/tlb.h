/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _ASM_SW64_TLB_H
#define _ASM_SW64_TLB_H

#define tlb_start_vma(tlb, vma)			do { } while (0)
#define tlb_end_vma(tlb, vma)			do { } while (0)
#define __tlb_remove_tlb_entry(tlb, pte, addr)	do { } while (0)

#define tlb_flush(tlb)				flush_tlb_mm((tlb)->mm)

#include <asm-generic/tlb.h>

#define __pte_free_tlb(tlb, pte, address)	pte_free((tlb)->mm, pte)
#define __pmd_free_tlb(tlb, pmd, address)	pmd_free((tlb)->mm, pmd)

#define __pud_free_tlb(tlb, pud, address)	pud_free((tlb)->mm, pud)

#endif
