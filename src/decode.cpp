/*
Copyright 2026 Spalishe

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

#include "../include/decode.hpp"
#include <assert.h>
#include <bitset>
#include <cstdio>

int32_t sext(uint32_t val, int bits)
{
	int32_t shift = 32 - bits;
	return (int32_t)(val << shift) >> shift;
}
uint32_t get_bits(uint32_t inst, int hi, int lo)
{
	return (inst >> lo) & ((1u << (hi - lo + 1)) - 1);
}
uint64_t d_rd(uint32_t inst)
{
	return (inst >> 7) & 0x1f; // rd in bits 11..7
}
uint64_t d_rs1(uint32_t inst)
{
	return (inst >> 15) & 0x1f; // rs1 in bits 19..15
}
uint64_t d_rs2(uint32_t inst)
{
	return (inst >> 20) & 0x1f; // rs2 in bits 24..20
}
uint64_t d_rs3(uint32_t inst)
{
	return (inst >> 27) & 0x1f; // rs3 in bits 31..27
}
uint64_t d_rm(uint32_t inst)
{
	return (inst >> 12) & 0x7; // rm in bits 13..15
}
uint64_t imm_Zicsr(uint32_t inst)
{
	return (inst >> 20);
}
uint64_t imm_I(uint32_t inst)
{
	// imm[11:0] = inst[31:20]
	return sext(inst >> 20, 12);
}
uint64_t imm_S(uint32_t inst)
{
	// imm[11:5] = inst[31:25], imm[4:0] = inst[11:7]
	return sext((get_bits(inst, 11, 7) | (get_bits(inst, 31, 25) << 5)), 12);
}
uint64_t imm_B(uint32_t inst)
{
	// imm[12|10:5|4:1|11] = inst[31|30:25|11:8|7]
	return sext(((get_bits(inst, 11, 8) << 1) | (get_bits(inst, 30, 25) << 5) | (get_bits(inst, 7, 7) << 11) | (get_bits(inst, 31, 31) << 12)),
				13);
}
uint64_t imm_U(uint32_t inst)
{
	// imm[31:12] = inst[31:12]
	return (int64_t)(int32_t)(inst & 0xFFFFF000u) >> 12;
}
uint64_t imm_J(uint32_t inst)
{
	// imm[20|10:1|11|19:12] = inst[31|30:21|20|19:12]
	return sext((get_bits(inst, 30, 21) << 1) | (get_bits(inst, 20, 20) << 11) | (get_bits(inst, 19, 12) << 12) | (get_bits(inst, 31, 31) << 20),
				21);
}
uint64_t shamt(uint32_t inst)
{
	// shamt(shift amount) only required for immediate shift instructions
	// shamt[4:5] = imm[5:0]
	return (uint32_t)(imm_I(inst) & 0x1f);
}
uint64_t shamt64(uint32_t inst)
{
	// shamt(shift amount) only required for immediate shift instructions
	// shamt[4:5] = imm[5:0]
	return (uint32_t)(imm_I(inst) & 0x3f);
}

__attribute__((noinline)) InstructionCache& InstructionDecoder::decode_inst_slow(uint64_t pc, uint32_t inst)
{
	size_t idx = (pc >> 2) & (32768 - 1);

	uint32_t lut_idx		 = get_lut_index(inst);
	const Instruction* dinst = lut[lut_idx];

	bool f = (dinst != nullptr) && ((inst & dinst->mask) == dinst->match);
	InstructionData data;
	data.inst = inst;
	data.rd	  = d_rd(inst);
	data.rs1  = d_rs1(inst);
	data.rs2  = d_rs2(inst);
#ifdef USE_FPU
	data.rs3 = d_rs3(inst);
	data.rm	 = d_rm(inst);
#endif
	if(f) [[likely]]
	{
		data.imm   = dinst->imm_decode_func(inst);
		cache[idx] = { pc, inst, *dinst, data, true };
	}
	else
	{
		data.imm = 0;
		Instruction invalid_inst{};
		cache[idx] = { pc, inst, invalid_inst, data, false };
	}

	// printf("inst=0x%lx match=0x%lx mask=0x%lx func=%p\n", inst, dinst->match, dinst->mask, dinst->func);
	return cache[idx];
}

void InstructionDecoder::register_instr(std::string mask, ExecReturn (*func)(Hart&, InstructionData&), uint64_t (*imm_decode_func)(uint32_t inst))
{
	assert(mask.size() == 32 && "Instruction mask size isn't 32 bits, good luck finding this broken instruction.");
	uint32_t inst_mask	= 0;
	uint32_t inst_match = 0;
	for(int i = 0; i < 32; i++)
	{
		char c = mask[31 - i];
		if(c == '1')
		{
			inst_match |= (1u << i);
			inst_mask |= (1u << i);
		}
		else if(c == '0')
		{
			inst_mask |= (1u << i);
		}
		else if(c == '*')
		{
			// dont care
		}
	}

	Instruction inst{
		inst_mask,
		inst_match,
		func,
		(imm_decode_func == NULL) ? imm_I : imm_decode_func, // default decode func if user dont provide such
	};
	// printf("Registered instr mask=0x%dx match=0x%dx with func=%p, imm_decode_func=%p\n", inst_mask, inst_match, func, imm_decode_func);
	instructions.push_back(inst);
	// std::string opcode_str = mask.substr(mask.length() - 7);
	// std::bitset<7> bits(opcode_str);
	// unsigned long opcode = bits.to_ulong();
	// opcode_table[opcode].push_back(inst);
	global_hash_mask |= inst_mask;
}

void InstructionDecoder::init_all_instrs()
{
	instructions.reserve(512);
	init_rv64i();
	init_rv64m();
	init_rv64a();
#ifdef USE_FPU
	init_rv64f();
#endif
	init_priv();
	init_zicsr();
	init_zifencei();
	init_zba();
	init_zbb();
	init_zbc();
	init_zbs();
	init_zicboz();
	build_lut();
}
void InstructionDecoder::build_lut()
{
	for(const auto& inst : instructions)
	{
		uint32_t effective_mask = inst.mask & HASH_MASK;
		uint32_t dont_care_bits = HASH_MASK & (~effective_mask);

		uint32_t sub_mask = 0;
		do
		{
			uint32_t test_inst = (inst.match & HASH_MASK) | sub_mask;

			uint32_t lut_idx = _pext_u32(test_inst, HASH_MASK);

			if(lut_idx >= LUT_SIZE)
			{
				printf("[CRITICAL] Index %u out of bounds!\n", lut_idx);
				continue;
			}

			if(lut[lut_idx] != nullptr)
			{
				if(__builtin_popcount(inst.mask) > __builtin_popcount(lut[lut_idx]->mask))
				{
					lut[lut_idx] = &inst;
				}
			}
			else
			{
				lut[lut_idx] = &inst;
			}

			sub_mask = (sub_mask - dont_care_bits) & dont_care_bits;
		} while(sub_mask != 0);
	}
}
