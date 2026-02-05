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

#include "../../include/cpu.hpp"
#include "../../include/csr.h"
#include "../../include/decode.h"

// ZiCSR

bool compatible_mode(PrivilegeMode csr_level, PrivilegeMode cur_mode) {
    if(csr_level == PrivilegeMode::Machine) {
        return cur_mode == PrivilegeMode::Machine;
    } else if(csr_level == PrivilegeMode::Supervisor) {
        return (cur_mode == PrivilegeMode::Machine) || (cur_mode == PrivilegeMode::Supervisor);
    } else if(csr_level == PrivilegeMode::User) {
        return (cur_mode == PrivilegeMode::Machine) || (cur_mode == PrivilegeMode::Supervisor) || (cur_mode == PrivilegeMode::User);
    }
    return true;
}

bool csr_accessible(uint16_t csr_addr, PrivilegeMode current_priv, bool write) {
    PrivilegeMode csr_level;

    if (csr_addr >= 0x000 && csr_addr <= 0x0FF) csr_level = PrivilegeMode::User;              // User
    else if (csr_addr >= 0x100 && csr_addr <= 0x1FF) csr_level = PrivilegeMode::Supervisor;   // Supervisor
    else if (csr_addr >= 0x200 && csr_addr <= 0x2FF) csr_level = PrivilegeMode::Hypervisor;   // Hypervisor
    else if (csr_addr >= 0x300 && csr_addr <= 0x3FF) csr_level = PrivilegeMode::Machine;      // Machine
    else if (csr_addr >= 0xB00 && csr_addr <= 0xBFF) csr_level = PrivilegeMode::Machine;      // Machine counters
    else if (csr_addr >= 0xC00 && csr_addr <= 0xCFF) csr_level = PrivilegeMode::User;         // User counters (cycle/time)
    else if (csr_addr >= 0xF00 && csr_addr <= 0xFFF) csr_level = PrivilegeMode::Machine;      // Machine info
    else csr_level = PrivilegeMode::User;

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
inst_ret exec_CSRRW(HART *hart, inst_data& inst) {
    uint64_t mstatus = hart->csrs[MSTATUS];
    bool tvm = (mstatus >> 20) & 1; // TVM bit
    if(hart->mode == PrivilegeMode::Supervisor && tvm && inst.imm == SATP) {
        hart_trap(*hart,EXC_ILLEGAL_INSTRUCTION,inst.inst,false);
        return false;
    }
    if(!csr_accessible(inst.imm,hart->mode,true)) {
        hart_trap(*hart,EXC_ILLEGAL_INSTRUCTION,inst.inst,false);
        return false;
    }

    uint64_t init_val = csr_read(hart,inst.imm);
    if(inst.rs1 != 0) {
	    csr_write(hart,inst.imm,hart->GPR[inst.rs1]);
    }
    if (inst.rd != 0) {
        hart->GPR[inst.rd] = init_val;
    }
    return true;
}
inst_ret exec_CSRRS(HART *hart, inst_data& inst) {
    uint64_t mstatus = hart->csrs[MSTATUS];
    bool tvm = (mstatus >> 20) & 1; // TVM bit
    if(hart->mode == PrivilegeMode::Supervisor && tvm && inst.imm == SATP) {
        hart_trap(*hart,EXC_ILLEGAL_INSTRUCTION,inst.inst,false);
        return false;
    }
    if(!csr_accessible(inst.imm,hart->mode,(inst.rs1 != 0))) {
        hart_trap(*hart,EXC_ILLEGAL_INSTRUCTION,inst.inst,false);
        return false;
    }

    uint64_t init_val = csr_read(hart,inst.imm);
    if (inst.rs1 != 0) {
        uint64_t mask = hart->GPR[inst.rs1];
	    csr_write(hart,inst.imm,init_val | mask);
    }
    hart->GPR[inst.rd] = init_val;
    return true;
}
inst_ret exec_CSRRC(HART *hart, inst_data& inst) {
    uint64_t mstatus = hart->csrs[MSTATUS];
    bool tvm = (mstatus >> 20) & 1; // TVM bit
    if(hart->mode == PrivilegeMode::Supervisor && tvm && inst.imm == SATP) {
        hart_trap(*hart,EXC_ILLEGAL_INSTRUCTION,inst.inst,false);
        return false;
    }
    if(!csr_accessible(inst.imm,hart->mode,(inst.rs1 != 0))) {
        hart_trap(*hart,EXC_ILLEGAL_INSTRUCTION,inst.inst,false);
        return false;
    }

    uint64_t init_val = csr_read(hart,inst.imm);
    if (inst.rs1 != 0) {
        uint64_t mask = hart->GPR[inst.rs1];
	    csr_write(hart,inst.imm,init_val & ~mask);
    }
    hart->GPR[inst.rd] = init_val;
    return true;
}

inst_ret exec_CSRRWI(HART *hart, inst_data& inst) {
    uint64_t mstatus = hart->csrs[MSTATUS];
    bool tvm = (mstatus >> 20) & 1; // TVM bit
    if(hart->mode == PrivilegeMode::Supervisor && tvm && inst.imm == SATP) {
        hart_trap(*hart,EXC_ILLEGAL_INSTRUCTION,inst.inst,false);
        return false;
    }
    if(!csr_accessible(inst.imm,hart->mode,inst.rs1 != 0)) {
        hart_trap(*hart,EXC_ILLEGAL_INSTRUCTION,inst.inst,false);
        return false;
    }

    if (inst.rd != 0) {
        hart->GPR[inst.rd] = csr_read(hart,inst.imm);
    }
	csr_write(hart,inst.imm,inst.rs1);
    return true;
}
inst_ret exec_CSRRSI(HART *hart, inst_data& inst) {
    uint64_t mstatus = hart->csrs[MSTATUS];
    bool tvm = (mstatus >> 20) & 1; // TVM bit
    if(hart->mode == PrivilegeMode::Supervisor && tvm && inst.imm == SATP) {
        hart_trap(*hart,EXC_ILLEGAL_INSTRUCTION,inst.inst,false);
        return false;
    }
    if(!csr_accessible(inst.imm,hart->mode,(inst.rs1 != 0))) {
        hart_trap(*hart,EXC_ILLEGAL_INSTRUCTION,inst.inst,false);
        return false;
    }

    uint64_t init_val = csr_read(hart,inst.imm);
    if (inst.rs1 != 0) {
        uint64_t mask = inst.rs1;
	    csr_write(hart,inst.imm,init_val | mask);
    }
    hart->GPR[inst.rd] = init_val;
    return true;
}
inst_ret exec_CSRRCI(HART *hart, inst_data& inst) {
    uint64_t mstatus = hart->csrs[MSTATUS];
    bool tvm = (mstatus >> 20) & 1; // TVM bit
    if(hart->mode == PrivilegeMode::Supervisor && tvm && inst.imm == SATP) {
        hart_trap(*hart,EXC_ILLEGAL_INSTRUCTION,inst.inst,false);
        return false;
    }
    if(!csr_accessible(inst.imm,hart->mode,(inst.rs1 != 0))) {
        hart_trap(*hart,EXC_ILLEGAL_INSTRUCTION,inst.inst,false);
        return false;
    }

    uint64_t init_val = csr_read(hart,inst.imm);
    if (inst.rs1 != 0) {
        uint64_t mask = inst.rs1;
	    csr_write(hart,inst.imm,init_val & ~mask);
    }
    hart->GPR[inst.rd] = init_val;
    return true;
}