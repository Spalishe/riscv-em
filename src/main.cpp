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

#include <iostream>
#include "../include/cpu.h"
#include "../include/argparser.h"

#include "../include/devices/mmio.h"
#include "../include/devices/aclint.hpp"
#include "../include/devices/rom.hpp"
#include "../include/devices/plic.hpp"
#include "../include/devices/uart.hpp"
#include "../include/devices/dtb.hpp"
#include "../include/devices/syscon.hpp"
#include "../include/devices/virtio_blk.hpp"

#include "../include/memory_map.h"
#include "../include/libfdt.hpp"
#include "../include/gdbstub.hpp"
#include <string>
#include <vector>
#include <random>

#include "../include/main.hpp"
#include "../include/machine.hpp"

#include <stdio.h>
#include <signal.h>
#include <sys/types.h>
#include <dirent.h>
#include <filesystem>

/*
	What should i add to functionality if i want to add 2 harts:
		- FENCE
		- THE REAL ATOMICITY

	The Broken Insts:
		none
	TODO:
		Entire core btw
		- Counter CSRs
		-RTC GoldFish

		-Zba - bit manip
		-Zbb - bit manip
		-Zbc - carry-less mul
		-Zicbom - non-coherent DMA
		-Zicboz - fast memory zeroing

		-MMU
		-RV32F
		-RV64F
		-RV32F for RV32C
		-RV64F for RV64C
		-RV32D
		-RV64D
		-RV32D for RV32C
		-RV64D for RV64C
*/

int main(int argc, char* argv[]) {
	Argparser::Argparser parser(argc,argv);
	parser.setProgramName("RISC-V EM");
	parser.addArgument("--bios", "File with Machine Level program (bootloader)",false,false,Argparser::ArgumentType::str);

	parser.addArgument("--kernel", "File with Supervisor Level program",false,false,Argparser::ArgumentType::str);
	parser.addArgument("--image", "File with Image file that will put on VirtIO-BLK",false,false,Argparser::ArgumentType::str);

	parser.addArgument("--dtb", "Use specified FDT instead of auto-generated",false,false,Argparser::ArgumentType::str);
	parser.addArgument("--dumpdtb", "Dumps auto-generated FDT to file",false,false,Argparser::ArgumentType::str);
	parser.addArgument("--gdb", "Starts GDB Stub on port 1512", false, false, Argparser::ArgumentType::def);
	parser.addArgument("--append", "Append command line arguments", false, false, Argparser::ArgumentType::str);

	parser.parse();

	std::string cmdline_append;
	if(parser.getDefined(6)) {
		cmdline_append = parser.getString(6);
		std::cout << "Custom cmdargs defined: " << cmdline_append << std::endl;
	}

	bool kernel_has = parser.getDefined(1);
	bool image_has = parser.getDefined(2);

	bool dtb_has = parser.getDefined(3);
	bool dtb_dump_has = parser.getDefined(4);
	bool gdb_stub = parser.getDefined(5);

	Machine VM = Machine();
	VM.core_count = 1;
	VM.memsize = DRAM_SIZE;
	VM.gdb = gdb_stub;
	machine_create_memory(VM);

	bool file_has = parser.getDefined(0);
	if(file_has) {
		std::string file = parser.getString(0);
		VM.memmap.load_file(DRAM_BASE,file);
	}
	
	machine_create_devices(VM,(image_has ? parser.getString(2) : ""));
	machine_create_fdt(VM,(dtb_has ? parser.getString(3) : ""),cmdline_append,(dtb_dump_has ? parser.getString(4) : ""));

	if (kernel_has) {
		std::string kernel_path = parser.getString(1);
		VM.memmap.load_file(DRAM_BASE+0x200000,kernel_path);
	}

	machine_run(VM);
	std::thread VM_thread = std::thread(machine_run,VM);

	VM_thread.join();
	
	return 0;
}
