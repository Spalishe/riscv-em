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

#include "../include/instset.h"
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
void exec_CSRRW(struct HART *hart, uint32_t inst, CACHE_DecodedOperands* opers) {
    uint64_t rd_l = opers->s ? opers->rd : rd(inst);
    uint64_t rs1_l = opers->s ? opers->rs1 : rs1(inst);
    uint64_t imm = opers->s ? opers->imm : imm_Zicsr(inst);

    uint64_t mstatus = hart->csrs[MSTATUS];
    bool tvm = (mstatus >> 20) & 1; // TVM bit
    if(hart->mode == 1 && tvm && imm == SATP) {
        if(hart->dbg) std::cout << "TVM ENABLED" << std::endl;
        hart->cpu_trap(EXC_ILLEGAL_INSTRUCTION,inst,false);
    }
    if(!csr_accessible(imm,hart->mode,true)) {
        if(hart->dbg) std::cout << "MODE: " << (uint32_t)hart->mode << std::endl;
        hart->cpu_trap(EXC_ILLEGAL_INSTRUCTION,inst,false);
    }

    uint64_t init_val = hart->csr_read(imm);
    if (rs1_l != 0)
        hart->csr_write(imm,hart->regs[rs1_l]);
        
    if (rd_l != 0) {
        hart->regs[rd_l] = init_val;
    }

    if(!opers->s){
        opers->rd = rd_l;
        opers->rs1 = rs1_l;
        opers->imm = imm;
        opers->s = true;
    }
	if(hart->dbg) hart->print_d("{0x%.8X} [CSRRW] rs1: %d; rd: %d; imm: int %d uint %u",hart->pc,rs1_l,rd_l,(int64_t) imm, imm);
}
void exec_CSRRS(struct HART *hart, uint32_t inst, CACHE_DecodedOperands* opers) {
    uint64_t rd_l = opers->s ? opers->rd : rd(inst);
    uint64_t rs1_l = opers->s ? opers->rs1 : rs1(inst);
    uint64_t imm = opers->s ? opers->imm : imm_Zicsr(inst);

    uint64_t mstatus = hart->csrs[MSTATUS];
    bool tvm = (mstatus >> 20) & 1; // TVM bit
    if(hart->mode == 1 && tvm && imm == SATP) {
        if(hart->dbg) std::cout << "TVM ENABLED" << std::endl;
        hart->cpu_trap(EXC_ILLEGAL_INSTRUCTION,inst,false);
    }
    if(!csr_accessible(imm,hart->mode,(rs1_l != 0))) {
        if(hart->dbg) std::cout << "MODE: " << (uint32_t)hart->mode << std::endl;
        hart->cpu_trap(EXC_ILLEGAL_INSTRUCTION,inst,false);
    }

    uint64_t init_val = hart->csr_read(imm);
    if (rs1_l != 0) {
        uint64_t mask = hart->regs[rs1_l];
        hart->csr_write(imm,init_val | mask);
    }
    hart->regs[rd_l] = init_val;

    if(!opers->s){
        opers->rd = rd_l;
        opers->rs1 = rs1_l;
        opers->imm = imm;
        opers->s = true;
    }
	if(hart->dbg) hart->print_d("{0x%.8X} [CSRRS] rs1: %d; rd: %d; imm: int %d uint %u",hart->pc,rs1_l,rd_l,(int64_t) imm, imm);
}
void exec_CSRRC(struct HART *hart, uint32_t inst, CACHE_DecodedOperands* opers) {
    uint64_t rd_l = opers->s ? opers->rd : rd(inst);
    uint64_t rs1_l = opers->s ? opers->rs1 : rs1(inst);
    uint64_t imm = opers->s ? opers->imm : imm_Zicsr(inst);

    uint64_t mstatus = hart->csrs[MSTATUS];
    bool tvm = (mstatus >> 20) & 1; // TVM bit
    if(hart->mode == 1 && tvm && imm == SATP) {
        if(hart->dbg) std::cout << "TVM ENABLED" << std::endl;
        hart->cpu_trap(EXC_ILLEGAL_INSTRUCTION,inst,false);
    }
    if(!csr_accessible(imm,hart->mode,(rs1_l != 0))) {
        if(hart->dbg) std::cout << "MODE: " << (uint32_t)hart->mode << std::endl;
        hart->cpu_trap(EXC_ILLEGAL_INSTRUCTION,inst,false);
    }

    uint64_t init_val = hart->csrs[imm];
    if (rs1_l != 0) {
        uint64_t mask = hart->regs[rs1_l];
        hart->csr_write(imm,init_val & ~mask);
    }
    hart->regs[rd_l] = init_val;

    if(!opers->s){
        opers->rd = rd_l;
        opers->rs1 = rs1_l;
        opers->imm = imm;
        opers->s = true;
    }
	if(hart->dbg) hart->print_d("{0x%.8X} [CSRRC] rs1: %d; rd: %d; imm: int %d uint %u",hart->pc,rs1_l,rd_l,(int64_t) imm, imm);
}

void exec_CSRRWI(struct HART *hart, uint32_t inst, CACHE_DecodedOperands* opers) {
    uint64_t rd_l = opers->s ? opers->rd : rd(inst);
    uint64_t rs1_l = opers->s ? opers->rs1 : rs1(inst);
    uint64_t imm = opers->s ? opers->imm : imm_Zicsr(inst);

    uint64_t mstatus = hart->csrs[MSTATUS];
    bool tvm = (mstatus >> 20) & 1; // TVM bit
    if(hart->mode == 1 && tvm && imm == SATP) {
        if(hart->dbg) std::cout << "TVM ENABLED" << std::endl;
        hart->cpu_trap(EXC_ILLEGAL_INSTRUCTION,inst,false);
    }
    if(!csr_accessible(imm,hart->mode,true)) {
        if(hart->dbg) std::cout << "MODE: " << (uint32_t)hart->mode << std::endl;
        hart->cpu_trap(EXC_ILLEGAL_INSTRUCTION,inst,false);
    }

    if (rd_l != 0) {
        hart->regs[rd_l] = hart->csr_read(imm);
    }
    if (rs1_l != 0)
        hart->csr_write(imm,rs1_l);

    if(!opers->s){
        opers->rd = rd_l;
        opers->rs1 = rs1_l;
        opers->imm = imm;
        opers->s = true;
    }
	if(hart->dbg) hart->print_d("{0x%.8X} [CSRRWI] rs1: %d; rd: %d; imm: int %d uint %u",hart->pc,rs1_l,rd_l,(int64_t) imm, imm);
}
void exec_CSRRSI(struct HART *hart, uint32_t inst, CACHE_DecodedOperands* opers) {
    uint64_t rd_l = opers->s ? opers->rd : rd(inst);
    uint64_t rs1_l = opers->s ? opers->rs1 : rs1(inst);
    uint64_t imm = opers->s ? opers->imm : imm_Zicsr(inst);

    uint64_t mstatus = hart->csrs[MSTATUS];
    bool tvm = (mstatus >> 20) & 1; // TVM bit
    if(hart->mode == 1 && tvm && imm == SATP) {
        if(hart->dbg) std::cout << "TVM ENABLED" << std::endl;
        hart->cpu_trap(EXC_ILLEGAL_INSTRUCTION,inst,false);
    }
    if(!csr_accessible(imm,hart->mode,(rs1_l != 0))) {
        if(hart->dbg) std::cout << "MODE: " << (uint32_t)hart->mode << std::endl;
        hart->cpu_trap(EXC_ILLEGAL_INSTRUCTION,inst,false);
    }

    uint64_t init_val = hart->csr_read(imm);
    if (rs1_l != 0) {
        uint64_t mask = rs1_l;
        hart->csr_write(imm,init_val | mask);
    }
    hart->regs[rd_l] = init_val;

    if(!opers->s){
        opers->rd = rd_l;
        opers->rs1 = rs1_l;
        opers->imm = imm;
        opers->s = true;
    }
	if(hart->dbg) hart->print_d("{0x%.8X} [CSRRSI] rs1: %d; rd: %d; imm: int %d uint %u",hart->pc,rs1_l,rd_l,(int64_t) imm, imm);
}
void exec_CSRRCI(struct HART *hart, uint32_t inst, CACHE_DecodedOperands* opers) {
    uint64_t rd_l = opers->s ? opers->rd : rd(inst);
    uint64_t rs1_l = opers->s ? opers->rs1 : rs1(inst);
    uint64_t imm = opers->s ? opers->imm : imm_Zicsr(inst);

    uint64_t mstatus = hart->csrs[MSTATUS];
    bool tvm = (mstatus >> 20) & 1; // TVM bit
    if(hart->mode == 1 && tvm && imm == SATP) {
        if(hart->dbg) std::cout << "TVM ENABLED" << std::endl;
        hart->cpu_trap(EXC_ILLEGAL_INSTRUCTION,inst,false);
    }
    if(!csr_accessible(imm,hart->mode,(rs1_l != 0))) {
        if(hart->dbg) std::cout << "MODE: " << (uint32_t)hart->mode << std::endl;
        hart->cpu_trap(EXC_ILLEGAL_INSTRUCTION,inst,false);
    }

    uint64_t init_val = hart->csr_read(imm);
    if (rs1_l != 0) {
        uint64_t mask = rs1_l;
        hart->csr_write(imm,init_val & ~mask);
    }
    hart->regs[rd_l] = init_val;

    if(!opers->s){
        opers->rd = rd_l;
        opers->rs1 = rs1_l;
        opers->imm = imm;
        opers->s = true;
    }
	if(hart->dbg) hart->print_d("{0x%.8X} [CSRRCI] rs1: %d; rd: %d; imm: int %d uint %u",hart->pc,rs1_l,rd_l,(int64_t) imm, imm);
}