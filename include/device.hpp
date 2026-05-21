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
#include "defines/traps.hpp"
#include "libfdt.hpp"
#include "memory_map.hpp"
#include <cstdint>

struct Device : std::enable_shared_from_this<Device>
{
	Device(uint64_t start, uint64_t size, fdt_node* fdt) : start(start), size(size) {

														   };
	MemoryMap* mmap;
	uint64_t start;
	uint64_t size;

	MemoryReturn read(uint64_t addr, uint8_t size);
	MemoryReturn write(uint64_t addr, uint8_t size, uint64_t val);
	void tick();
	static Device auto_init(fdt_node* fdt);

	// Returns device
	template <typename T>
	std::shared_ptr<T> get()
	{
		return std::dynamic_pointer_cast<T>(shared_from_this());
	}
};
