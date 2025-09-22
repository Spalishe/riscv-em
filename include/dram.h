/*
Copyright 2025 Spalishe

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
#include <stdint.h>
#include <memory_map.h>

#define DRAM_SIZE 1024*1024*512 //512 MiB
#define DRAM_BASE 0x80000000

struct DRAM {
	uint8_t mem[DRAM_SIZE] = {};
	MemoryMap* mmap;
};

uint64_t dram_load(DRAM* dram, uint64_t addr, uint64_t size);
void dram_store(DRAM* dram, uint64_t addr, uint64_t size, uint64_t value);
void dram_cache(DRAM* dram);
