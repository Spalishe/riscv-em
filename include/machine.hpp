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
#include "mmio.hpp"
#include <thread>
#include <vector>

enum class MachineState
{
	Off		  = 0,
	Halted	  = 1,
	Running	  = 2,
	Resetting = 3,
};

struct Machine
{
	Machine(uint64_t mem_size, uint8_t hart_count);
	uint64_t memory_size;
	std::string append;
	std::string dtb_dump_path;
	// File, located on that path will be automatically loaded as Block device.
	std::string image_path	= "";
	// File, located on that path will be automatically loaded as Firmware on 0x80000000
	std::string bios_path	= "";
	// File, located on that path will be automatically loaded as Kernel on 0x80200000
	std::string kernel_path = "";
	fdt_node* fdt;
	MemoryMap* mmap;
	MMIO* mmio;
	InstructionDecoder* idec;
	uint64_t timebase = 3'500'000ULL;
	uint8_t harts_count;
	std::vector<Hart> harts;
	std::thread work_thread;
	bool work_thread_w;
	bool work_thread_joined; // manual variable to indicate that thread was joined. You probably want to set it to true if you do work_thread.join();

	std::atomic<MachineState> state = MachineState::Off;

#ifdef USE_GDBSTUB
	bool gdb			 = false;
	bool gdb_single_step = false;
#endif

	void init_mmap();
	void init_fdt();
	void write_fdt();
	void load_fdt(const std::string&);
	void init_auto_devices();
	void destroy_harts();
	void destroy_devices();
	void destroy_mmap();
	void reset_memory();
	void run();
	void reset();
	void work();
	void stop();
};
