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

#include "../include/argparser.h"
#include <iostream>
#include <string>

#include "../include/machine.hpp"
#include "../include/main.hpp"
#include "../include/termios.hpp"

/*
        TODO:
    1. Replace pc_hits with array[128], indexing: idx = (pc >> 2) & (SIZE - 1);
    2. Remove all instances of copying inst_data, make pointers instead
    3. Replace for(inst_data dat : block->instrs) with for(const inst_data& dat
   : block->instrs)
    4. Remove vector from InstructionBlock with array
    5. Remove all optionals and replace those with return bool & write val at
   pointer
    6. Remove PC from inst_data
    7. Replace bools in inst_data to 1 bit flags
    8. Change instr_cache size to smth lower
    9. Optimize devices search
    10. Replace switches with to lookup table

                - Fix the problem that kernel is stuck in arch_cpu_idle ->
   handle_exception -> arch_cpu_idle

                -RV64F
                -RV64D

                -RTC GoldFish

                -MMU Fixes(if required)
                -RVC    <- this is required to run MMU-Linux
*/

int main(int argc, char *argv[]) {
  Argparser::Argparser parser(argc, argv);
  parser.setProgramName("RISC-V EM");
  parser.addArgument("--bios", "File with Machine Level program (bootloader)",
                     true, false, Argparser::ArgumentType::str);

  parser.addArgument("--kernel", "File with Supervisor Level program", false,
                     false, Argparser::ArgumentType::str);
  parser.addArgument("--image",
                     "File with Image file that will put on VirtIO-BLK", false,
                     false, Argparser::ArgumentType::str);

  parser.addArgument("--dtb", "Use specified FDT instead of auto-generated",
                     false, false, Argparser::ArgumentType::str);
  parser.addArgument("--dumpdtb", "Dumps auto-generated FDT to file", false,
                     false, Argparser::ArgumentType::str);
  parser.addArgument("--append", "Append command line arguments", false, false,
                     Argparser::ArgumentType::str);
#ifdef USE_GDBSTUB
  parser.addArgument("--gdb", "Starts GDB Stub on port 1512", false, false,
                     Argparser::ArgumentType::def);
#endif

  parser.parse();

  bool kernel_has = parser.getDefined(1);
  bool image_has = parser.getDefined(2);

  bool dtb_has = parser.getDefined(3);
  bool dtb_dump_has = parser.getDefined(4);

  std::string cmdline_append;
  if (parser.getDefined(5)) {
    cmdline_append = parser.getString(5);
    std::cout << "Custom cmdargs defined: " << cmdline_append << std::endl;
  }

  Machine VM = Machine();
  VM.core_count = 1;
  VM.memsize = DRAM_SIZE;

  machine_create_memory(VM);

  bool file_has = parser.getDefined(0);
  if (file_has) {
    std::string file = parser.getString(0);
    VM.memmap.load_file(DRAM_BASE, file);
  }

  if (!dtb_has)
    machine_create_fdt(VM, cmdline_append);
  machine_create_devices(VM, (image_has ? parser.getString(2) : ""));
  machine_load_fdt(VM, (dtb_has ? parser.getString(3) : ""),
                   (dtb_dump_has ? parser.getString(4) : ""));

  if (kernel_has) {
    std::string kernel_path = parser.getString(1);
    VM.memmap.load_file(DRAM_BASE + 0x200000, kernel_path);
  }
  machine_create_harts(VM);

  termios_start();

  std::thread VM_thread(machine_run, std::ref(VM));

#ifdef USE_GDBSTUB
  bool gdb_stub = parser.getDefined(6);
  VM.gdb = gdb_stub;
  if (gdb_stub) {
    std::thread GDB_thread(GDB_Create, VM.harts[0], &VM);
    GDB_thread.join();
  }
#endif

  VM_thread.join();

  return 0;
}
