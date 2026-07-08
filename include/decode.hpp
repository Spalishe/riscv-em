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

#pragma once
#include "defines/traps.hpp"
#include <array>
#include <cstdint>
#include <immintrin.h>
#include <string>
#include <vector>

int32_t sext(uint32_t val, int bits);
uint32_t get_bits(uint32_t inst, int hi, int lo);
uint64_t d_rd(uint32_t inst);
uint64_t d_rs1(uint32_t inst);
uint64_t d_rs2(uint32_t inst);
uint64_t d_rs3(uint32_t inst);
uint64_t d_rm(uint32_t inst);
uint64_t imm_Zicsr(uint32_t inst);
uint64_t imm_I(uint32_t inst);
uint64_t imm_S(uint32_t inst);
uint64_t imm_B(uint32_t inst);
uint64_t imm_U(uint32_t inst);
uint64_t imm_J(uint32_t inst);
uint64_t shamt(uint32_t inst);
uint64_t shamt64(uint32_t inst);

struct Hart;

struct InstructionData
{
	uint32_t inst;
	uint8_t rs1;
	uint8_t rs2;
	uint8_t rd;
	uint64_t imm;
#ifdef USE_FPU
	uint8_t rs3;
	uint8_t rm;
#endif
};

struct Instruction
{
	uint32_t mask;
	uint32_t match;
	ExecReturn (*func)(Hart& h, InstructionData& data);
	uint64_t (*imm_decode_func)(uint32_t inst);
	uint8_t size = 4;
};

struct InstructionCache
{
	uint64_t pc;
	uint32_t inst_raw = 0;
	Instruction inst;
	InstructionData data;
	bool valid = false;
};

struct InstructionDecoder
{
	std::vector<Instruction> instructions;
	static constexpr size_t LUT_SIZE	= 1 << 22;
	const Instruction* lut[LUT_SIZE]	= { nullptr };
	static constexpr uint32_t HASH_MASK = 0xFFF0707F;
	InstructionCache cache[32768];
	InstructionCache& decode_inst_slow(uint64_t pc, uint32_t inst);
	inline InstructionCache& decode_inst(uint64_t pc, uint32_t inst)
	{
		size_t idx = (pc >> 2) & (32768 - 1);
		if(cache[idx].valid && cache[idx].pc == pc) [[likely]]
		{
			return cache[idx];
		}

		return decode_inst_slow(pc, inst);
	}

	uint32_t global_hash_mask = 0;
	void build_lut();
	void register_instr(std::string mask, ExecReturn (*func)(Hart&, InstructionData&), uint64_t (*imm_decode_func)(uint32_t inst) = NULL);
	static inline uint32_t get_lut_index(uint32_t inst)
	{
		return _pext_u32(inst, HASH_MASK);
	}
	// This function will call on init, calling all sets functions to initialize
	void init_all_instrs();
	void init_rv64i();
	void init_rv64m();
	void init_rv64a();
#ifdef USE_FPU
	void init_rv64f();
#endif
	void init_priv();
	void init_zicsr();
	void init_zifencei();
	void init_zba();
	void init_zbb();
	void init_zbc();
	void init_zbs();
	void init_zicboz();
};
