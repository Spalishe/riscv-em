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
#include <stdint.h>
#include <string>
#include <fstream>
#include "../include/decode.h"
#include "../include/csr.h"
#include "../include/devices/plic.hpp"
#include <cstdio>
#include <cstdarg>
#include <format>
#include <sstream>
#include <iomanip>
#include <functional>

void hart_reset(HART&, uint64_t dtb_path, bool gdbstub) {

}

uint32_t hart_fetch(HART&, uint64_t _pc) {

}

void hart_step(HART&) {

}

void hart_execute(HART&) {

}

void hart_check_interrupts(HART&) {

}

void hart_trap(HART&, uint64_t cause, uint64_t tval, bool is_interrupt) {

}