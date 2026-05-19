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
#include <iostream>
#include <string>

#include "../include/gdbstub.hpp"
#include "../include/main.hpp"

/*
 *		   TODO:
 *          -RV64I
 *          -RV64M
 *          -RV64A
 *          -CSRs
 *          -Zicsr
 *          -ZiFenceI
 *          -EXC
 *          -Device:
 *            1. PLIC
 *            2. UART
 *            3. VirtIO-BLK
 *            4. ACLINT
 *            5. RTC GoldFish
 *
 */

int main(int argc, char *argv[]) {
  arp::Argparser parser(argc, argv);
  parser.setDescription("RISC-V EM");
  auto bios_var = parser.add<arp::str>(
      "--bios", "File with Machine level program (bootloader)", arp::norequired,
      arp::nopos);
  auto kernel_var =
      parser.add<arp::str>("--kernel", "File with Supervisor Level program",
                           arp::norequired, arp::nopos);
  auto image_var = parser.add<arp::str>(
      "--image", "File with Image file that will put on VirtIO-BLK",
      arp::norequired, arp::nopos);

  auto dtb_var = parser.add<arp::str>(
      "--dtb", "Use specified FDT instead of auto-generated", arp::norequired,
      arp::nopos);
  auto dumpdtb_var =
      parser.add<arp::str>("--dumpdtb", "Dumps auto-generated FDT to file",
                           arp::norequired, arp::nopos);
  auto append_var = parser.add<arp::str>(
      "--append", "Append command line arguments", arp::norequired, arp::nopos);
#ifdef USE_GDBSTUB
  auto gdb_var = parser.add<arp::def>("--gdb", "Starts GDB Stub on port 1512",
                                      arp::norequired, arp::nopos);
#endif

  parser.parse();

  std::string cmdline_append;
  if (append_var->defined()) {
    cmdline_append = append_var->val();
    std::cout << "Custom cmdargs defined: " << cmdline_append << std::endl;
  }

  return 0;
}
