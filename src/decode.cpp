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

InstructionCache InstructionDecoder::decode_inst(uint32_t inst)
{
	if(cache[inst & 0x1FFF].inst_raw == inst && cache[inst & 0x1FFF].valid)
	{
		return cache[inst & 0x1FFF];
	}
	else
	{
		const Instruction* dinst;
		bool f = false;
		for(const auto& ins : instructions)
		{
			if((inst & ins.mask) == ins.match)
			{
				dinst = &ins;
				f	  = true;
				break;
			}
		}
		InstructionData data;
		data.inst = inst;
		data.rd	  = d_rd(inst);
		data.rs1  = d_rs1(inst);
		data.rs2  = d_rs2(inst);
		data.imm  = f ? dinst->imm_decode_func(inst) : 0;

		InstructionCache inst_cache;
		inst_cache.inst_raw	 = inst;
		inst_cache.inst		 = *dinst;
		inst_cache.data		 = data;
		inst_cache.valid	 = f;
		cache[inst & 0x1FFF] = inst_cache;
		// printf("inst=0x%lx match=0x%lx mask=0x%lx func=%p\n", inst, dinst->match, dinst->mask, dinst->func);
		return inst_cache;
	}
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
}

void InstructionDecoder::init_all_instrs()
{
	init_rv64i();
	init_rv64m();
	init_rv64a();
	init_priv();
	init_zicsr();
	init_zifencei();
	init_zba();
	init_zbb();
	init_zbc();
	init_zbs();
}
