/*
* Copyright 2026 Spalishe

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

	   http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

*/
#pragma once
#ifdef USE_JIT
#include "rvjit_emit.hpp"
#include <cstdint>
#ifdef HOST_TARGET_X86_64

struct JIT_Emitter;

constexpr uint8_t REG_RAX = 0b0000;
constexpr uint8_t REG_RCX = 0b0001;
constexpr uint8_t REG_RDX = 0b0010;
constexpr uint8_t REG_RBX = 0b0011;
constexpr uint8_t REG_RSP = 0b0100;
constexpr uint8_t REG_RBP = 0b0101;
constexpr uint8_t REG_RSI = 0b0110;
constexpr uint8_t REG_RDI = 0b0111;
constexpr uint8_t REG_R8  = 0b0000;
constexpr uint8_t REG_R9  = 0b0001;
constexpr uint8_t REG_R10 = 0b1010;
constexpr uint8_t REG_R11 = 0b1011;
constexpr uint8_t REG_R12 = 0b1100;
constexpr uint8_t REG_R13 = 0b1101;
constexpr uint8_t REG_R14 = 0b1110;
constexpr uint8_t REG_R15 = 0b1111;

/*
 *	MOD:
 *		11: both registers
 *		00: memory without offset ([reg] or [base+index*scale])
 *		01: memory + disp8
 *		10: memory + disp32
 *	REG & RM: (REG is like rs1, RM is rd)
 *		000: RAX   R8
 *		001: RCX   R9
 *		010: RDX   R10
 *		011: RBX   R11
 *		100: RSP   R12
 *		101: RBP   R13
 *		110: RSI   R14
 *		111: RDI   R15
 */
inline uint8_t
modrm(uint8_t mod, uint8_t reg, uint8_t rm)
{
	return (mod << 6) | ((reg & 7) << 3) | (rm & 7);
}
/*
 *	Scale is basically multiplier
 *		00: *1
 *		01: *2
 *		10: *4
 *		11: *8
 *	Index and base are same registers
 */
inline uint8_t sib(char scale, char index, char base)
{
	if(index == -1)
		index = 4; // no index
	return (scale << 6) | ((index & 7) << 3) | (base & 7);
}

/*
 *	W: Operand size
 *		0 - default
 *		1 - 64-bit
 *	R: Extends REG field in ModRM
 *		allows to use r8-r15
 *	X: Extends index register in SIB
 *	B: Extends RM field in ModRM, or BASE field in SIB
 */
inline uint8_t rex(bool W, bool R, bool X, bool B)
{
	return (0b0100 << 4) | (W << 3) | (R << 2) | (X << 1) | B;
}

#define NO_INDEX 0xFF
inline bool needs_sib(uint8_t base, uint8_t index)
{
	return index != 0xFF || (base & 7) == 4; // rsp/r12
}

// SUB r/m64, r64
inline void sub_rr(JIT_Block& blk, char dest, char source)
{
	// dest is RM, source is REG
	blk.bytes[blk.byte_pos++] = rex(1, (source > 7), 0, (dest > 7));
	blk.bytes[blk.byte_pos++] = 0x29;
	blk.bytes[blk.byte_pos++] = modrm(3, (source & 7), (dest & 7));
}
// SUB r/m64, imm32
inline void sub_riw(JIT_Block& blk, char dest, int32_t imm32)
{
	// dest is RM, REG must be 0b101
	blk.bytes[blk.byte_pos++] = rex(1, 0, 0, (dest > 7));
	blk.bytes[blk.byte_pos++] = 0x81;
	blk.bytes[blk.byte_pos++] = modrm(3, 0b101, (dest & 7));
	blk.bytes[blk.byte_pos++] = imm32 & 0xFF;
	blk.bytes[blk.byte_pos++] = (imm32 >> 8) & 0xFF;
	blk.bytes[blk.byte_pos++] = (imm32 >> 16) & 0xFF;
	blk.bytes[blk.byte_pos++] = (imm32 >> 24) & 0xFF;
}
// ADD r/m64, r64
inline void add_rr(JIT_Block& blk, char dest, char source)
{
	// dest is RM, source is REG
	blk.bytes[blk.byte_pos++] = rex(1, (source > 7), 0, (dest > 7));
	blk.bytes[blk.byte_pos++] = 0x01;
	blk.bytes[blk.byte_pos++] = modrm(3, (source & 7), (dest & 7));
}
// ADD r/m64, imm32
inline void add_riw(JIT_Block& blk, char dest, int32_t imm32)
{
	// dest is RM, REG must be 0
	blk.bytes[blk.byte_pos++] = rex(1, 0, 0, (dest > 7));
	blk.bytes[blk.byte_pos++] = 0x81;
	blk.bytes[blk.byte_pos++] = modrm(3, 0, (dest & 7));
	blk.bytes[blk.byte_pos++] = imm32 & 0xFF;
	blk.bytes[blk.byte_pos++] = (imm32 >> 8) & 0xFF;
	blk.bytes[blk.byte_pos++] = (imm32 >> 16) & 0xFF;
	blk.bytes[blk.byte_pos++] = (imm32 >> 24) & 0xFF;
}
// XOR r/m64, r64
inline void xor_rr(JIT_Block& blk, char dest, char source)
{
	// dest is RM, source is REG
	blk.bytes[blk.byte_pos++] = rex(1, (source > 7), 0, (dest > 7));
	blk.bytes[blk.byte_pos++] = 0x31;
	blk.bytes[blk.byte_pos++] = modrm(3, (source & 7), (dest & 7));
}
// MOV r/m64, r64
inline void mov(JIT_Block& blk, char dest, char source)
{
	// dest is RM, source is REG
	blk.bytes[blk.byte_pos++] = rex(1, (source > 7), 0, (dest > 7));
	blk.bytes[blk.byte_pos++] = 0x89;
	blk.bytes[blk.byte_pos++] = modrm(3, (source & 7), (dest & 7));
}
// MOV helper
inline void mov_helper(JIT_Block& blk, uint8_t reg, uint8_t base, uint8_t index, uint8_t scale, int32_t disp)
{
	bool has_index = index != 0xFF;
	bool need_sib  = needs_sib(base, index);
	uint8_t mod;
	if(disp == 0 && (base & 7) != 5)
		mod = 0;
	else if(disp >= -128 && disp <= 127)
		mod = 1;
	else
		mod = 2;
	blk.bytes[blk.byte_pos++] = modrm(mod, reg & 7, need_sib ? 4 : (base & 7));

	if(need_sib)
	{
		blk.bytes[blk.byte_pos++] = sib(scale, has_index ? (index & 7) : 4, base & 7);
	}

	if(mod == 1)
	{
		blk.bytes[blk.byte_pos++] = (uint8_t)disp;
	}
	else if(mod == 2)
	{
		blk.bytes[blk.byte_pos++] = disp & 0xFF;
		blk.bytes[blk.byte_pos++] = (disp >> 8) & 0xFF;
		blk.bytes[blk.byte_pos++] = (disp >> 16) & 0xFF;
		blk.bytes[blk.byte_pos++] = (disp >> 24) & 0xFF;
	}
}
// MOV memory,r64
inline void mov_mr(JIT_Block& blk, uint8_t source, uint8_t reg_base, uint8_t reg_index, uint8_t scale, int32_t disp = 0)
{
	// dest is RM, source is REG
	blk.bytes[blk.byte_pos++] = rex(1, (source > 7), (reg_index != 0xFF && reg_index > 7), (reg_base > 7));
	blk.bytes[blk.byte_pos++] = 0x89;
	mov_helper(blk, source, reg_base, reg_index, scale, disp);
}
// MOV r64,memory
inline void mov_rm(JIT_Block& blk, uint8_t dest, uint8_t reg_base, uint8_t reg_index, uint8_t scale, int32_t disp = 0)
{
	// dest is RM, source is REG
	blk.bytes[blk.byte_pos++] = rex(1, (dest > 7), (reg_index != 0xFF && reg_index > 7), (reg_base > 7));
	blk.bytes[blk.byte_pos++] = 0x8B;
	mov_helper(blk, dest, reg_base, reg_index, scale, disp);
}

// PUSH r64
inline void push(JIT_Block& blk, char reg)
{
	if(reg > 7)
		blk.bytes[blk.byte_pos++] = rex(0, 0, 0, 1);
	blk.bytes[blk.byte_pos++] = 0x50 + (reg & 7);
	;
}
// POP r64
inline void pop(JIT_Block& blk, char reg)
{
	if(reg > 7)
		blk.bytes[blk.byte_pos++] = rex(0, 0, 0, 1);
	blk.bytes[blk.byte_pos++] = 0x58 + (reg & 7);
}
// RET
inline void ret(JIT_Block& blk)
{
	blk.bytes[blk.byte_pos++] = 0xC3;
}

inline void JIT_Emitter::rvjit_emit_prologue(JIT_Block& blk)
{
	push(blk, 4);
	mov(blk, REG_RDI, REG_R12);
}
inline void JIT_Emitter::rvjit_emit_epilogue(JIT_Block& blk)
{
	pop(blk, 4); // pop hart context from r12
	ret(blk);
}
inline void JIT_Emitter::ensure_loaded(JIT_Block& blk, VReg& vreg)
{
	if(vreg.allocated && !vreg.valid)
	{
		mov_rm(blk, vreg.host_reg, REG_R12, NO_INDEX, 0, vreg.vreg * 8);
		vreg.valid = true;
	}
}
inline HReg* JIT_Emitter::spill(JIT_Block& blk, uint64_t locked)
{
	// Gonna spill random one
	HReg* victim = nullptr;

	for(auto& reg : host_regs)
	{
		if(locked & (1ULL << reg.host_reg))
			continue;
		if(victim == nullptr)
			victim = &reg;

		if(reg.last_use < victim->last_use)
			victim = &reg;
	}
	victim->used = false;
	VReg& vreg	 = vregs[victim->vreg];
	if(vreg.dirty)
		mov_mr(blk, victim->host_reg, REG_R12, NO_INDEX, 0, victim->vreg * 8);
	vreg.allocated = false;
	vreg.valid	   = false;
	vreg.dirty	   = false;
	return victim;
}
inline VReg& JIT_Emitter::rvjit_alloc_reg(JIT_Block& blk, uint8_t user_reg, uint64_t locked)
{
	// Check if requested user_reg == x0
	if(user_reg == 0)
	{
		return vregs[0];
	}
	// Check if there's already allocated
	if(vregs[user_reg].allocated)
	{
		global_use_counter++;
		host_regs[vregs[user_reg].host_reg].last_use = global_use_counter;
		ensure_loaded(blk, vregs[user_reg]);
		return vregs[user_reg];
	}

	// Find some free host register
	for(auto& hreg : host_regs)
	{
		if(locked & (1u << hreg.host_reg))
			continue;
		if(!hreg.used)
		{
			hreg.used	   = true;
			hreg.vreg	   = user_reg;
			auto& vreg	   = vregs[user_reg];
			vreg.host_reg  = hreg.host_reg;
			vreg.allocated = true;
			ensure_loaded(blk, vreg);
			return vreg;
		}
	}

	// Seems we dont have any free host reg
	HReg* victim	   = spill(blk, locked);
	victim->vreg	   = user_reg;
	victim->used	   = true;
	VReg& new_vreg	   = vregs[user_reg];
	new_vreg.valid	   = false;
	new_vreg.allocated = true;
	new_vreg.host_reg  = victim->host_reg;
	ensure_loaded(blk, new_vreg);
	return new_vreg;
}

// Instruction emit part
inline void JIT_Emitter::inst_emit_r_type(Hart& h, InstructionData& inst, JIT_Block& blk, bool optimize_if_rsz, ROpFunction emit_op)
{
	if(inst.rd == 0)
	{
		// x0 write ignored
		return;
	}
	bool rs1_zero = optimize_if_rsz ? (inst.rs1 == 0) : false;
	bool rs2_zero = optimize_if_rsz ? (inst.rs2 == 0) : false;

	VReg& rd = rvjit_alloc_reg(blk, inst.rd, 0);
	if(rs1_zero && rs2_zero)
	{
		xor_rr(blk, rd.host_reg, rd.host_reg);
		rd.dirty = true;
		return;
	}

	if(rs1_zero)
	{
		VReg& rs2 = rvjit_alloc_reg(blk, inst.rs2, (1ULL << rd.host_reg));
		mov(blk, rd.host_reg, rs2.host_reg);
		rd.dirty = true;
		return;
	}

	if(rs2_zero)
	{
		VReg& rs1 = rvjit_alloc_reg(blk, inst.rs1, (1ULL << rd.host_reg));
		mov(blk, rd.host_reg, rs1.host_reg);
		rd.dirty = true;
		return;
	}
	VReg& rs1 = rvjit_alloc_reg(
		blk,
		inst.rs1,
		(1ULL << rd.host_reg));

	VReg& rs2 = rvjit_alloc_reg(
		blk,
		inst.rs2,
		(1ULL << rs1.host_reg) | (1ULL << rd.host_reg));
	emit_op(blk, rd, rs1, rs2);

	rd.dirty = true;
}

#endif
#endif
