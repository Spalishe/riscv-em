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

#include "../include/mmio.hpp"
#include "../include/hart.hpp"

MMIO::MMIO(MemoryMap* mmap, uint64_t mem_size) : mmap(mmap), memsize(mem_size) {};

MemoryReturn MMIO::write(Hart& h, uint64_t vaddr, MemorySize size, uint64_t val)
{
	if(vaddr >= 0x80000000 && vaddr < (0x80000000 + memsize - (int)size)) // We subtracting by size to exclude chance of buffer overflow
	{
		// DRAM
		mmap->store(vaddr, (int)size * 8, val);
		return { true, 0, 0 };
	}
	// Looking up for devices in this range
	for(const auto& dev : devs)
	{
		if(vaddr >= dev->start && vaddr < (dev->start + dev->size - (int)size))
		{
			// found a device
			// mmap->store(vaddr, (int)size * 8, val); // unnecessary
			dev->write(vaddr, size, val);
			return { true, 0, 0 };
		}
	}

	// We hit none of the existing regions
	// h.trap(EXC_STORE_ACCESS_FAULT, vaddr, false);
	return { false, EXC_STORE_ACCESS_FAULT, vaddr };
}
MemoryReturn MMIO::read(Hart& h, uint64_t vaddr, MemorySize size, void* val)
{
	uint64_t out;
	if(vaddr >= 0x80000000 && vaddr < (0x80000000 + memsize - (int)size + 1)) // We subtracting by size to exclude chance of buffer overflow
	{
		// DRAM
		out = mmap->load(vaddr, (int)size * 8);
		goto success;
	}
	// Looking up for devices in this range
	for(const auto& dev : devs)
	{
		if(vaddr >= dev->start && vaddr < (dev->start + dev->size - (int)size + 1))
		{
			// found a device
			// out = mmap->load(vaddr, (int)size * 8); // unnecessary
			out = dev->read(vaddr, size);
			goto success;
		}
	}

	// We hit none of the existing regions
	// h.trap(EXC_LOAD_ACCESS_FAULT, vaddr, false);
	return { false, EXC_LOAD_ACCESS_FAULT, vaddr };

success:
	// write out to val
	std::memcpy(val, &out, (int)size);
	return { true, 0, 0 };
}
