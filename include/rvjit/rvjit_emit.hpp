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
#include "rvjit_x86_64.hpp"
#include <cstdint>
#include <unordered_set>
struct VReg
{
	uint8_t user_reg;
	uint8_t host_reg;
	bool dirty;
};
extern std::unordered_set<uint8_t> spill_regs;
extern void rvjit_emit_prologue(unsigned char bytes[], uint16_t byte_pos);
extern void rvjit_emit_epilogue(unsigned char bytes[], uint16_t byte_pos);
extern VReg rvjit_alloc_reg(uint8_t user_reg);

#endif
