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
#include "device.hpp"
#include "hart.hpp"
#include "libfdt.hpp"
#include "memory_map.hpp"
#include <thread>
#include <vector>

enum MachineState
{
	Off		= 0,
	Halted	= 1,
	Running = 2,
};

struct Machine
{
	Machine(uint64_t mem_size, uint8_t hart_count);
	uint64_t memory_size;
	std::string append;
	std::string dtb_dump_path;
	fdt_node* fdt;
	MemoryMap* mmap;
	uint64_t timebase;
	uint8_t harts_count;
	std::vector<Hart> harts;
	std::vector<Device> devices;
	std::thread work_thread;
	bool work_thread_w;

	MachineState state = MachineState::Off;

	void init_mmap();
	void init_fdt();
	void write_fdt();
	void load_fdt(const std::string&);
	void init_auto_devices();
	void run();

	void work();
	void stop();
};
