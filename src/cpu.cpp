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

void hart_reset(HART& h, uint64_t dtb_path) {
    for(int i = 0; i < 32; i++) {h.GPR[i] = 0;}
    for(int i = 0; i < 4069; i++) {h.csrs[i] = 0;}

	h.pc = DRAM_BASE;
	h.GPR[10] = h.id;

	h.GPR[11] = dtb_path;

	h.csrs[MISA] = riscv_mkmisa("i");
	h.csrs[MVENDORID] = 0; 
	h.csrs[MARCHID] = 0; 
	h.csrs[MIMPID] = 0;
	h.csrs[MHARTID] = h.id;
	h.csrs[MIDELEG] = 5188;
	h.csrs[MIP] = 0x80;
	h.csrs[MSTATUS] = (0xA00000000);
	h.csrs[SSTATUS] = (0x200000000);
}

uint32_t hart_fetch(HART& h, uint64_t _pc) {
    uint64_t fetch_buffer_end = h.fetch_pc + 28;
	if(_pc < h.fetch_pc || _pc > fetch_buffer_end) {
		// Refetch
		bool out = h.mmio->ram.mmap->copy_mem_safe(_pc,32,&h.fetch_buffer);
		if(out == false) return 0; 
		h.fetch_pc = _pc;
	}
	uint8_t fetch_indx = (_pc - h.fetch_pc) / 4;
	return h.fetch_buffer[fetch_indx];
}

void hart_step(HART& h) {
    uint32_t inst = hart_fetch(h,h.pc);
    inst_data d = parse_instruction(&h, inst, h.pc);
    if(d.valid == false) {
        hart_trap(h,EXC_ILLEGAL_INSTRUCTION, inst, false);
    }
    hart_execute(h,d);
}

void hart_execute(HART& h, inst_data inst) {
    inst.fn(&h,inst);
    h.pc += 4;
}

void hart_check_interrupts(HART& h) {

}

void hart_trap(HART& h, uint64_t cause, uint64_t tval, bool is_interrupt) {

}