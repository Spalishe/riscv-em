/*
Copyright 2025 Spalishe

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
#include <stdint.h>
#include <string>
#include <fstream>
#include "../include/opcodes.h"
#include "../include/csr.h"
#include "../include/devices/plic.hpp"
#include <cstdio>
#include <cstdarg>
#include <format>
#include <sstream>
#include <iomanip>
#include <functional>

uint32_t HART::cpu_fetch(uint64_t _pc) {

}

void HART::cpu_start(uint64_t dtb_path, bool gdbstub) {
	
}

void HART::cpu_loop() {
	
}

void HART::cpu_trap(uint64_t cause, uint64_t tval, bool is_interrupt)
{

}

void HART::cpu_check_interrupts() {
    
}