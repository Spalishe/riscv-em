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
#include "../../include/rvjit/rvjit_emit.hpp"

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
				// Check if our arena is overfilled
				if(arenas[last_arena].used_size == arenas[last_arena].size)
				{
					// Create new arena
					createNewArena();
				}

				rvjit_emit_epilogue(block.bytes, block.byte_pos);

				// We built block sized enough. Go go gadget w^x allocations
				JIT_Function func = arenas[last_arena].push_function(block.bytes, block.byte_pos);
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

			rvjit_emit_prologue(block.bytes, block.byte_pos);

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

void JIT_Context::createNewArena()
{
	last_arena++;
	arenas.insert({ last_arena, JIT_Arena() });
}

void JIT_Arena::allocate()
{
	_page_size = sysconf(_SC_PAGESIZE);
	size	   = RVJIT_ARENA_PAGES * _page_size;

	// Allocate READ | WRITE
	void* buffer = mmap(NULL, size, PROT_READ | PROT_WRITE,
						MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	if(buffer == MAP_FAILED)
	{
		fprintf(stderr, "[RVJIT] Failed to allocate RW region.\n");
		return;
	}

	// Change permissions to READ | EXEC for default
	if(mprotect(buffer, size, PROT_READ | PROT_EXEC) == -1)
	{
		munmap(buffer, size);
		fprintf(stderr, "[RVJIT] Failed to change region permission to RX.\n");
		return;
	}
	base	  = reinterpret_cast<JITCompilatedFunc>(buffer);
	valid	  = true;
	used_size = 0;
}
JIT_Function JIT_Arena::push_function(const void* code, size_t code_size)
{
	if(used_size + RVJIT_FUNC_SIZE > size)
	{
		fprintf(stderr, "[RVJIT] Arena is full.\n");
		return JIT_Function{};
	}

	if(code_size > RVJIT_FUNC_SIZE)
	{
		fprintf(stderr, "[RVJIT] Emitted code is larger than RVJIT_FUNC_SIZE.\n");
		return JIT_Function{};
	}

	void* buffer	  = reinterpret_cast<void*>(base);
	uint8_t* func_pos = static_cast<uint8_t*>(buffer) + used_size;

	uintptr_t page_addr	 = reinterpret_cast<uintptr_t>(func_pos);
	uintptr_t page_start = page_addr - (page_addr % _page_size);
	// Change permission to READ | WRITE
	if(mprotect(reinterpret_cast<void*>(page_start), _page_size, PROT_READ | PROT_WRITE) == -1)
	{
		fprintf(stderr, "[RVJIT] Failed to change region permission to RW.\n");
		return JIT_Function{};
	}

	// Write bytecode
	std::memcpy(func_pos, code, code_size);
	// Memcpy goes firstly to CPU I-cache, rather than straight to a memory
	__builtin___clear_cache(func_pos, func_pos + code_size);

	// Change permissions back to READ | EXEC
	if(mprotect(reinterpret_cast<void*>(page_start), _page_size, PROT_READ | PROT_EXEC) == -1)
	{
		fprintf(stderr, "[RVJIT] Failed to change region permission to RX.\n");
		return JIT_Function{};
	}

	used_size += RVJIT_FUNC_SIZE;
	JIT_Function result;
	result.func	 = reinterpret_cast<JITCompilatedFunc>(func_pos);
	result.size	 = code_size;
	result.valid = true;
	return result;
}

#endif
