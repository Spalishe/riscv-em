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

#include "rvjit_emit.hpp"
#ifdef USE_JIT
#include "../decode.hpp"
#include <unordered_map>

struct JIT_Instruction
{
	bool (*func)(Hart& h, InstructionData& data, JIT_Block& ctx, JIT_Emitter& emitter);
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
	std::unordered_map<ExecReturn (*)(Hart&, InstructionData&), bool (*)(Hart&, InstructionData&, JIT_Block&, JIT_Emitter&)> conversion_tbl;
	JIT_InstructionCache decode_inst(InstructionCache cache);

	// This function will call on init, calling all sets functions to initialize
	void init_all_instrs();
	void init_rv64i();
};
#endif
