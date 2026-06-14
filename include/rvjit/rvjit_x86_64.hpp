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
#include "../host.hpp"
#include <cstdint>
#ifdef HOST_TARGET_X86_64
#include "rvjit_emit.hpp"

std::unordered_set<uint8_t> spill_regs = {

};

/*
 *	MOD:
 *		11: both registers
 *		00: memory without offset
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
inline uint8_t sib(uint8_t scale, uint8_t index, uint8_t base)
{
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

// SUB r/m64, r64
inline void sub_rr(unsigned char bytes[], uint16_t byte_pos, char source, char dest)
{
	// dest is RM, source is REG
	bytes[byte_pos++] = rex(1, (source > 7), 0, (dest > 7));
	bytes[byte_pos++] = 0x29;
	bytes[byte_pos++] = modrm(3, source, dest);
}
// ADD r/m64, r64
inline void add_rr(unsigned char bytes[], uint16_t byte_pos, char source, char dest)
{
	// dest is RM, source is REG
	bytes[byte_pos++] = rex(1, (source > 7), 0, (dest > 7));
	bytes[byte_pos++] = 0x01;
	bytes[byte_pos++] = modrm(3, source, dest);
}
// PUSH r64
inline void push(unsigned char bytes[], uint16_t byte_pos, char reg)
{
	if(reg > 7)
		bytes[byte_pos++] = rex(0, 0, 0, 1);
	bytes[byte_pos++] = 0x50 + (reg & 7);
	;
}
// POP r64
inline void pop(unsigned char bytes[], uint16_t byte_pos, char reg)
{
	if(reg > 7)
		bytes[byte_pos++] = rex(0, 0, 0, 1);
	bytes[byte_pos++] = 0x58 + (reg & 7);
}
// RET
inline void ret(unsigned char bytes[], uint16_t& byte_pos)
{
	bytes[byte_pos++] = 0xC3;
}

inline void rvjit_emit_prologue(unsigned char bytes[], uint16_t byte_pos)
{
	push(bytes, byte_pos, 4); // push hart context into r12
}
inline void rvjit_emit_epilogue(unsigned char bytes[], uint16_t byte_pos)
{
	pop(bytes, byte_pos, 4); // pop hart context from r12
	ret(bytes, byte_pos);
}

#endif
#endif
