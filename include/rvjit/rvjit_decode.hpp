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

#ifdef USE_JIT
#include "../decode.hpp"
#include "rvjit.hpp"
#include <unordered_map>

struct JIT_Instruction
{
	ExecReturn (*func)(Hart& h, InstructionData& data, JIT_Block& ctx);
	uint64_t (*imm_decode_func)(uint32_t inst);
};

struct JIT_InstructionCache
{
	uint32_t inst_raw = 0;
	JIT_Instruction inst;
	InstructionData data;
	bool valid = false;
};
struct JIT_InstructionDecoder
{
	std::unordered_map<ExecReturn (*)(Hart&, InstructionData&), ExecReturn (*)(Hart&, InstructionData&, JIT_Block&)> conversion_tbl;
	JIT_InstructionCache decode_inst(InstructionCache cache);

	// This function will call on init, calling all sets functions to initialize
	void init_all_instrs();
	void init_rv64i();
};
#endif
