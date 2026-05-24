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
#include <cstdint>
#include <string>
#include <vector>

int32_t sext(uint32_t val, int bits);
uint32_t get_bits(uint32_t inst, int hi, int lo);
uint64_t d_rd(uint32_t inst);
uint64_t d_rs1(uint32_t inst);
uint64_t d_rs2(uint32_t inst);
uint64_t d_rs3(uint32_t inst);
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
};

struct Instruction
{
	uint32_t mask;
	uint32_t match;
	ExecReturn (*func)(Hart& h, InstructionData& data);
	uint64_t (*imm_decode_func)(uint32_t inst);
};

struct InstructionCache
{
	uint32_t inst_raw = 0;
	Instruction inst;
	InstructionData data;
	bool valid = true;
};

struct InstructionDecoder
{
	std::vector<Instruction> instructions;
	InstructionCache cache[16384];
	InstructionCache decode_inst(uint32_t inst);
	void register_instr(std::string mask, ExecReturn (*func)(Hart&, InstructionData&), uint64_t (*imm_decode_func)(uint32_t inst) = NULL);

	// This function will call on init, calling all sets functions to initialize
	void init_all_instrs();
	void init_rv64i();
	void init_rv64m();
	void init_rv64a();
	void init_priv();
	void init_zicsr();
	void init_zifencei();
};
