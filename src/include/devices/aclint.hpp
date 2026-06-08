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

#include "../device.hpp"

#define ACLINT_MSWI_SIZE   0x4000
#define ACLINT_MTIMER_SIZE 0x8000

struct ACLINT : public Device
{
	ACLINT(uint64_t base, uint64_t size, Machine& cpu, fdt_node* fdt);

	// Memory-mapped registers
	std::vector<uint32_t> msip;		// one per HART, 32-bit
	std::vector<uint64_t> mtimecmp; // one per HART, 64-bit
	uint64_t mtime;					// global timer
	uint16_t tmp;

	Machine& cpu;

	uint64_t read(uint64_t addr, MemorySize size);
	void write(uint64_t addr, MemorySize size, uint64_t val);
	void tick();
	static std::shared_ptr<ACLINT> init_auto(Machine& cpu);

	uint64_t read_mswi(uint64_t offset);
	uint64_t read_mtimer(uint64_t offset);
	void write_mswi(uint64_t offset, uint64_t value);
	void write_mtimer(uint64_t offset, uint64_t value);
	void update_mip();
};
