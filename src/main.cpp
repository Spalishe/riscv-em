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

#include "../include/argparser.hpp"
#include <iostream>
#include <string>

#include "../include/gdbstub.hpp"
#include "../include/main.hpp"

/*
        TODO:
    5. Remove all optionals and replace those with return bool & write val at
   pointer
    9. Optimize devices search

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

  return 0;
}
