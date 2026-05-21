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
};
