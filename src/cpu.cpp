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
#include "../include/mmu.hpp"
#include <stdint.h>
#include <string>
#include <fstream>
#include "../include/devices/mmio.h"
#include "../include/machine.hpp"
#include "../include/decode.h"
#include "../include/csr.h"
#include "../include/devices/plic.hpp"
#include "../include/devices/aclint.hpp"
#include <cstdio>
#include <cstdarg>
#include <format>
#include <sstream>
#include <iomanip>
#include <functional>

void hart_reset(HART& h, uint64_t dtb_path) {
    for(int i = 0; i < 32; i++) {
        h.GPR[i] = 0; 
        #ifdef USE_FPU
        h.FPR[i] = 0.0;
        #endif
    }
    for(int i = 0; i < 4069; i++) {h.csrs[i] = 0;}

	h.pc = DRAM_BASE;
	h.GPR[10] = h.id;

	h.GPR[11] = dtb_path;

    std::string fpustr = "";
    #ifdef USE_FPU
    fpustr = "fd";
    #endif
	h.csrs[CSR_MISA] = riscv_mkmisa(std::format("imasu%s",fpustr).c_str());
	h.csrs[CSR_MVENDORID] = 0; 
	h.csrs[CSR_MARCHID] = 0; 
	h.csrs[CSR_MIMPID] = 0;
	h.csrs[CSR_MHARTID] = h.id;
	h.csrs[CSR_MIDELEG] = 5188;
    h.csrs[CSR_MEDELEG] = 0x4B109;
	//h.csrs[MIP] = 0x80;
    //timecmp_init(&h.stimecmp,&h.aclint->mtime);
    h.ie = 0;
    h.ip = 0;
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
    h.instr_cache[(h.pc >> 2) & 0x1FFF] = d;
    h.csrs[CSR_CYCLE]++;
    if(d.valid == false) {
        hart_trap(h,EXC_ILLEGAL_INSTRUCTION, inst, false);
        return;
    }
    uint16_t idx = (h.pc >> 2) & (256 - 1);
    h.pc_hits[idx]++;

    #ifdef USE_GDB
    if((h.pc_hits[idx] >= HART_INST_EXECUTION_COUNT_CAP && !h.aclint->cpu.gdb) && !h.is_creating_block && !d.canChangePC) {
    #else
    if(h.pc_hits[idx] >= HART_INST_EXECUTION_COUNT_CAP && !h.is_creating_block && !d.canChangePC) {
    #endif
        InstructionBlock* block = &h.blocks[h.pc % 256];
        if(block->valid == false && block->start_pc != h.pc) {
            h.is_creating_block = true;
            h.block_creation_start_pc = h.pc;
            InstructionBlock* block = &h.blocks[h.block_creation_start_pc % 256];
            block->valid = true;
            block->start_pc = h.block_creation_start_pc;
            memset(block->instrs,0,sizeof(block->instrs));
            block->size = 0;
            block->length = 0;
        }
    }
    if(h.is_creating_block) {
        InstructionBlock* block = &h.blocks[h.block_creation_start_pc % 256];
        if(block->valid == false || block->start_pc != h.block_creation_start_pc) {
            // Someone changed blocks in runtime!!
            // Invalidate ALL blocks, just in case
            for(int i = 0; i < 256; i++) {
                InstructionBlock* block = &h.blocks[i];
                block->valid = false;
            }
            memset(h.pc_hits,0,sizeof(h.pc_hits));
            h.is_creating_block = false;
            return;
        } else if(d.canChangePC) {
            // We got a branch maybe
            h.is_creating_block = false;
            return;
        } else if(block->length >= 128) {
            // We got block size limit
            h.is_creating_block = false;
            return;
        } else {
            // Normal case
            if(d.valid) {
                inst_ret ret = hart_execute(h,d);
                if(ret.increasePC == false) {
                    // Probably branch case / TRAP
                    // Stop compiling, invalidate this block
                    h.is_creating_block = false;
                    block->valid = false;
                } else {
                    block->instrs[block->length] = d;
                    block->size += ret.isCompressed ? 2 : 4;
                    block->length++;
                }
            }
        }

    } else {
        InstructionBlock& block = h.blocks[h.pc % 256];
        if(block.valid == true && block.start_pc == h.pc) {
            // Execute block
            for(int i = 0; i < 256; i++) {
                inst_data dat = block.instrs[i];
                if(h.pc < block.start_pc || h.pc >= block.start_pc + block.size){
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
inst_ret hart_execute(HART& h, inst_data& inst) {
    inst_ret success = inst.fn(&h,inst);
    if(success.increasePC) {
        h.csrs[CSR_INSTRET]++;
        h.pc += success.isCompressed ? 2 : 4;
    }
    return success;
}

void hart_check_interrupts(HART& h) {
    uint64_t mip = h.ip;
    uint64_t mie = h.ie;
    uint64_t mideleg = h.csrs[CSR_MIDELEG];

    bool m_global = h.status.fields.MIE;
    bool s_global = h.status.fields.SIE;

    bool take_m =
    (h.mode == PrivilegeMode::Machine && m_global) ||
    (h.mode < PrivilegeMode::Machine);

    if (take_m) {
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
    bool take_s =
    (h.mode == PrivilegeMode::Supervisor && s_global) ||
    (h.mode < PrivilegeMode::Supervisor);

    if (take_s) {
        uint64_t pending = (mip & mideleg) & mie;
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
    uint64_t mip = h.ip;
    uint64_t mie = h.ie;
    uint64_t sip = mip & SE_MASK;
    uint64_t sie = mie & SE_MASK;
    uint64_t mideleg = h.csrs[CSR_MIDELEG];

    uint64_t pending = mip & mie & ~mideleg;
    uint64_t pending_s = sip & sie & mideleg;

    if (h.mode > PrivilegeMode::Supervisor) pending_s = 0;

    return pending | pending_s;
}

void hart_trap(HART& h, uint64_t cause, uint64_t tval, bool is_interrupt) {
    h.WFI = false;

    uint64_t trap_pc = h.pc;
    PrivilegeMode prev_mode = h.mode;

    uint64_t medeleg = h.csrs[CSR_MEDELEG];
    uint64_t mideleg = h.csrs[CSR_MIDELEG];
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
        uint64_t vector = (((h.csrs[CSR_STVEC] & 1) == 1 && is_interrupt) ? 4 * cause : 0);
        h.pc = (h.csrs[CSR_STVEC] & ~3) + vector;
        h.csrs[CSR_SEPC] = trap_pc & ~3;
        h.csrs[CSR_SCAUSE] = ((is_interrupt ? (1ULL << 63) : 0) | cause);
        h.csrs[CSR_STVAL] = tval;
        h.status.fields.SPIE = h.status.fields.SIE;
        h.status.fields.SIE = 0;
        h.status.fields.SPP = (prev_mode != PrivilegeMode::User);
    } else {
        // Machine
        h.mode = PrivilegeMode::Machine;
        uint64_t vector = (((h.csrs[CSR_MTVEC] & 1) == 1 && is_interrupt) ? 4 * cause : 0);
        h.pc = (h.csrs[CSR_MTVEC] & ~3) + vector;
        h.csrs[CSR_MEPC] = trap_pc & ~3;
        h.csrs[CSR_MCAUSE] = ((is_interrupt ? (1ULL << 63) : 0) | cause);
        h.csrs[CSR_MTVAL] = tval;
        h.status.fields.MPIE = h.status.fields.MIE;
        h.status.fields.MIE = 0;
        h.status.fields.MPP = (uint8_t)prev_mode;
    }
}
uint64_t csr_read(HART *hart, uint16_t csr) {
    switch(csr) {
        case CSR_MSTATUS:
            return hart->status.raw;
        case CSR_SSTATUS:
            return hart->status.raw & SSTATUS_MASK;
        case CSR_SIE:
            return hart->ie & SE_MASK;
        case CSR_SIP:
            return hart->ip & SE_MASK;
        case CSR_MIE:
            return hart->ie;
        case CSR_MIP:
            return hart->ip;
        case CSR_FFLAGS:
            return hart->csrs[CSR_FCSR] & 0x1F;
        case CSR_TIME:
            //return timer_get(&hart->aclint->mtime);
            return hart->aclint->mtime;
        case CSR_FRM:
            return (hart->csrs[CSR_FCSR] >> 5) & 0x7;
        case CSR_STIMECMP:
            //return timecmp_get(&hart->stimecmp);
            return hart->stimecmp;
        default:
            return hart->csrs[csr];
    }
}
void csr_write(HART *hart, uint16_t csr, uint64_t val) {
    if(csr == CSR_SSTATUS) {
        uint64_t mst = hart->status.raw & ~SSTATUS_MASK;
        hart->status.raw = mst | (val & SSTATUS_MASK);
    } else if(csr == CSR_SIP) {
        uint64_t mst = hart->ip & ~SE_MASK;
        hart->ip = mst | (val & SE_MASK);
    } else if(csr == CSR_SIE) {
        uint64_t mst = hart->ie & ~SE_MASK;
        hart->ie = mst | (val & SE_MASK);
    } else if(csr == CSR_MIP) {
        hart->ip = val;
    } else if(csr == CSR_MIE) {
        hart->ie = val;
    } else if(csr >= CSR_PMPCFG0 && csr <= CSR_PMPCFG15) {
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
    } else if(csr >= CSR_PMPADDR0 && csr <= CSR_PMPADDR63) {
        uint8_t i = csr - CSR_PMPADDR0;
        if(!pmp_check_locked_addr(hart,i)) hart->csrs[csr] = val;
    } else if(csr == CSR_MSTATUS) {
        uint64_t mask = 1ULL << 32 | 1ULL << 33 | 1ULL << 34 | 1ULL << 35;
        val &= ~mask;
        val |= hart->status.raw & mask;
        hart->status.raw = val;
    } else if(csr == CSR_FFLAGS) {
        uint64_t cs = hart->csrs[CSR_FCSR];
        cs &= ~31;
        cs |= (val & 31);
        hart->csrs[CSR_FCSR] = cs;
    } else if(csr == CSR_FRM) {
        uint64_t cs = hart->csrs[CSR_FCSR];
        cs &= ~224;
        cs |= ((val & 7)<<5);
        hart->csrs[CSR_FCSR] = cs;
    } else if(csr == CSR_FCSR) {
        hart->csrs[CSR_FCSR] = val & 0xFF;
    } else if(csr == CSR_STIMECMP) {
        //timecmp_set(&hart->stimecmp,val);
        hart->stimecmp = val;
        hart->aclint->update_mip(hart);
    } else if(csr == CSR_MISA || csr == CSR_MARCHID || csr == CSR_MVENDORID || csr == CSR_MIMPID || csr == CSR_MHARTID) {
        // RO: nop
    } else {
        hart->csrs[csr] = val;
    }
}
