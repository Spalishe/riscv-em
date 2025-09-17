#include "../include/dram.h"

uint64_t dram_load(DRAM* dram, uint64_t addr, uint64_t size) {
	return dram->mmap->load(addr,size);
}

void dram_store(DRAM* dram, uint64_t addr, uint64_t size, uint64_t value) {
	dram->mmap->store(addr,size,value);
}

void dram_cache(DRAM* dram) {
	dram->mmap->cache.clear();
}