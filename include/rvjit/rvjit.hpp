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
#include "../decode.hpp"
#include <cstdint>
#include <sys/mman.h>
#include <unordered_map>
#include <unordered_set>

#ifdef USE_JIT
#define RVJIT_MIN_INSTRUCTIONS 12
#define RVJIT_MAX_INSTRUCTIONS 48
#define RVJIT_PC_CAP		   100
#define RVJIT_FUNC_SIZE		   0x400
using JITCompilatedFunc = int (*)(void*);

struct JIT_Function
{
	// Default constructor
	JIT_Function() = default;

	// Destructor safely unmaps memory
	~JIT_Function()
	{
		if(func && size > 0)
		{
			munmap(reinterpret_cast<void*>(func), size);
		}
	}

	// Disable copy
	JIT_Function(const JIT_Function&)			 = delete;
	JIT_Function& operator=(const JIT_Function&) = delete;

	// Moving
	JIT_Function(JIT_Function&& other) noexcept
		: func(other.func), size(other.size), pc(other.pc),
		  inst_size(other.inst_size), valid(other.valid)
	{
		// Strip resource away from the old object so its destructor does nothing
		other.func	= nullptr;
		other.size	= 0;
		other.valid = false;
	}

	JIT_Function& operator=(JIT_Function&& other) noexcept
	{
		if(this != &other)
		{
			// Clean up our own existing memory first
			if(func && size > 0) munmap(reinterpret_cast<void*>(func), size);

			// Copy data
			func	  = other.func;
			size	  = other.size;
			pc		  = other.pc;
			inst_size = other.inst_size;
			valid	  = other.valid;

			// Reset other
			other.func	= nullptr;
			other.size	= 0;
			other.valid = false;
		}
		return *this;
	}

	JITCompilatedFunc func = nullptr;
	uint64_t size		   = 0;
	uint64_t pc			   = 0;
	uint64_t inst_size	   = 0;
	bool valid			   = false;
};

struct JIT_Block
{
	uint8_t bytes[RVJIT_FUNC_SIZE];
	uint16_t byte_pos;

	uint64_t pc;
	uint64_t size;
	uint64_t count;
	bool valid = false;
};
struct JIT_Context
{
	std::unordered_map<uint64_t, JIT_Function> jits;
	uint64_t pc_hits[1024];

	bool block_c	= false;
	JIT_Block block = { 0 };

	std::unordered_set<uint64_t> ignore_pc;
	void handleInstruction(Hart& h, InstructionCache& cache);
	void stopBlock();
	JIT_Function allocateRX(unsigned char code[RVJIT_FUNC_SIZE]);
};
#endif
