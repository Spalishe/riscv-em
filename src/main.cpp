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

#include "argparser.cpp"
#include <exception>
#include <iostream>
#include <string>
#include <unordered_map>

#include "../include/gdbstub.hpp"
#include "../include/machine.hpp"
#include "../include/main.hpp"
#include "fcntl.h"
#include "termios.h"
#include <thread>

/*
 *		   TODO:
 *		    -CSRs
 *          -EXC
 *          -RV64I
 *          -RV64M
 *          -RV64A
 *          -Zicsr
 *          -ZiFenceI
 *          -Device:
 *            1. PLIC
 *            2. UART
 *            3. VirtIO-BLK
 *            4. ACLINT
 *            5. RTC GoldFish
 *			-Counters
 */

termios oldt;
bool termios_running = false;
// Thread function for overriding default stdin control keys
void termios_loop()
{
	char ch;
	while(termios_running)
	{
		if(read(STDIN_FILENO, &ch, 1) > 0)
		{
		}
	}
}
void cleanup_terminal()
{
	tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
}

int main(int argc, char* argv[])
{
	arp::Argparser parser(argc, argv);
	parser.setDescription("RISC-V EM");
	auto bios_var
		= parser.add<arp::str>("--bios", "File with Machine level program (bootloader)", arp::required, arp::nopos);
	auto kernel_var
		= parser.add<arp::str>("--kernel", "File with Supervisor Level program", arp::norequired, arp::nopos);
	auto image_var = parser.add<arp::str>("--image", "File with Image file that will put on VirtIO-BLK",
										  arp::norequired, arp::nopos);

	auto dtb_var
		= parser.add<arp::str>("--dtb", "Use specified FDT instead of auto-generated", arp::norequired, arp::nopos);
	auto dumpdtb_var
		= parser.add<arp::str>("--dumpdtb", "Dumps auto-generated FDT to file", arp::norequired, arp::nopos);
	auto append_var = parser.add<arp::str>("--append", "Append command line arguments", arp::norequired, arp::nopos);
#ifdef USE_GDBSTUB
	auto gdb_var = parser.add<arp::def>("--gdb", "Starts GDB Stub on port 1512", arp::norequired, arp::nopos);
#endif
	auto mem_var = parser.add<arp::str>("--memsize", "Set custom memory size (Default is 512 MB)", arp::norequired,
										arp::nopos, "-M");
	auto harts_var
		= parser.add<arp::uint>("--harts", "Set custom harts count (Default is 1)", arp::norequired, arp::nopos, "-S");

	parser.parse();

	std::string cmdline_append;
	if(append_var->defined())
	{
		cmdline_append = append_var->val();
		std::cout << "Custom cmdargs defined: " << cmdline_append << std::endl;
	}

	uint64_t memsize = 1024 * 1024 * 512; // 512 MB
	if(mem_var->defined())
	{
		std::string s = mem_var->val();
		char l		  = s.back();
		s.pop_back();

		std::unordered_map<char, int8_t> symbol_to_shift = {
			{ 'B', -10 },
			{ 'K', 1	 },
			{ 'M', 10  },
			{ 'G', 20  },
		};

		uint64_t mul = 0;
		try
		{
			mul = stoull(s);
		}
		catch(const std::exception& ex)
		{
			std::cerr << "Size '" << s << "' is not a number." << std::endl;
			return -1;
		}
		uint64_t size = 1024;
		size		  = size << symbol_to_shift[l];
		size *= mul;
	}
	uint8_t harts = 1;
	if(harts_var->defined())
	{
		harts = harts_var->val();
		if(harts > 64)
		{
			std::cerr << "RISC-V CPU cannot have more than 64 cores at 1 time." << std::endl;

			return -1;
		}
	}

	atexit(cleanup_terminal);
	termios newt;
	tcgetattr(STDIN_FILENO, &oldt);
	newt = oldt;
	newt.c_lflag &= ~(ICANON | ISIG | ECHO);
	tcsetattr(STDIN_FILENO, TCSANOW, &newt);

	Machine machine = Machine(memsize, harts);

	if(dtb_var->defined())
	{
		machine.load_fdt(dtb_var->val());
	}
	else
	{
		machine.init_fdt();
	}

	machine.init_auto_devices();
	machine.run();
	machine.work_thread.join(); // joining it so our program will not exit right after creating machine

	return 0;
}
