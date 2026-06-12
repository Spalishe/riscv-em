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
#include "../../include/rvjit/rvjit.hpp"
#include "../../include/hart.hpp"

void JIT_Context::handleInstruction(Hart& h, InstructionCache& cache)
{
	// This function excepts it will run after instruction execution, so subtract from current pc instruction size to get previous one
	uint64_t pc = h.pc - cache.inst.size;
	if(ignore_pc.contains(pc)) return;
	pc_hits[pc]++;

	if(block_c)
	{
		auto jc = h.jidec->decode_inst(cache);
		if(!jc.valid || block.count >= RVJIT_MAX_INSTRUCTIONS)
		{
			block_c = false;
			if(block.count > RVJIT_MIN_INSTRUCTIONS)
			{
				// We built block sized enough. Go go gadget w^x allocations
				JIT_Function func = allocateRX(block.bytes);
				func.inst_size	  = block.size;
				func.pc			  = block.pc;
				jits.insert({ func.pc, std::move(func) });
			}
			return;
		}
		block.size += cache.inst.size;
		block.count++;
		jc.inst.func(h, jc.data, block);

		return;
	}

	// If not block creating rn
	if(pc_hits[pc] > RVJIT_PC_CAP)
	{
		// Check if there any reference of this instruction in decoder
		auto jc = h.jidec->decode_inst(cache);
		if(jc.valid)
		{
			block_c = true;
			memset(&block, 0, sizeof(block));
			block.valid = true;
			block.pc	= pc;
			block.size	= cache.inst.size;
			block.count = 1;
			jc.inst.func(h, jc.data, block);
		}
		ignore_pc.insert(pc);
	}
}
void JIT_Context::stopBlock()
{
	if(block_c)
	{
		block_c = false;
	}
}

#include <sys/mman.h>
#include <unistd.h>

// TODO:
// Implement arena method, so we will allocate N pages(for 0x1000 each), and then internally split it
// to RVJIT_FUNC_SIZE sized functions, and place functions with increment.
// If our arena is fulled, allocate another one.
// If FENCE.I is upahead, clean up all pages and call __builtin___clear_cache(nullptr, nullptr) to clean up CPU I-cache
JIT_Function JIT_Context::allocateRX(unsigned char code[])
{
	// So we like instead of creation vulnerable RWX region we will instead firstly allocate RW
	// and only after writing bytecode to region we will change permissions to RX
	long page_size = sysconf(_SC_PAGESIZE);
	size_t size	   = (RVJIT_FUNC_SIZE + page_size - 1) & ~(page_size - 1);

	// Allocate READ | WRITE
	void* buffer = mmap(NULL, size, PROT_READ | PROT_WRITE,
						MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	if(buffer == MAP_FAILED)
	{
		fprintf(stderr, "[RVJIT] Failed to allocate RW region.\n");
		return JIT_Function{};
	}

	// Write the bytecode
	std::memcpy(buffer, code, RVJIT_FUNC_SIZE);

	// Change permissions to READ | EXEC
	if(mprotect(buffer, size, PROT_READ | PROT_EXEC) == -1)
	{
		munmap(buffer, size);
		fprintf(stderr, "[RVJIT] Failed to change region permission to RX.\n");
		return JIT_Function{};
	}

	// Memcopy goes firstly to CPU I-cache, rather than straight to a memory
	char* casted_ptr = static_cast<char*>(buffer);
	__builtin___clear_cache(casted_ptr, casted_ptr + RVJIT_FUNC_SIZE);

	JIT_Function result;
	result.func	 = reinterpret_cast<JITCompilatedFunc>(buffer);
	result.size	 = size;
	result.valid = true;
	return result;
}

#endif
