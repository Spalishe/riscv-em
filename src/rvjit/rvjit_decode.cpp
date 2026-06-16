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

#ifdef USE_JIT
#include "../../include/rvjit/rvjit_decode.hpp"

JIT_InstructionCache JIT_InstructionDecoder::decode_inst(InstructionCache cache)
{
	uint32_t inst_raw = 0;
	JIT_Instruction inst;
	InstructionData data;
	bool valid = false;
	if(auto val = conversion_tbl.find(cache.inst.func); val != conversion_tbl.end())
	{
		valid	 = true;
		inst	 = { val->second, cache.inst.imm_decode_func };
		data	 = { cache.data.inst, cache.data.rs1, cache.data.rs2, cache.data.rd, cache.data.imm };
		inst_raw = cache.inst_raw;
	}
	return { inst_raw, inst, data, valid };
}
void JIT_InstructionDecoder::init_all_instrs()
{
	init_rv64i();
}
#endif
