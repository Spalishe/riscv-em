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
#include "../mmio.hpp"
#include <cstdint>
#include <cstring>
#include <sys/mman.h>
#include <unordered_map>

#define RVJIT_MIN_INSTRUCTIONS 12
#define RVJIT_MAX_INSTRUCTIONS 48
#define RVJIT_PC_CAP		   100
#define RVJIT_FUNC_SIZE		   0x1000 // DONT CHANGE IT IF YOU DONT KNOW WHAT YOU'RE DOING! If emitted function will overflow arena's buffer, it will be your fault
#define RVJIT_ARENA_PAGES	   0x400  // Linux default page size is 4096, then 1024 * 4096 = 4194304 bytes, 4 MB
#include "rvjit_emit.hpp"
struct JIT_HartContext
{
	uint64_t* regs;
	uint8_t* ram;
	MMIO* mmio;
	uint64_t memsize;
	uint64_t exit_pc = 0;
};

using JITCompilatedFunc = void (*)(JIT_HartContext*);

struct JIT_Function
{
	JITCompilatedFunc func = nullptr;
	uint64_t offset		   = 0; // offset from arena base
	uint64_t size		   = 0; // emitted code size
	uint64_t pc			   = 0;
	uint64_t inst_size	   = 0;
	bool valid			   = false;

	JIT_Function(const JIT_Function&)			 = delete;
	JIT_Function& operator=(const JIT_Function&) = delete;

	JIT_Function(JIT_Function&& other) noexcept
		: func(other.func),
		  offset(other.offset),
		  size(other.size),
		  pc(other.pc),
		  inst_size(other.inst_size),
		  valid(other.valid)
	{
		other.func		= nullptr;
		other.offset	= 0;
		other.size		= 0;
		other.pc		= 0;
		other.inst_size = 0;
		other.valid		= false;
	}

	JIT_Function& operator=(JIT_Function&& other) noexcept
	{
		if(this != &other)
		{
			func	  = other.func;
			offset	  = other.offset;
			size	  = other.size;
			pc		  = other.pc;
			inst_size = other.inst_size;
			valid	  = other.valid;

			other.func		= nullptr;
			other.offset	= 0;
			other.size		= 0;
			other.pc		= 0;
			other.inst_size = 0;
			other.valid		= false;
		}
		return *this;
	}

	JIT_Function() = default;
};

struct JIT_Arena
{
	JIT_Arena() {

	};
	// Destructor
	~JIT_Arena()
	{
		if(base && size > 0)
		{
			munmap(reinterpret_cast<void*>(base), size);
		}
	}

	// Disable copy, only move
	JIT_Arena(const JIT_Arena&)			   = delete;
	JIT_Arena& operator=(const JIT_Arena&) = delete;

	// Move
	JIT_Arena(JIT_Arena&& other) noexcept
		: size(other.size), used_size(other.used_size),
		  base(other.base), valid(other.valid)
	{
		// Strip resource away from the old object so its destructor does nothing
		other.base		= nullptr;
		other.size		= 0;
		other.valid		= false;
		other.used_size = 0;
	}
	// Move assigment
	JIT_Arena& operator=(JIT_Arena&& other) noexcept
	{
		if(this != &other)
		{
			// Clean up our own existing memory first
			if(base && size > 0) munmap(reinterpret_cast<void*>(base), size);

			// Copy data
			base	  = other.base;
			size	  = other.size;
			valid	  = other.valid;
			used_size = other.used_size;

			// Reset other
			other.base		= nullptr;
			other.size		= 0;
			other.valid		= false;
			other.used_size = 0;
		}
		return *this;
	}

	bool valid = true;
	JITCompilatedFunc base;
	uint64_t size	   = 0;
	uint64_t used_size = 0;

	JIT_Function push_function(const void* code, size_t code_size);
	void init()
	{
		allocate();
	}

  private:
	uint64_t _page_size = 0;
	void allocate();
};
template <typename Key, typename Value, typename Hash = std::hash<Key>>
class FastHashMap
{
  public:
	using value_type = std::pair<Key, Value>;

	FastHashMap(size_t initial_capacity = 64, float max_load = 0.7f)
		: max_load_(max_load), size_(0)
	{
		rehash(initial_capacity);
	}

	void insert(const Key& key, Value&& val)
	{
		if(size_ + 1 > max_load_ * capacity_)
		{
			rehash(capacity_ * 2);
		}
		size_t idx = find_index(key);
		if(idx != npos && occupied_[idx])
		{
			data_[idx].second = std::move(val);
			return;
		}
		idx			   = find_empty(key);
		data_[idx]	   = value_type(key, std::move(val));
		occupied_[idx] = true;
		++size_;
	}

	Value* find(const Key& key)
	{
		size_t idx = find_index(key);
		if(idx != npos && occupied_[idx] && data_[idx].first == key)
		{
			return &data_[idx].second;
		}
		return nullptr;
	}

	const Value* find(const Key& key) const
	{
		size_t idx = find_index(key);
		if(idx != npos && occupied_[idx] && data_[idx].first == key)
		{
			return &data_[idx].second;
		}
		return nullptr;
	}

	Value& operator[](const Key& key)
	{
		Value* v = find(key);
		if(v) return *v;
		insert(key, Value{});
		return *find(key);
	}

	size_t size() const { return size_; }
	void reserve(size_t new_cap)
	{
		if(new_cap > capacity_)
		{
			rehash(new_cap);
		}
	}

	void clear()
	{
		data_.clear();
		data_.resize(capacity_);
		std::fill(occupied_.begin(), occupied_.end(), false);
		size_ = 0;
	}

  private:
	static constexpr size_t npos = ~size_t(0);

	std::vector<value_type> data_;
	std::vector<bool> occupied_;
	size_t capacity_ = 0;
	size_t size_	 = 0;
	float max_load_;
	Hash hash_;

	size_t hash_key(const Key& k) const
	{
		return hash_(k);
	}

	size_t find_index(const Key& key) const
	{
		size_t h	 = hash_key(key);
		size_t mask	 = capacity_ - 1;
		size_t idx	 = h & mask;
		size_t start = idx;
		do
		{
			if(!occupied_[idx]) return npos;
			if(data_[idx].first == key) return idx;
			idx = (idx + 1) & mask;
		} while(idx != start);
		return npos;
	}

	size_t find_empty(const Key& key)
	{
		size_t h	 = hash_key(key);
		size_t mask	 = capacity_ - 1;
		size_t idx	 = h & mask;
		size_t start = idx;
		do
		{
			if(!occupied_[idx]) return idx;
			idx = (idx + 1) & mask;
		} while(idx != start);
		return npos;
	}

	void rehash(size_t new_cap)
	{
		size_t cap = 1;
		while(cap < new_cap)
			cap <<= 1;

		std::vector<value_type> old_data = std::move(data_);
		std::vector<bool> old_occ		 = std::move(occupied_);
		size_t old_cap					 = capacity_;

		data_.resize(cap);
		occupied_.resize(cap);
		for(size_t i = 0; i < cap; ++i)
			occupied_[i] = false;
		capacity_ = cap;
		size_	  = 0;

		for(size_t i = 0; i < old_cap; ++i)
		{
			if(old_occ[i])
			{
				insert(std::move(old_data[i].first),
					   std::move(old_data[i].second));
			}
		}
	}
};
struct JIT_Context
{
	JIT_Context()
	{
		last_arena = 0;
		emitter	   = JIT_Emitter();
		createNewArena();
	};

	// Forbid copy
	JIT_Context(const JIT_Context&)			   = delete;
	JIT_Context& operator=(const JIT_Context&) = delete;

	// Move constructor
	JIT_Context(JIT_Context&& other) noexcept
		: last_arena(other.last_arena), jits(std::move(other.jits)),
		  arenas(std::move(other.arenas)),
		  block_c(other.block_c), block(other.block)
	{
		// Copy pc_hits
		memcpy(pc_hits, other.pc_hits, sizeof(pc_hits));
		memcpy(&ignore_pc, &other.ignore_pc, sizeof(ignore_pc));
	}
	// Move assigment
	JIT_Context& operator=(JIT_Context&& other) noexcept
	{
		if(this != &other)
		{
			jits   = std::move(other.jits);
			arenas = std::move(other.arenas);
			memcpy(&ignore_pc, &other.ignore_pc, sizeof(ignore_pc));

			memcpy(pc_hits, other.pc_hits, sizeof(pc_hits));

			block_c	   = other.block_c;
			block	   = other.block;
			last_arena = other.last_arena;
		}

		return *this;
	}

	FastHashMap<uint64_t, JIT_Function> jits;
	std::unordered_map<uint64_t, JIT_Arena> arenas;
	uint64_t pc_hits[16384] = { 0 };

	bool block_c	= false;
	JIT_Block block = { 0 };

	uint64_t last_arena = 0;

	JIT_Emitter emitter;

	uint64_t ignore_pc[1 << 20] = { 0 };
	void handleInstruction(Hart& h, InstructionCache& cache, uint64_t prev_pc);
	void stopBlock();
	void createNewArena();
};
#endif
