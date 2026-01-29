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

#include "../include/cpu.h"
#include "../include/csr.h"
#include <iostream>

// ZiCSR

bool compatible_mode(uint64_t csr_level, uint64_t cur_mode) {
    if(csr_level == 3) {
        return cur_mode == 3;
    } else if(csr_level == 1) {
        return (cur_mode == 3) || (cur_mode == 1);
    } else if(csr_level == 0) {
        return (cur_mode == 3) || (cur_mode == 1) || (cur_mode == 0);
    }
    return true;
}

bool csr_accessible(uint16_t csr_addr, uint64_t current_priv, bool write) {
    uint64_t csr_level;

    if (csr_addr >= 0x000 && csr_addr <= 0x0FF) csr_level = 0;        // User
    else if (csr_addr >= 0x100 && csr_addr <= 0x1FF) csr_level = 1;   // Supervisor
    else if (csr_addr >= 0x200 && csr_addr <= 0x2FF) csr_level = 2;   // Hypervisor
    else if (csr_addr >= 0x300 && csr_addr <= 0x3FF) csr_level = 3;   // Machine
    else if (csr_addr >= 0xB00 && csr_addr <= 0xBFF) csr_level = 3;   // Machine counters
    else if (csr_addr >= 0xC00 && csr_addr <= 0xCFF) csr_level = 0;   // User counters (cycle/time)
    else if (csr_addr >= 0xF00 && csr_addr <= 0xFFF) csr_level = 3;   // Machine info
    else csr_level = 0;

    if (!compatible_mode(csr_level,current_priv)) {
        return false;
    }

    if (csr_addr == 0xF11
        || csr_addr == 0xF12
        || csr_addr == 0xF13
        || csr_addr == 0xF14
        || csr_addr == 0xF15
        || csr_addr == 0xC00
        || csr_addr == 0xC01
        || csr_addr == 0xC02
        || (csr_addr >= 0xC03 && csr_addr <= 0xC1F)
        || csr_addr == 0xC80
        || csr_addr == 0xC81
        || csr_addr == 0xC82
        || (csr_addr >= 0xC83 && csr_addr <= 0xC9F)
    ) {
        return !write; // RO
    }

    return true;
}
void exec_CSRRW(HART *hart, inst_data& inst) {
    uint64_t mstatus = hart->csrs[MSTATUS];
    bool tvm = (mstatus >> 20) & 1; // TVM bit
    if(hart->mode == 1 && tvm && inst.imm == SATP) {
        hart_trap(*hart,EXC_ILLEGAL_INSTRUCTION,inst.inst,false);
        return;
    }
    if(!csr_accessible(inst.imm,hart->mode,true)) {
        hart_trap(*hart,EXC_ILLEGAL_INSTRUCTION,inst.inst,false);
        return;
    }

    uint64_t init_val = hart->csrs[inst.imm];
    if(inst.rs1 != 0) {
        hart->csrs[inst.imm] = hart->GPR[inst.rs1];
    }
    if (inst.rd != 0) {
        hart->GPR[inst.rd] = init_val;
    }
}
void exec_CSRRS(HART *hart, inst_data& inst) {
    uint64_t mstatus = hart->csrs[MSTATUS];
    bool tvm = (mstatus >> 20) & 1; // TVM bit
    if(hart->mode == 1 && tvm && inst.imm == SATP) {
        hart_trap(*hart,EXC_ILLEGAL_INSTRUCTION,inst.inst,false);
        return;
    }
    if(!csr_accessible(inst.imm,hart->mode,(inst.rs1 != 0))) {
        hart_trap(*hart,EXC_ILLEGAL_INSTRUCTION,inst.inst,false);
        return;
    }

    uint64_t init_val = hart->csrs[inst.imm];
    if (inst.rs1 != 0) {
        uint64_t mask = hart->GPR[inst.rs1];
        hart->csrs[inst.imm] = init_val | mask;
    }
    hart->GPR[inst.rd] = init_val;
}
void exec_CSRRC(HART *hart, inst_data& inst) {
    uint64_t mstatus = hart->csrs[MSTATUS];
    bool tvm = (mstatus >> 20) & 1; // TVM bit
    if(hart->mode == 1 && tvm && inst.imm == SATP) {
        hart_trap(*hart,EXC_ILLEGAL_INSTRUCTION,inst.inst,false);
        return;
    }
    if(!csr_accessible(inst.imm,hart->mode,(inst.rs1 != 0))) {
        hart_trap(*hart,EXC_ILLEGAL_INSTRUCTION,inst.inst,false);
        return;
    }

    uint64_t init_val = hart->csrs[inst.imm];
    if (inst.rs1 != 0) {
        uint64_t mask = hart->GPR[inst.rs1];
        hart->csrs[inst.imm] = init_val & ~mask;
    }
    hart->GPR[inst.rd] = init_val;
}

void exec_CSRRWI(HART *hart, inst_data& inst) {
    uint64_t mstatus = hart->csrs[MSTATUS];
    bool tvm = (mstatus >> 20) & 1; // TVM bit
    if(hart->mode == 1 && tvm && inst.imm == SATP) {
        hart_trap(*hart,EXC_ILLEGAL_INSTRUCTION,inst.inst,false);
        return;
    }
    if(!csr_accessible(inst.imm,hart->mode,inst.rs1 != 0)) {
        hart_trap(*hart,EXC_ILLEGAL_INSTRUCTION,inst.inst,false);
        return;
    }

    if (inst.rd != 0) {
        hart->GPR[inst.rd] = hart->csrs[inst.imm];
    }
    hart->csrs[inst.imm] = inst.rs1;
}
void exec_CSRRSI(HART *hart, inst_data& inst) {
    uint64_t mstatus = hart->csrs[MSTATUS];
    bool tvm = (mstatus >> 20) & 1; // TVM bit
    if(hart->mode == 1 && tvm && inst.imm == SATP) {
        hart_trap(*hart,EXC_ILLEGAL_INSTRUCTION,inst.inst,false);
        return;
    }
    if(!csr_accessible(inst.imm,hart->mode,(inst.rs1 != 0))) {
        hart_trap(*hart,EXC_ILLEGAL_INSTRUCTION,inst.inst,false);
        return;
    }

    uint64_t init_val = hart->csrs[inst.imm];
    if (inst.rs1 != 0) {
        uint64_t mask = inst.rs1;
        hart->csrs[inst.imm] = init_val | mask;
    }
    hart->GPR[inst.rd] = init_val;
}
void exec_CSRRCI(HART *hart, inst_data& inst) {
    uint64_t mstatus = hart->csrs[MSTATUS];
    bool tvm = (mstatus >> 20) & 1; // TVM bit
    if(hart->mode == 1 && tvm && inst.imm == SATP) {
        hart_trap(*hart,EXC_ILLEGAL_INSTRUCTION,inst.inst,false);
        return;
    }
    if(!csr_accessible(inst.imm,hart->mode,(inst.rs1 != 0))) {
        hart_trap(*hart,EXC_ILLEGAL_INSTRUCTION,inst.inst,false);
        return;
    }

    uint64_t init_val = hart->csrs[inst.imm];
    if (inst.rs1 != 0) {
        uint64_t mask = inst.rs1;
        hart->csrs[inst.imm] = init_val & ~mask;
    }
    hart->GPR[inst.rd] = init_val;
}