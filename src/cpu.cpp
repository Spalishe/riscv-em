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
#include "../include/cpu.hpp"
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
	csr_write(&h,SSTATUS,0x200000000);
}

uint32_t hart_fetch(HART& h, uint64_t _pc) {
    uint64_t fetch_buffer_end = h.fetch_pc + 28;
	if(_pc < h.fetch_pc || _pc > fetch_buffer_end) {
		// Refetch
		bool out = h.mmio->ram->mmap->copy_mem_safe(_pc,32,&h.fetch_buffer);
		if(out == false) return 0; 
		h.fetch_pc = _pc;
	}
	uint8_t fetch_indx = (_pc - h.fetch_pc) / 4;
	return h.fetch_buffer[fetch_indx];
}

void hart_step(HART& h) {
    auto phys = mmu_translate(*h.mmio->mmu, &h, h.pc, AccessType::EXECUTE);
    if (!phys.has_value()) return;
    uint64_t addr = phys.value();
    uint32_t inst = hart_fetch(h,addr);
    inst_data d = parse_instruction(&h, inst, addr);
    h.csrs[CYCLE]++;
    if(d.valid == false) {
        hart_trap(h,EXC_ILLEGAL_INSTRUCTION, inst, false);
        return;
    }
    hart_execute(h,d);
}

void hart_execute(HART& h, inst_data inst) {
    bool success = inst.fn(&h,inst);
    if(success) {
        h.csrs[INSTRET]++;
        h.pc += 4;
    }
}

void hart_check_interrupts(HART& h) {
    uint64_t mip     = h.csrs[MIP];
    uint64_t mie     = h.csrs[MIE];
    uint64_t mstatus = h.csrs[MSTATUS];
    uint64_t sstatus = csr_read(&h,SSTATUS);
    uint64_t mideleg = h.csrs[MIDELEG];

    bool m_ie = (mstatus >> MSTATUS_MIE) & 1;
    bool s_ie = (sstatus >> SSTATUS_SIE_BIT) & 1;

    uint64_t pending = mip & mie;
    if (!pending)
        return;

    if (h.mode == PrivilegeMode::Machine) {
        if (!m_ie)
            return;

        if (pending & (1ULL << MIP_MEIP)) {
            hart_trap(h, IRQ_MEXT, 0, true);
            return;
        }
        if (pending & (1ULL << MIP_MSIP)) {
            hart_trap(h, IRQ_MSW, 0, true);
            return;
        }
        if (pending & (1ULL << MIP_MTIP)) {
            hart_trap(h, IRQ_MTIMER, 0, true);
            return;
        }
        return;
    }

    /* ---- external ---- */
    if (pending & (1ULL << MIP_SEIP)) {
        if ((mideleg & (1ULL << IRQ_SEXT)) && s_ie) {
            hart_trap(h, IRQ_SEXT, 0, true);
        } else if (m_ie) {
            hart_trap(h, IRQ_SEXT, 0, true);
        }
        return;
    }

    /* ---- software ---- */
    if (pending & (1ULL << MIP_SSIP)) {
        if ((mideleg & (1ULL << IRQ_SSW)) && s_ie) {
            hart_trap(h, IRQ_SSW, 0, true);
        } else if (m_ie) {
            hart_trap(h, IRQ_SSW, 0, true);
        }
        return;
    }

    /* ---- timer ---- */
    if (pending & (1ULL << MIP_STIP)) {
        if ((mideleg & (1ULL << IRQ_STIMER)) && s_ie) {
            hart_trap(h, IRQ_STIMER, 0, true);
        } else if (m_ie) {
            hart_trap(h, IRQ_STIMER, 0, true);
        }
        return;
    }
}

void hart_trap(HART& h, uint64_t cause, uint64_t tval, bool is_interrupt) {
    h.WFI = false;

    uint64_t trap_pc = h.pc;
    PrivilegeMode prev_mode = h.mode;

    uint64_t medeleg = h.csrs[MEDELEG];
    uint64_t mideleg = h.csrs[MIDELEG];
    bool delegate_to_s = false;
    if (prev_mode != PrivilegeMode::Machine) {
        if (is_interrupt)
            delegate_to_s = (mideleg >> cause) & 1ULL;
        else
            delegate_to_s = (medeleg >> cause) & 1ULL;
    }

    if(delegate_to_s) {
        // Supervisor
        h.mode = PrivilegeMode::Supervisor;
        uint64_t vector = ((h.csrs[STVEC] & 1 == 1 && is_interrupt) ? 4 * cause : 0);
        h.pc = (h.csrs[STVEC] & ~1) + vector;
        h.csrs[SEPC] = trap_pc & ~1;
        h.csrs[SCAUSE] = ((is_interrupt ? (1ULL << 63) : 0) | cause);
        h.csrs[STVAL] = tval;
        csr_write_mstatus(&h,MSTATUS_SPIE,MSTATUS_SPIE,csr_read_mstatus(&h,MSTATUS_SIE,MSTATUS_SIE));
        csr_write_mstatus(&h,MSTATUS_SIE,MSTATUS_SIE,0);
        csr_write_mstatus(&h,MSTATUS_SPP,MSTATUS_SPP,(prev_mode != PrivilegeMode::User));
    } else {
        // Machine
        h.mode = PrivilegeMode::Machine;
        uint64_t vector = ((h.csrs[MTVEC] & 1 == 1 && is_interrupt) ? 4 * cause : 0);
        h.pc = (h.csrs[MTVEC] & ~1) + vector;
        h.csrs[MEPC] = trap_pc & ~1;
        h.csrs[MCAUSE] = ((is_interrupt ? (1ULL << 63) : 0) | cause);
        h.csrs[MTVAL] = tval;
        csr_write_mstatus(&h,MSTATUS_MPIE,MSTATUS_MPIE,csr_read_mstatus(&h,MSTATUS_MIE,MSTATUS_MIE));
        csr_write_mstatus(&h,MSTATUS_MIE,MSTATUS_MIE,0);
        csr_write_mstatus(&h,MSTATUS_MPP_LOW,MSTATUS_MPP_HIGH,(uint8_t)prev_mode);
    }
}
uint64_t csr_read(HART *hart, uint16_t csr) {
    switch(csr) {
        case SSTATUS:
            return hart->csrs[MSTATUS] & SSTATUS_MASK;
        default:
            return hart->csrs[csr];
    }
}
void csr_write(HART *hart, uint16_t csr, uint64_t val) {
    if(csr == SSTATUS) {
        uint64_t mst = hart->csrs[MSTATUS] & ~SSTATUS_MASK;
        hart->csrs[MSTATUS] = mst | (val & SSTATUS_MASK);
    } else if(csr >= PMPCFG0 && csr <= PMPCFG15) {
        auto vals = pmp_get_num_cfgs(val);
        std::vector<uint8_t> update;
        uint64_t old_val = hart->csrs[csr];
        uint64_t new_val = val;
        for(uint8_t i : vals) {
            if(pmp_check_locked_cfg(hart,i)) {
                uint64_t mask = 0xFF << (i*8);
                new_val &= ~mask;
                new_val |= (old_val & mask);
            } else {
                update.push_back(i);
            }
        }
        hart->csrs[csr] = new_val;
        for(uint8_t i : update) {
            pmp_update(hart, i);
        }
    } else if(csr >= PMPADDR0 && csr <= PMPADDR63) {
        uint8_t i = csr - PMPADDR0;
        if(!pmp_check_locked_addr(hart,i)) hart->csrs[csr] = val;
    } else {
        hart->csrs[csr] = val;
    }
}
