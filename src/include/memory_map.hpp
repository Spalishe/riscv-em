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
#include "elfparser.hpp"
#include <cstdint>
#include <cstring>
#include <vector>

struct MemoryRegion
{
	uint64_t base_addr;
	size_t size;
	uint8_t* data;

	MemoryRegion(uint64_t base, size_t sz);
	~MemoryRegion();
	uint8_t* ptr(uint64_t addr);
};

struct MemoryMap
{
	std::vector<MemoryRegion*> regions;
	ELFParser elf = ELFParser(this);

	~MemoryMap();
	void add_region(uint64_t base, size_t size);
	bool load_file(uint64_t memory_path, std::string path = "", uint64_t* entry_pc = NULL);
	bool load_buffer(uint64_t memory_path, char* buffer, uint64_t size, uint64_t* entry_pc = NULL);
	uint64_t load(uint64_t addr, uint64_t size);
	void copy_mem(uint64_t addr, uint64_t size, void* buffer);
	void store(uint64_t addr, uint64_t size, uint64_t value);
	MemoryRegion* find_region(uint64_t addr);
};
