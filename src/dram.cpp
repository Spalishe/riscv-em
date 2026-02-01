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

#include "../include/dram.h"
#include "../include/decode.h"
#include <format>

uint64_t dram_load(DRAM* dram, uint64_t addr, uint64_t size) {
	return dram->mmap->load(addr,size);
}

void dram_store(DRAM* dram, uint64_t addr, uint64_t size, uint64_t value) {
	dram->mmap->store(addr,size,value);
    amo_check_reservation(*dram->cpu, addr);
}

void dram_cache_clear(DRAM* dram) {
	dram->mmap->cache.clear();
}