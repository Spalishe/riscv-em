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
#include "../include/argparser.h"
#include <string>

#include "../include/main.hpp"
#include "../include/machine.hpp"

/*
	TODO:
		-MMU
		-Zba - bit manip
		-Zbb - bit manip
		-Zbc - carry-less mul
		-Zicbom - non-coherent DMA
		-Zicboz - fast memory zeroing

		- Counter CSRs

		-RTC GoldFish

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
	parser.addArgument("--bios", "File with Machine Level program (bootloader)",true,false,Argparser::ArgumentType::str);

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
	machine_create_harts(VM);

	std::thread VM_thread(machine_run,std::ref(VM));
	if(gdb_stub) {
		std::thread GDB_thread(GDB_Create,VM.harts[0],&VM);
		GDB_thread.join();
	}

	VM_thread.join();
	
	return 0;
}
