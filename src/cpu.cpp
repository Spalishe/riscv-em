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
    for(int i = 0; i < 32; i++) {h.GPR[i] = 0; h.FPR[i] = 0.0;}
    for(int i = 0; i < 4069; i++) {h.csrs[i] = 0;}

	h.pc = DRAM_BASE;
	h.GPR[10] = h.id;

	h.GPR[11] = dtb_path;

	h.csrs[MISA] = riscv_mkmisa("imasu");
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
        auto pa_opt = mmu_translate(*h.mmio->mmu, &h, _pc, AccessType::EXECUTE);
        if(!pa_opt) {
            return 0; 
        }
        uint64_t pa = *pa_opt;

        bool out = h.mmio->ram->mmap->copy_mem_safe(pa, 32, &h.fetch_buffer);
        if(!out) return 0;

        h.fetch_pc = _pc;
    }
	uint8_t fetch_indx = (_pc - h.fetch_pc) / 4;
	return h.fetch_buffer[fetch_indx];
    /*auto phys = mmu_translate(*h.mmio->mmu, &h, _pc, AccessType::EXECUTE);
    if (!phys.has_value()) return 0;
    uint64_t addr = phys.value();
    return (uint32_t)dram_load(h.mmio->mmu->dram,addr,32);*/
}

void hart_step(HART& h) {
    uint32_t inst = hart_fetch(h,h.pc);
    inst_data d = parse_instruction(&h, inst, h.pc);
    h.csrs[CYCLE]++;
    if(d.valid == false) {
        hart_trap(h,EXC_ILLEGAL_INSTRUCTION, inst, false);
        return;
    }
    h.pc_hits[h.pc]++;

    if(h.pc_hits[h.pc] >= HART_INST_EXECUTION_COUNT_CAP && !h.is_creating_block && !d.canChangePC) {
        InstructionBlock* block = &h.blocks[h.pc % 1024];
        if(block->valid == false || block->start_pc != h.pc) {
            h.is_creating_block = true;
            h.block_creation_start_pc = h.pc;
            InstructionBlock* block = &h.blocks[h.block_creation_start_pc % 1024];
            block->valid = true;
            block->start_pc = h.block_creation_start_pc;
            block->instrs.clear();
            block->size = 0;
        }
    }
    if(h.is_creating_block) {
        InstructionBlock* block = &h.blocks[h.block_creation_start_pc % 1024];
        if(block->valid == false || block->start_pc != h.block_creation_start_pc) {
            // Someone changed blocks in runtime!!
            // Invalidate ALL blocks, just in case
            for(int i = 0; i < 1024; i++) {
                InstructionBlock* block = &h.blocks[i];
                block->valid = false;
            }
            h.pc_hits.clear();
            h.is_creating_block = false;
            return;
        } else if(d.canChangePC) {
            // We got a branch maybe
            h.is_creating_block = false;
            return;
        } else {
            // Normal case
            if(d.valid) {
                block->instrs.push_back(d);
                inst_ret ret = hart_execute(h,d);
                if(ret.increasePC == false) {
                    // Probably branch case / TRAP
                    // Stop compiling, invalidate this block
                    h.is_creating_block = false;
                    block->valid = false;
                } else {
                    block->size += ret.isCompressed ? 2 : 4;
                }
            }
        }

    } else {
        InstructionBlock* block = &h.blocks[h.pc % 1024];
        if(block->valid == true && block->start_pc == h.pc) {
            // Execute block
            for(inst_data dat : block->instrs) {
                if(h.pc < block->start_pc || h.pc >= block->start_pc + block->size){
                    // Interrupt
                    break;
                }
                inst_ret ret = hart_execute(h,dat);
                if(ret.increasePC == false) {
                    // Probably branch case / TRAP
                    return;
                }
            }
        } else hart_execute(h,d);
    }
}
inst_ret hart_execute(HART& h, inst_data inst) {
    inst_ret success = inst.fn(&h,inst);
    if(success.increasePC) {
        h.csrs[INSTRET]++;
        h.pc += success.isCompressed ? 2 : 4;
    }
    return success;
}

void hart_check_interrupts(HART& h) {
    uint64_t mip = h.csrs[MIP];
    uint64_t mie = h.csrs[MIE];
    uint64_t sip = csr_read(&h,SIP);
    uint64_t sie = csr_read(&h,SIE);
    uint64_t mideleg = h.csrs[MIDELEG];

    bool m_global = csr_read_mstatus(&h,MSTATUS_MIE,MSTATUS_MIE);
    bool s_global = csr_read_mstatus(&h,MSTATUS_SIE,MSTATUS_SIE);

    if (m_global) {
        uint64_t pending = mip & mie & ~mideleg;

        if (pending) {
            for (int irq : irq_priority) {
                if (pending & (1ULL << irq)) {
                    hart_trap(h, irq, 0, true);
                    return;
                }
            }
        }
    }

    if (h.mode <= PrivilegeMode::Supervisor && s_global) {
        uint64_t pending = sip & sie & mideleg;
        if (pending) {
            for (int irq : irq_priority) {
                if (pending & (1ULL << irq)) {
                    hart_trap(h, irq, 0, true);
                    return;
                }
            }
        }
    }

    return;
}

bool hart_have_local_pending(HART& h) {
    uint64_t mip = h.csrs[MIP];
    uint64_t mie = h.csrs[MIE];
    uint64_t sip = csr_read(&h,SIP);
    uint64_t sie = csr_read(&h,SIE);
    uint64_t mideleg = h.csrs[MIDELEG];

    uint64_t pending = mip & mie & ~mideleg;
    uint64_t pending_s = sip & sie & mideleg;

    if (h.mode > PrivilegeMode::Supervisor) pending_s = 0;

    return pending | pending_s;
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
        case SIE:
            return hart->csrs[MIE] & SE_MASK;
        case SIP:
            return hart->csrs[MIP] & SE_MASK;
        case FFLAGS:
            return hart->csrs[FCSR] & 0x1F;
        case FRM:
            return (hart->csrs[FCSR] >> 5) & 0x7;
        default:
            return hart->csrs[csr];
    }
}
void csr_write(HART *hart, uint16_t csr, uint64_t val) {
    if(csr == SSTATUS) {
        uint64_t mst = hart->csrs[MSTATUS] & ~SSTATUS_MASK;
        hart->csrs[MSTATUS] = mst | (val & SSTATUS_MASK);
    } else if(csr == SIP) {
        uint64_t mst = hart->csrs[MIP] & ~SE_MASK;
        hart->csrs[MIP] = mst | (val & SE_MASK);
    } else if(csr == SIE) {
        uint64_t mst = hart->csrs[MIE] & ~SE_MASK;
        hart->csrs[MIE] = mst | (val & SE_MASK);
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
    } else if(csr == MSTATUS) {
        uint64_t mask = 1ULL << 32 | 1ULL << 33 | 1ULL << 34 | 1ULL << 35;
        val &= ~mask;
        val |= hart->csrs[MSTATUS] & mask;
        hart->csrs[MSTATUS] = val;
    } else if(csr == FFLAGS) {
        uint64_t cs = hart->csrs[FCSR];
        cs &= ~31;
        cs |= (val & 31);
        hart->csrs[FCSR] = cs;
    } else if(csr == FRM) {
        uint64_t cs = hart->csrs[FCSR];
        cs &= ~224;
        cs |= ((val & 7)<<5);
        hart->csrs[FCSR] = cs;
    } else if(csr == FCSR) {
        hart->csrs[FCSR] = val & 0xFF;
    } else if(csr == MISA || csr == MARCHID || csr == MVENDORID || csr == MIMPID || csr == MHARTID) {
        // RO: nop
    } else {
        hart->csrs[csr] = val;
    }
}
