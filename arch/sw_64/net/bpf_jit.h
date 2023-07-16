/* SPDX-License-Identifier: GPL-2.0 */
/*
 * BPF JIT compiler for SW64
 *
 * Copyright (C) Mao Minkai
 * Author: Mao Minkai
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

#ifndef _SW64_BPF_JIT_H
#define _SW64_BPF_JIT_H

#define SW64_BPF_OPCODE_OFFSET		26
#define SW64_BPF_RA_OFFSET		21
#define SW64_BPF_RB_OFFSET		16
#define SW64_BPF_SIMPLE_ALU_IMM_OFFSET	13
#define SW64_BPF_SIMPLE_ALU_FUNC_OFFSET	5
#define SW64_BPF_SIMPLE_ALU_RC_OFFSET	0

#define SW64_BPF_OPCODE_BR_CALL		0x01
#define SW64_BPF_OPCODE_BR_RET		0x02
#define SW64_BPF_OPCODE_BR_JMP		0x03
#define SW64_BPF_OPCODE_BR_BR		0x04
#define SW64_BPF_OPCODE_BR_BSR		0x05
#define SW64_BPF_OPCODE_BR_BEQ		0x30
#define SW64_BPF_OPCODE_BR_BNE		0x31
#define SW64_BPF_OPCODE_BR_BLT		0x32
#define SW64_BPF_OPCODE_BR_BLE		0x33
#define SW64_BPF_OPCODE_BR_BGT		0x34
#define SW64_BPF_OPCODE_BR_BGE		0x35
#define SW64_BPF_OPCODE_BR_BLBC		0x36
#define SW64_BPF_OPCODE_BR_BLBS		0x37

#define SW64_BPF_OPCODE_LS_LDBU		0x20
#define SW64_BPF_OPCODE_LS_LDHU		0x21
#define SW64_BPF_OPCODE_LS_LDW		0x22
#define SW64_BPF_OPCODE_LS_LDL		0x23
#define SW64_BPF_OPCODE_LS_STB		0x28
#define SW64_BPF_OPCODE_LS_STH		0x29
#define SW64_BPF_OPCODE_LS_STW		0x2A
#define SW64_BPF_OPCODE_LS_STL		0x2B
#define SW64_BPF_OPCODE_LS_LDI		0x3E
#define SW64_BPF_OPCODE_LS_LDIH		0x3F

#define SW64_BPF_OPCODE_ALU_REG		0x10
#define SW64_BPF_OPCODE_ALU_IMM		0x12

#define SW64_BPF_FUNC_ALU_ADDW		0x00
#define SW64_BPF_FUNC_ALU_SUBW		0x01
#define SW64_BPF_FUNC_ALU_ADDL		0x08
#define SW64_BPF_FUNC_ALU_SUBL		0x09
#define SW64_BPF_FUNC_ALU_MULW		0x10
#define SW64_BPF_FUNC_ALU_MULL		0x18
#define SW64_BPF_FUNC_ALU_ZAP		0x68
#define SW64_BPF_FUNC_ALU_ZAPNOT	0x69
#define SW64_BPF_FUNC_ALU_SEXTB		0x6A
#define SW64_BPF_FUNC_ALU_SEXTH		0x6B

#define SW64_BPF_OPCODE_BS_REG		0x10
#define SW64_BPF_OPCODE_BS_IMM		0x12

#define SW64_BPF_FUNC_BS_SLL		0x48
#define SW64_BPF_FUNC_BS_SRL		0x49
#define SW64_BPF_FUNC_BS_SRA		0x4A

#define SW64_BPF_OPCODE_LOGIC_REG	0x10
#define SW64_BPF_OPCODE_LOGIC_IMM	0x12

#define SW64_BPF_FUNC_LOGIC_AND		0x38
#define SW64_BPF_FUNC_LOGIC_BIC		0x39
#define SW64_BPF_FUNC_LOGIC_BIS		0x3A
#define SW64_BPF_FUNC_LOGIC_ORNOT	0x3B
#define SW64_BPF_FUNC_LOGIC_XOR		0x3C
#define SW64_BPF_FUNC_LOGIC_EQV		0x3D

#define SW64_BPF_OPCODE_CMP_REG		0x10
#define SW64_BPF_OPCODE_CMP_IMM		0x12

#define SW64_BPF_FUNC_CMP_EQ		0x28
#define SW64_BPF_FUNC_CMP_LT		0x29
#define SW64_BPF_FUNC_CMP_LE		0x2A
#define SW64_BPF_FUNC_CMP_ULT		0x2B
#define SW64_BPF_FUNC_CMP_ULE		0x2C

/* special instuction used in jit_fill_hole() */
#define SW64_BPF_ILLEGAL_INSN	((1 << 25) | 0x80)

enum sw64_bpf_registers {
	SW64_BPF_REG_V0		= 0,	/* keep return value */
	SW64_BPF_REG_T0		= 1,
	SW64_BPF_REG_T1		= 2,
	SW64_BPF_REG_T2		= 3,
	SW64_BPF_REG_T3		= 4,
	SW64_BPF_REG_T4		= 5,
	SW64_BPF_REG_T5		= 6,
	SW64_BPF_REG_T6		= 7,
	SW64_BPF_REG_T7		= 8,
	SW64_BPF_REG_S0		= 9,	/* callee saved */
	SW64_BPF_REG_S1		= 10,	/* callee saved */
	SW64_BPF_REG_S2		= 11,	/* callee saved */
	SW64_BPF_REG_S3		= 12,	/* callee saved */
	SW64_BPF_REG_S4		= 13,	/* callee saved */
	SW64_BPF_REG_S5		= 14,	/* callee saved */
	SW64_BPF_REG_S6		= 15,	/* callee saved */
	SW64_BPF_REG_FP		= 15,	/* frame pointer if necessary */
	SW64_BPF_REG_A0		= 16,	/* argument 0 */
	SW64_BPF_REG_A1		= 17,	/* argument 1 */
	SW64_BPF_REG_A2		= 18,	/* argument 2 */
	SW64_BPF_REG_A3		= 19,	/* argument 3 */
	SW64_BPF_REG_A4		= 20,	/* argument 4 */
	SW64_BPF_REG_A5		= 21,	/* argument 5 */
	SW64_BPF_REG_T8		= 22,
	SW64_BPF_REG_T9		= 23,
	SW64_BPF_REG_T10	= 24,
	SW64_BPF_REG_T11	= 25,
	SW64_BPF_REG_RA		= 26,	/* callee saved, keep retuen address */
	SW64_BPF_REG_T12	= 27,
	SW64_BPF_REG_PV		= 27,
	SW64_BPF_REG_AT		= 28,	/* reserved by assembler */
	SW64_BPF_REG_GP		= 29,	/* global pointer */
	SW64_BPF_REG_SP		= 30,	/* callee saved, stack pointer */
	SW64_BPF_REG_ZR		= 31	/* read 0 */
};

/* SW64 load and store instructions */
#define SW64_BPF_LDBU(dst, rb, offset16) \
	sw64_bpf_gen_format_ls(SW64_BPF_OPCODE_LS_LDBU, dst, rb, offset16)
#define SW64_BPF_LDHU(dst, rb, offset16) \
	sw64_bpf_gen_format_ls(SW64_BPF_OPCODE_LS_LDHU, dst, rb, offset16)
#define SW64_BPF_LDW(dst, rb, offset16) \
	sw64_bpf_gen_format_ls(SW64_BPF_OPCODE_LS_LDW, dst, rb, offset16)
#define SW64_BPF_LDL(dst, rb, offset16) \
	sw64_bpf_gen_format_ls(SW64_BPF_OPCODE_LS_LDL, dst, rb, offset16)
#define SW64_BPF_STB(src, rb, offset16) \
	sw64_bpf_gen_format_ls(SW64_BPF_OPCODE_LS_STB, src, rb, offset16)
#define SW64_BPF_STH(src, rb, offset16) \
	sw64_bpf_gen_format_ls(SW64_BPF_OPCODE_LS_STH, src, rb, offset16)
#define SW64_BPF_STW(src, rb, offset16) \
	sw64_bpf_gen_format_ls(SW64_BPF_OPCODE_LS_STW, src, rb, offset16)
#define SW64_BPF_STL(src, rb, offset16) \
	sw64_bpf_gen_format_ls(SW64_BPF_OPCODE_LS_STL, src, rb, offset16)
#define SW64_BPF_LDI(dst, rb, imm16) \
	sw64_bpf_gen_format_ls(SW64_BPF_OPCODE_LS_LDI, dst, rb, imm16)
#define SW64_BPF_LDIH(dst, rb, imm16) \
	sw64_bpf_gen_format_ls(SW64_BPF_OPCODE_LS_LDIH, dst, rb, imm16)

/* SW64 ALU instructions REG format */
#define SW64_BPF_ADDW_REG(ra, rb, dst) \
	sw64_bpf_gen_format_simple_alu_reg(SW64_BPF_OPCODE_ALU_REG, \
			ra, rb, dst, SW64_BPF_FUNC_ALU_ADDW)
#define SW64_BPF_ADDL_REG(ra, rb, dst) \
	sw64_bpf_gen_format_simple_alu_reg(SW64_BPF_OPCODE_ALU_REG, \
			ra, rb, dst, SW64_BPF_FUNC_ALU_ADDL)
#define SW64_BPF_SUBW_REG(ra, rb, dst) \
	sw64_bpf_gen_format_simple_alu_reg(SW64_BPF_OPCODE_ALU_REG, \
			ra, rb, dst, SW64_BPF_FUNC_ALU_SUBW)
#define SW64_BPF_SUBL_REG(ra, rb, dst) \
	sw64_bpf_gen_format_simple_alu_reg(SW64_BPF_OPCODE_ALU_REG, \
			ra, rb, dst, SW64_BPF_FUNC_ALU_SUBL)
#define SW64_BPF_MULW_REG(ra, rb, dst) \
	sw64_bpf_gen_format_simple_alu_reg(SW64_BPF_OPCODE_ALU_REG, \
			ra, rb, dst, SW64_BPF_FUNC_ALU_MULW)
#define SW64_BPF_MULL_REG(ra, rb, dst) \
	sw64_bpf_gen_format_simple_alu_reg(SW64_BPF_OPCODE_ALU_REG, \
			ra, rb, dst, SW64_BPF_FUNC_ALU_MULL)
#define SW64_BPF_ZAP_REG(ra, rb, dst) \
	sw64_bpf_gen_format_simple_alu_reg(SW64_BPF_OPCODE_ALU_REG, \
			ra, rb, dst, SW64_BPF_FUNC_ALU_ZAP)
#define SW64_BPF_ZAPNOT_REG(ra, rb, dst) \
	sw64_bpf_gen_format_simple_alu_reg(SW64_BPF_OPCODE_ALU_REG, \
			ra, rb, dst, SW64_BPF_FUNC_ALU_ZAPNOT)
#define SW64_BPF_SEXTB_REG(rb, dst) \
	sw64_bpf_gen_format_simple_alu_reg(SW64_BPF_OPCODE_ALU_REG, \
			0, rb, dst, SW64_BPF_FUNC_ALU_SEXTB)
#define SW64_BPF_SEXTH_REG(rb, dst) \
	sw64_bpf_gen_format_simple_alu_reg(SW64_BPF_OPCODE_ALU_REG, \
			0, rb, dst, SW64_BPF_FUNC_ALU_SEXTH)

/* SW64 ALU instructions IMM format */
#define SW64_BPF_ADDW_IMM(ra, imm8, dst) \
	sw64_bpf_gen_format_simple_alu_imm(SW64_BPF_OPCODE_ALU_IMM, \
			ra, imm8, dst, SW64_BPF_FUNC_ALU_ADDW)
#define SW64_BPF_ADDL_IMM(ra, imm8, dst) \
	sw64_bpf_gen_format_simple_alu_imm(SW64_BPF_OPCODE_ALU_IMM, \
			ra, imm8, dst, SW64_BPF_FUNC_ALU_ADDL)
#define SW64_BPF_SUBW_IMM(ra, imm8, dst) \
	sw64_bpf_gen_format_simple_alu_imm(SW64_BPF_OPCODE_ALU_IMM, \
			ra, imm8, dst, SW64_BPF_FUNC_ALU_SUBW)
#define SW64_BPF_SUBL_IMM(ra, imm8, dst) \
	sw64_bpf_gen_format_simple_alu_imm(SW64_BPF_OPCODE_ALU_IMM, \
			ra, imm8, dst, SW64_BPF_FUNC_ALU_SUBL)
#define SW64_BPF_MULW_IMM(ra, imm8, dst) \
	sw64_bpf_gen_format_simple_alu_imm(SW64_BPF_OPCODE_ALU_IMM, \
			ra, imm8, dst, SW64_BPF_FUNC_ALU_MULW)
#define SW64_BPF_MULL_IMM(ra, imm8, dst) \
	sw64_bpf_gen_format_simple_alu_imm(SW64_BPF_OPCODE_ALU_IMM, \
			ra, imm8, dst, SW64_BPF_FUNC_ALU_MULL)
#define SW64_BPF_ZAP_IMM(ra, imm8, dst) \
	sw64_bpf_gen_format_simple_alu_imm(SW64_BPF_OPCODE_ALU_IMM, \
			ra, imm8, dst, SW64_BPF_FUNC_ALU_ZAP)
#define SW64_BPF_ZAPNOT_IMM(ra, imm8, dst) \
	sw64_bpf_gen_format_simple_alu_imm(SW64_BPF_OPCODE_ALU_IMM, \
			ra, imm8, dst, SW64_BPF_FUNC_ALU_ZAPNOT)
#define SW64_BPF_SEXTB_IMM(imm8, dst) \
	sw64_bpf_gen_format_simple_alu_imm(SW64_BPF_OPCODE_ALU_IMM, \
			0, imm8, dst, SW64_BPF_FUNC_ALU_SEXTB)

/* SW64 bit shift instructions REG format */
#define SW64_BPF_SLL_REG(src, rb, dst) \
	sw64_bpf_gen_format_simple_alu_reg(SW64_BPF_OPCODE_BS_REG, \
			src, rb, dst, SW64_BPF_FUNC_BS_SLL)
#define SW64_BPF_SRL_REG(src, rb, dst) \
	sw64_bpf_gen_format_simple_alu_reg(SW64_BPF_OPCODE_BS_REG, \
			src, rb, dst, SW64_BPF_FUNC_BS_SRL)
#define SW64_BPF_SRA_REG(src, rb, dst) \
	sw64_bpf_gen_format_simple_alu_reg(SW64_BPF_OPCODE_BS_REG, \
			src, rb, dst, SW64_BPF_FUNC_BS_SRA)

/* SW64 bit shift instructions IMM format */
#define SW64_BPF_SLL_IMM(src, imm8, dst) \
	sw64_bpf_gen_format_simple_alu_imm(SW64_BPF_OPCODE_BS_IMM, \
			src, imm8, dst, SW64_BPF_FUNC_BS_SLL)
#define SW64_BPF_SRL_IMM(src, imm8, dst) \
	sw64_bpf_gen_format_simple_alu_imm(SW64_BPF_OPCODE_BS_IMM, \
			src, imm8, dst, SW64_BPF_FUNC_BS_SRL)
#define SW64_BPF_SRA_IMM(src, imm8, dst) \
	sw64_bpf_gen_format_simple_alu_imm(SW64_BPF_OPCODE_BS_IMM, \
			src, imm8, dst, SW64_BPF_FUNC_BS_SRA)

/* SW64 control instructions */
#define SW64_BPF_CALL(ra, rb) \
	sw64_bpf_gen_format_ls(SW64_BPF_OPCODE_BR_CALL, ra, rb, 0)
#define SW64_BPF_RET(rb) \
	sw64_bpf_gen_format_ls(SW64_BPF_OPCODE_BR_RET, SW64_BPF_REG_ZR, rb, 0)
#define SW64_BPF_JMP(ra, rb) \
	sw64_bpf_gen_format_ls(SW64_BPF_OPCODE_BR_JMP, ra, rb, 0)
#define SW64_BPF_BR(ra, offset) \
	sw64_bpf_gen_format_br(SW64_BPF_OPCODE_BR_BR, ra, offset)
#define SW64_BPF_BSR(ra, offset) \
	sw64_bpf_gen_format_br(SW64_BPF_OPCODE_BR_BSR, ra, offset)
#define SW64_BPF_BEQ(ra, offset) \
	sw64_bpf_gen_format_br(SW64_BPF_OPCODE_BR_BEQ, ra, offset)
#define SW64_BPF_BNE(ra, offset) \
	sw64_bpf_gen_format_br(SW64_BPF_OPCODE_BR_BNE, ra, offset)
#define SW64_BPF_BLT(ra, offset) \
	sw64_bpf_gen_format_br(SW64_BPF_OPCODE_BR_BLT, ra, offset)
#define SW64_BPF_BLE(ra, offset) \
	sw64_bpf_gen_format_br(SW64_BPF_OPCODE_BR_BLE, ra, offset)
#define SW64_BPF_BGT(ra, offset) \
	sw64_bpf_gen_format_br(SW64_BPF_OPCODE_BR_BGT, ra, offset)
#define SW64_BPF_BGE(ra, offset) \
	sw64_bpf_gen_format_br(SW64_BPF_OPCODE_BR_BGE, ra, offset)
#define SW64_BPF_BLBC(ra, offset) \
	sw64_bpf_gen_format_br(SW64_BPF_OPCODE_BR_BLBC, ra, offset)
#define SW64_BPF_BLBS(ra, offset) \
	sw64_bpf_gen_format_br(SW64_BPF_OPCODE_BR_BLBS, ra, offset)

/* SW64 bit logic instructions REG format */
#define SW64_BPF_AND_REG(ra, rb, dst) \
	sw64_bpf_gen_format_simple_alu_reg(SW64_BPF_OPCODE_LOGIC_REG, \
			ra, rb, dst, SW64_BPF_FUNC_LOGIC_AND)
#define SW64_BPF_ANDNOT_REG(ra, rb, dst) \
	sw64_bpf_gen_format_simple_alu_reg(SW64_BPF_OPCODE_LOGIC_REG, \
			ra, rb, dst, SW64_BPF_FUNC_LOGIC_BIC)
#define SW64_BPF_OR_REG(ra, rb, dst) \
	sw64_bpf_gen_format_simple_alu_reg(SW64_BPF_OPCODE_LOGIC_REG, \
			ra, rb, dst, SW64_BPF_FUNC_LOGIC_BIS)
#define SW64_BPF_ORNOT_REG(ra, rb, dst) \
	sw64_bpf_gen_format_simple_alu_reg(SW64_BPF_OPCODE_LOGIC_REG, \
			ra, rb, dst, SW64_BPF_FUNC_LOGIC_ORNOT)
#define SW64_BPF_XOR_REG(ra, rb, dst) \
	sw64_bpf_gen_format_simple_alu_reg(SW64_BPF_OPCODE_LOGIC_REG, \
			ra, rb, dst, SW64_BPF_FUNC_LOGIC_XOR)
#define SW64_BPF_EQV_REG(ra, rb, dst) \
	sw64_bpf_gen_format_simple_alu_reg(SW64_BPF_OPCODE_LOGIC_REG, \
			ra, rb, dst, SW64_BPF_FUNC_LOGIC_EQV)

/* SW64 bit logic instructions IMM format */
#define SW64_BPF_AND_IMM(ra, imm8, dst) \
	sw64_bpf_gen_format_simple_alu_imm(SW64_BPF_OPCODE_LOGIC_IMM, \
			ra, imm8, dst, SW64_BPF_FUNC_LOGIC_AND)
#define SW64_BPF_ANDNOT_IMM(ra, imm8, dst) \
	sw64_bpf_gen_format_simple_alu_imm(SW64_BPF_OPCODE_LOGIC_IMM, \
			ra, imm8, dst, SW64_BPF_FUNC_LOGIC_BIC)
#define SW64_BPF_OR_IMM(ra, imm8, dst) \
	sw64_bpf_gen_format_simple_alu_imm(SW64_BPF_OPCODE_LOGIC_IMM, \
			ra, imm8, dst, SW64_BPF_FUNC_LOGIC_BIS)
#define SW64_BPF_ORNOT_IMM(ra, imm8, dst) \
	sw64_bpf_gen_format_simple_alu_imm(SW64_BPF_OPCODE_LOGIC_IMM, \
			ra, imm8, dst, SW64_BPF_FUNC_LOGIC_ORNOT)
#define SW64_BPF_XOR_IMM(ra, imm8, dst) \
	sw64_bpf_gen_format_simple_alu_imm(SW64_BPF_OPCODE_LOGIC_IMM, \
			ra, imm8, dst, SW64_BPF_FUNC_LOGIC_XOR)
#define SW64_BPF_EQV_IMM(ra, imm8, dst) \
	sw64_bpf_gen_format_simple_alu_imm(SW64_BPF_OPCODE_LOGIC_IMM, \
			ra, imm8, dst, SW64_BPF_FUNC_LOGIC_EQV)

/* SW64 compare instructions REG format */
#define SW64_BPF_CMPEQ_REG(ra, rb, dst) \
	sw64_bpf_gen_format_simple_alu_reg(SW64_BPF_OPCODE_CMP_REG, \
			ra, rb, dst, SW64_BPF_FUNC_CMP_EQ)
#define SW64_BPF_CMPLT_REG(ra, rb, dst) \
	sw64_bpf_gen_format_simple_alu_reg(SW64_BPF_OPCODE_CMP_REG, \
			ra, rb, dst, SW64_BPF_FUNC_CMP_LT)
#define SW64_BPF_CMPLE_REG(ra, rb, dst) \
	sw64_bpf_gen_format_simple_alu_reg(SW64_BPF_OPCODE_CMP_REG, \
			ra, rb, dst, SW64_BPF_FUNC_CMP_LE)
#define SW64_BPF_CMPULT_REG(ra, rb, dst) \
	sw64_bpf_gen_format_simple_alu_reg(SW64_BPF_OPCODE_CMP_REG, \
			ra, rb, dst, SW64_BPF_FUNC_CMP_ULT)
#define SW64_BPF_CMPULE_REG(ra, rb, dst) \
	sw64_bpf_gen_format_simple_alu_reg(SW64_BPF_OPCODE_CMP_REG, \
			ra, rb, dst, SW64_BPF_FUNC_CMP_ULE)

/* SW64 compare instructions imm format */
#define SW64_BPF_CMPEQ_IMM(ra, imm8, dst) \
	sw64_bpf_gen_format_simple_alu_imm(SW64_BPF_OPCODE_CMP_IMM, \
			ra, imm8, dst, SW64_BPF_FUNC_CMP_EQ)
#define SW64_BPF_CMPLT_IMM(ra, imm8, dst) \
	sw64_bpf_gen_format_simple_alu_imm(SW64_BPF_OPCODE_CMP_IMM, \
			ra, imm8, dst, SW64_BPF_FUNC_CMP_LT)
#define SW64_BPF_CMPLE_IMM(ra, imm8, dst) \
	sw64_bpf_gen_format_simple_alu_imm(SW64_BPF_OPCODE_CMP_IMM, \
			ra, imm8, dst, SW64_BPF_FUNC_CMP_LE)
#define SW64_BPF_CMPULT_IMM(ra, imm8, dst) \
	sw64_bpf_gen_format_simple_alu_imm(SW64_BPF_OPCODE_CMP_IMM, \
			ra, imm8, dst, SW64_BPF_FUNC_CMP_ULT)
#define SW64_BPF_CMPULE_IMM(ra, imm8, dst) \
	sw64_bpf_gen_format_simple_alu_imm(SW64_BPF_OPCODE_CMP_IMM, \
			ra, imm8, dst, SW64_BPF_FUNC_CMP_ULE)

#endif /* _SW64_BPF_JIT_H */
