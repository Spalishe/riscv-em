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
