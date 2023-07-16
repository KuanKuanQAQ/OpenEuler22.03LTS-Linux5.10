// SPDX-License-Identifier: GPL-2.0
/*
 * Generate definitions needed by assembly language modules.
 * This code generates raw asm output which is post-processed to extract
 * and format the required data.
 */

#include <linux/stddef.h>
#include <linux/sched.h>
#include <linux/kbuild.h>
#include <linux/suspend.h>

#include <asm/suspend.h>
#include <asm/kvm.h>

#include "traps.c"

void foo(void)
{
	DEFINE(TI_TASK, offsetof(struct thread_info, task));
	DEFINE(TI_FLAGS, offsetof(struct thread_info, flags));
	DEFINE(TI_CPU, offsetof(struct thread_info, cpu));
	BLANK();

	DEFINE(TASK_BLOCKED, offsetof(struct task_struct, blocked));
	DEFINE(TASK_CRED, offsetof(struct task_struct, cred));
	DEFINE(TASK_REAL_PARENT, offsetof(struct task_struct, real_parent));
	DEFINE(TASK_GROUP_LEADER, offsetof(struct task_struct, group_leader));
	DEFINE(TASK_TGID, offsetof(struct task_struct, tgid));
	BLANK();

	OFFSET(PSTATE_REGS, processor_state, regs);
	OFFSET(PSTATE_FPREGS, processor_state, fpregs);
	OFFSET(PSTATE_FPCR, processor_state, fpcr);
#ifdef CONFIG_HIBERNATION
	OFFSET(PSTATE_PCB, processor_state, pcb);
#endif
	OFFSET(PCB_KSP, pcb_struct, ksp);
	OFFSET(PBE_ADDR, pbe, address);
	OFFSET(PBE_ORIG_ADDR, pbe, orig_address);
	OFFSET(PBE_NEXT, pbe, next);
	OFFSET(CALLEE_R9, callee_saved_regs, r9);
	OFFSET(CALLEE_R10, callee_saved_regs, r10);
	OFFSET(CALLEE_R11, callee_saved_regs, r11);
	OFFSET(CALLEE_R12, callee_saved_regs, r12);
	OFFSET(CALLEE_R13, callee_saved_regs, r13);
	OFFSET(CALLEE_R14, callee_saved_regs, r14);
	OFFSET(CALLEE_R15, callee_saved_regs, r15);
	OFFSET(CALLEE_RA, callee_saved_regs, ra);
	OFFSET(CALLEE_F2, callee_saved_fpregs, f2);
	OFFSET(CALLEE_F3, callee_saved_fpregs, f3);
	OFFSET(CALLEE_F4, callee_saved_fpregs, f4);
	OFFSET(CALLEE_F5, callee_saved_fpregs, f5);
	OFFSET(CALLEE_F6, callee_saved_fpregs, f6);
	OFFSET(CALLEE_F7, callee_saved_fpregs, f7);
	OFFSET(CALLEE_F8, callee_saved_fpregs, f8);
	OFFSET(CALLEE_F9, callee_saved_fpregs, f9);
	BLANK();
	DEFINE(CRED_UID,  offsetof(struct cred, uid));
	DEFINE(CRED_EUID, offsetof(struct cred, euid));
	DEFINE(CRED_GID,  offsetof(struct cred, gid));
	DEFINE(CRED_EGID, offsetof(struct cred, egid));
	BLANK();

	DEFINE(PT_REGS_SIZE, sizeof(struct pt_regs));
	DEFINE(PT_REGS_R0, offsetof(struct pt_regs, r0));
	DEFINE(PT_REGS_R1, offsetof(struct pt_regs, r1));
	DEFINE(PT_REGS_R2, offsetof(struct pt_regs, r2));
	DEFINE(PT_REGS_R3, offsetof(struct pt_regs, r3));
	DEFINE(PT_REGS_R4, offsetof(struct pt_regs, r4));
	DEFINE(PT_REGS_R5, offsetof(struct pt_regs, r5));
	DEFINE(PT_REGS_R6, offsetof(struct pt_regs, r6));
	DEFINE(PT_REGS_R7, offsetof(struct pt_regs, r7));
	DEFINE(PT_REGS_R8, offsetof(struct pt_regs, r8));
	DEFINE(PT_REGS_R19, offsetof(struct pt_regs, r19));
	DEFINE(PT_REGS_R20, offsetof(struct pt_regs, r20));
	DEFINE(PT_REGS_R21, offsetof(struct pt_regs, r21));
	DEFINE(PT_REGS_R22, offsetof(struct pt_regs, r22));
	DEFINE(PT_REGS_R23, offsetof(struct pt_regs, r23));
	DEFINE(PT_REGS_R24, offsetof(struct pt_regs, r24));
	DEFINE(PT_REGS_R25, offsetof(struct pt_regs, r25));
	DEFINE(PT_REGS_R26, offsetof(struct pt_regs, r26));
	DEFINE(PT_REGS_R27, offsetof(struct pt_regs, r27));
	DEFINE(PT_REGS_R28, offsetof(struct pt_regs, r28));
	DEFINE(PT_REGS_TRAP_A0, offsetof(struct pt_regs, trap_a0));
	DEFINE(PT_REGS_TRAP_A1, offsetof(struct pt_regs, trap_a1));
	DEFINE(PT_REGS_TRAP_A2, offsetof(struct pt_regs, trap_a2));
	DEFINE(PT_REGS_PS, offsetof(struct pt_regs, ps));
	DEFINE(PT_REGS_PC, offsetof(struct pt_regs, pc));
	DEFINE(PT_REGS_GP, offsetof(struct pt_regs, gp));
	DEFINE(PT_REGS_R16, offsetof(struct pt_regs, r16));
	DEFINE(PT_REGS_R17, offsetof(struct pt_regs, r17));
	DEFINE(PT_REGS_R18, offsetof(struct pt_regs, r18));
	BLANK();

	DEFINE(SWITCH_STACK_SIZE, sizeof(struct switch_stack));
	DEFINE(SWITCH_STACK_R9, offsetof(struct switch_stack, r9));
	DEFINE(SWITCH_STACK_R10, offsetof(struct switch_stack, r10));
	DEFINE(SWITCH_STACK_R11, offsetof(struct switch_stack, r11));
	DEFINE(SWITCH_STACK_R12, offsetof(struct switch_stack, r12));
	DEFINE(SWITCH_STACK_R13, offsetof(struct switch_stack, r13));
	DEFINE(SWITCH_STACK_R14, offsetof(struct switch_stack, r14));
	DEFINE(SWITCH_STACK_R15, offsetof(struct switch_stack, r15));
	DEFINE(SWITCH_STACK_RA, offsetof(struct switch_stack, r26));
	BLANK();

	DEFINE(ALLREGS_SIZE, sizeof(struct allregs));
	DEFINE(ALLREGS_R0, offsetof(struct allregs, regs[0]));
	DEFINE(ALLREGS_R1, offsetof(struct allregs, regs[1]));
	DEFINE(ALLREGS_R2, offsetof(struct allregs, regs[2]));
	DEFINE(ALLREGS_R3, offsetof(struct allregs, regs[3]));
	DEFINE(ALLREGS_R4, offsetof(struct allregs, regs[4]));
	DEFINE(ALLREGS_R5, offsetof(struct allregs, regs[5]));
	DEFINE(ALLREGS_R6, offsetof(struct allregs, regs[6]));
	DEFINE(ALLREGS_R7, offsetof(struct allregs, regs[7]));
	DEFINE(ALLREGS_R8, offsetof(struct allregs, regs[8]));
	DEFINE(ALLREGS_R9, offsetof(struct allregs, regs[9]));
	DEFINE(ALLREGS_R10, offsetof(struct allregs, regs[10]));
	DEFINE(ALLREGS_R11, offsetof(struct allregs, regs[11]));
	DEFINE(ALLREGS_R12, offsetof(struct allregs, regs[12]));
	DEFINE(ALLREGS_R13, offsetof(struct allregs, regs[13]));
	DEFINE(ALLREGS_R14, offsetof(struct allregs, regs[14]));
	DEFINE(ALLREGS_R15, offsetof(struct allregs, regs[15]));
	DEFINE(ALLREGS_R16, offsetof(struct allregs, regs[16]));
	DEFINE(ALLREGS_R17, offsetof(struct allregs, regs[17]));
	DEFINE(ALLREGS_R18, offsetof(struct allregs, regs[18]));
	DEFINE(ALLREGS_R19, offsetof(struct allregs, regs[19]));
	DEFINE(ALLREGS_R20, offsetof(struct allregs, regs[20]));
	DEFINE(ALLREGS_R21, offsetof(struct allregs, regs[21]));
	DEFINE(ALLREGS_R22, offsetof(struct allregs, regs[22]));
	DEFINE(ALLREGS_R23, offsetof(struct allregs, regs[23]));
	DEFINE(ALLREGS_R24, offsetof(struct allregs, regs[24]));
	DEFINE(ALLREGS_R25, offsetof(struct allregs, regs[25]));
	DEFINE(ALLREGS_R26, offsetof(struct allregs, regs[26]));
	DEFINE(ALLREGS_R27, offsetof(struct allregs, regs[27]));
	DEFINE(ALLREGS_R28, offsetof(struct allregs, regs[28]));
	DEFINE(ALLREGS_R29, offsetof(struct allregs, regs[29]));
	DEFINE(ALLREGS_R30, offsetof(struct allregs, regs[30]));
	DEFINE(ALLREGS_R31, offsetof(struct allregs, regs[31]));
	DEFINE(ALLREGS_PS, offsetof(struct allregs, ps));
	DEFINE(ALLREGS_PC, offsetof(struct allregs, pc));
	DEFINE(ALLREGS_GP, offsetof(struct allregs, gp));
	DEFINE(ALLREGS_A0, offsetof(struct allregs, a0));
	DEFINE(ALLREGS_A1, offsetof(struct allregs, a1));
	DEFINE(ALLREGS_A2, offsetof(struct allregs, a2));
	BLANK();

	DEFINE(KVM_REGS_SIZE, sizeof(struct kvm_regs));
	DEFINE(KVM_REGS_R0, offsetof(struct kvm_regs, r0));
	DEFINE(KVM_REGS_R1, offsetof(struct kvm_regs, r1));
	DEFINE(KVM_REGS_R2, offsetof(struct kvm_regs, r2));
	DEFINE(KVM_REGS_R3, offsetof(struct kvm_regs, r3));
	DEFINE(KVM_REGS_R4, offsetof(struct kvm_regs, r4));
	DEFINE(KVM_REGS_R5, offsetof(struct kvm_regs, r5));
	DEFINE(KVM_REGS_R6, offsetof(struct kvm_regs, r6));
	DEFINE(KVM_REGS_R7, offsetof(struct kvm_regs, r7));
	DEFINE(KVM_REGS_R8, offsetof(struct kvm_regs, r8));
	DEFINE(KVM_REGS_R9, offsetof(struct kvm_regs, r9));
	DEFINE(KVM_REGS_R10, offsetof(struct kvm_regs, r10));
	DEFINE(KVM_REGS_R11, offsetof(struct kvm_regs, r11));
	DEFINE(KVM_REGS_R12, offsetof(struct kvm_regs, r12));
	DEFINE(KVM_REGS_R13, offsetof(struct kvm_regs, r13));
	DEFINE(KVM_REGS_R14, offsetof(struct kvm_regs, r14));
	DEFINE(KVM_REGS_R15, offsetof(struct kvm_regs, r15));
	DEFINE(KVM_REGS_R19, offsetof(struct kvm_regs, r19));
	DEFINE(KVM_REGS_R20, offsetof(struct kvm_regs, r20));
	DEFINE(KVM_REGS_R21, offsetof(struct kvm_regs, r21));
	DEFINE(KVM_REGS_R22, offsetof(struct kvm_regs, r22));
	DEFINE(KVM_REGS_R23, offsetof(struct kvm_regs, r23));
	DEFINE(KVM_REGS_R24, offsetof(struct kvm_regs, r24));
	DEFINE(KVM_REGS_R25, offsetof(struct kvm_regs, r25));
	DEFINE(KVM_REGS_R26, offsetof(struct kvm_regs, r26));
	DEFINE(KVM_REGS_R27, offsetof(struct kvm_regs, r27));
	DEFINE(KVM_REGS_R28, offsetof(struct kvm_regs, r28));
	DEFINE(KVM_REGS_FPCR, offsetof(struct kvm_regs, fpcr));
	DEFINE(KVM_REGS_F0, offsetof(struct kvm_regs, fp[0 * 4]));
	DEFINE(KVM_REGS_F1, offsetof(struct kvm_regs, fp[1 * 4]));
	DEFINE(KVM_REGS_F2, offsetof(struct kvm_regs, fp[2 * 4]));
	DEFINE(KVM_REGS_F3, offsetof(struct kvm_regs, fp[3 * 4]));
	DEFINE(KVM_REGS_F4, offsetof(struct kvm_regs, fp[4 * 4]));
	DEFINE(KVM_REGS_F5, offsetof(struct kvm_regs, fp[5 * 4]));
	DEFINE(KVM_REGS_F6, offsetof(struct kvm_regs, fp[6 * 4]));
	DEFINE(KVM_REGS_F7, offsetof(struct kvm_regs, fp[7 * 4]));
	DEFINE(KVM_REGS_F8, offsetof(struct kvm_regs, fp[8 * 4]));
	DEFINE(KVM_REGS_F9, offsetof(struct kvm_regs, fp[9 * 4]));
	DEFINE(KVM_REGS_F10, offsetof(struct kvm_regs, fp[10 * 4]));
	DEFINE(KVM_REGS_F11, offsetof(struct kvm_regs, fp[11 * 4]));
	DEFINE(KVM_REGS_F12, offsetof(struct kvm_regs, fp[12 * 4]));
	DEFINE(KVM_REGS_F13, offsetof(struct kvm_regs, fp[13 * 4]));
	DEFINE(KVM_REGS_F14, offsetof(struct kvm_regs, fp[14 * 4]));
	DEFINE(KVM_REGS_F15, offsetof(struct kvm_regs, fp[15 * 4]));
	DEFINE(KVM_REGS_F16, offsetof(struct kvm_regs, fp[16 * 4]));
	DEFINE(KVM_REGS_F17, offsetof(struct kvm_regs, fp[17 * 4]));
	DEFINE(KVM_REGS_F18, offsetof(struct kvm_regs, fp[18 * 4]));
	DEFINE(KVM_REGS_F19, offsetof(struct kvm_regs, fp[19 * 4]));
	DEFINE(KVM_REGS_F20, offsetof(struct kvm_regs, fp[20 * 4]));
	DEFINE(KVM_REGS_F21, offsetof(struct kvm_regs, fp[21 * 4]));
	DEFINE(KVM_REGS_F22, offsetof(struct kvm_regs, fp[22 * 4]));
	DEFINE(KVM_REGS_F23, offsetof(struct kvm_regs, fp[23 * 4]));
	DEFINE(KVM_REGS_F24, offsetof(struct kvm_regs, fp[24 * 4]));
	DEFINE(KVM_REGS_F25, offsetof(struct kvm_regs, fp[25 * 4]));
	DEFINE(KVM_REGS_F26, offsetof(struct kvm_regs, fp[26 * 4]));
	DEFINE(KVM_REGS_F27, offsetof(struct kvm_regs, fp[27 * 4]));
	DEFINE(KVM_REGS_F28, offsetof(struct kvm_regs, fp[28 * 4]));
	DEFINE(KVM_REGS_F29, offsetof(struct kvm_regs, fp[29 * 4]));
	DEFINE(KVM_REGS_F30, offsetof(struct kvm_regs, fp[30 * 4]));
	DEFINE(KVM_REGS_PS, offsetof(struct kvm_regs, ps));
	DEFINE(KVM_REGS_PC, offsetof(struct kvm_regs, pc));
	DEFINE(KVM_REGS_GP, offsetof(struct kvm_regs, gp));
	DEFINE(KVM_REGS_R16, offsetof(struct kvm_regs, r16));
	DEFINE(KVM_REGS_R17, offsetof(struct kvm_regs, r17));
	DEFINE(KVM_REGS_R18, offsetof(struct kvm_regs, r18));
	BLANK();

	DEFINE(VCPU_RET_SIZE, sizeof(struct vcpu_run_ret_stack));
	DEFINE(VCPU_RET_RA, offsetof(struct vcpu_run_ret_stack, ra));
	DEFINE(VCPU_RET_R0, offsetof(struct vcpu_run_ret_stack, r0));
	BLANK();

	DEFINE(HOST_INT_SIZE, sizeof(struct host_int_args));
	DEFINE(HOST_INT_R18, offsetof(struct host_int_args, r18));
	DEFINE(HOST_INT_R17, offsetof(struct host_int_args, r17));
	DEFINE(HOST_INT_R16, offsetof(struct host_int_args, r16));
	BLANK();

	DEFINE(TASK_THREAD, offsetof(struct task_struct, thread));
	DEFINE(THREAD_CTX_FP, offsetof(struct thread_struct, ctx_fp));
	DEFINE(THREAD_FPCR, offsetof(struct thread_struct, fpcr));
	DEFINE(CTX_FP_F0, offsetof(struct context_fpregs, f0));
	DEFINE(CTX_FP_F1, offsetof(struct context_fpregs, f1));
	DEFINE(CTX_FP_F2, offsetof(struct context_fpregs, f2));
	DEFINE(CTX_FP_F3, offsetof(struct context_fpregs, f3));
	DEFINE(CTX_FP_F4, offsetof(struct context_fpregs, f4));
	DEFINE(CTX_FP_F5, offsetof(struct context_fpregs, f5));
	DEFINE(CTX_FP_F6, offsetof(struct context_fpregs, f6));
	DEFINE(CTX_FP_F7, offsetof(struct context_fpregs, f7));
	DEFINE(CTX_FP_F8, offsetof(struct context_fpregs, f8));
	DEFINE(CTX_FP_F9, offsetof(struct context_fpregs, f9));
	DEFINE(CTX_FP_F10, offsetof(struct context_fpregs, f10));
	DEFINE(CTX_FP_F11, offsetof(struct context_fpregs, f11));
	DEFINE(CTX_FP_F12, offsetof(struct context_fpregs, f12));
	DEFINE(CTX_FP_F13, offsetof(struct context_fpregs, f13));
	DEFINE(CTX_FP_F14, offsetof(struct context_fpregs, f14));
	DEFINE(CTX_FP_F15, offsetof(struct context_fpregs, f15));
	DEFINE(CTX_FP_F16, offsetof(struct context_fpregs, f16));
	DEFINE(CTX_FP_F17, offsetof(struct context_fpregs, f17));
	DEFINE(CTX_FP_F18, offsetof(struct context_fpregs, f18));
	DEFINE(CTX_FP_F19, offsetof(struct context_fpregs, f19));
	DEFINE(CTX_FP_F20, offsetof(struct context_fpregs, f20));
	DEFINE(CTX_FP_F21, offsetof(struct context_fpregs, f21));
	DEFINE(CTX_FP_F22, offsetof(struct context_fpregs, f22));
	DEFINE(CTX_FP_F23, offsetof(struct context_fpregs, f23));
	DEFINE(CTX_FP_F24, offsetof(struct context_fpregs, f24));
	DEFINE(CTX_FP_F25, offsetof(struct context_fpregs, f25));
	DEFINE(CTX_FP_F26, offsetof(struct context_fpregs, f26));
	DEFINE(CTX_FP_F27, offsetof(struct context_fpregs, f27));
	DEFINE(CTX_FP_F28, offsetof(struct context_fpregs, f28));
	DEFINE(CTX_FP_F29, offsetof(struct context_fpregs, f29));
	DEFINE(CTX_FP_F30, offsetof(struct context_fpregs, f30));
	BLANK();
}
