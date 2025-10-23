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
#include "../include/jit_insts.hpp"
#include <iostream>

// ZiCSR

bool compatible_mode(uint64_t csr_level, uint64_t cur_mode) {
    if(csr_level == 3) {
        return cur_mode == 3;
    } else if(csr_level == 2) {
        return (cur_mode == 3) || (cur_mode == 1);
    } else if(csr_level == 2) {
        return (cur_mode == 3) || (cur_mode == 1) || (cur_mode == 0);
    }
    return true;
}

bool csr_accessible(uint16_t csr_addr, uint64_t current_priv, bool write) {
    uint64_t csr_level;

    if (csr_addr >= 0x000 && csr_addr <= 0x0FF) csr_level = 0;
    else if (csr_addr >= 0x100 && csr_addr <= 0x1FF) csr_level = 1;
    else if ((csr_addr >= 0x300 && csr_addr <= 0x3FF) || (csr_addr >= 0xB00 && csr_addr <= 0xBFF) || (csr_addr >= 0xF00 && csr_addr <= 0xFFF)) csr_level = 3;
    else {
        csr_level = 0;
    }

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
void exec_CSRRW(struct HART *hart, uint32_t inst, CACHE_DecodedOperands* opers, std::tuple<llvm::IRBuilder<>*, llvm::Function*, llvm::Value*>* jitd) {
    uint64_t rd_l = opers->s ? opers->rd : rd(inst);
    uint64_t rs1_l = opers->s ? opers->rs1 : rs1(inst);
    uint64_t imm = opers->s ? opers->imm : imm_Zicsr(inst);

    if(!csr_accessible(imm,hart->mode,true)) {
        if(hart->dbg) std::cout << "MODE: " << hart->mode << std::endl;
        hart->cpu_trap(EXC_ILLEGAL_INSTRUCTION,hart->mode,false);
    }
    if(jitd == NULL) {
        if (rd_l != 0) {
            hart->regs[rd_l] = hart->csrs[imm];
        }
        hart->csrs[imm] = hart->regs[rs1_l];
    }

    if(!opers->s){
        opers->rd = rd_l;
        opers->rs1 = rs1_l;
        opers->imm = imm;
        opers->s = true;
    }
    if(jitd != NULL) jit_ZICSR_R(hart, opers, std::get<0>(*jitd), std::get<1>(*jitd), std::get<2>(*jitd), 0);
	if(hart->dbg) hart->print_d("{0x%.8X} [CSRRW] rs1: %d; rd: %d; imm: int %d uint %u",hart->pc,rs1_l,rd_l,(int64_t) imm, imm);
}
void exec_CSRRS(struct HART *hart, uint32_t inst, CACHE_DecodedOperands* opers, std::tuple<llvm::IRBuilder<>*, llvm::Function*, llvm::Value*>* jitd) {
    uint64_t rd_l = opers->s ? opers->rd : rd(inst);
    uint64_t rs1_l = opers->s ? opers->rs1 : rs1(inst);
    uint64_t imm = opers->s ? opers->imm : imm_Zicsr(inst);

    if(!csr_accessible(imm,hart->mode,(rs1_l != 0))) {
        if(hart->dbg) std::cout << "MODE: " << hart->mode << std::endl;
        hart->cpu_trap(EXC_ILLEGAL_INSTRUCTION,hart->mode,false);
    }
    if(jitd == NULL) {
        uint64_t init_val = hart->csrs[imm];
        if (rs1_l != 0) {
            uint64_t mask = hart->regs[rs1_l];
            hart->csrs[imm] = hart->csrs[imm] | mask;
        }
        hart->regs[rd_l] = init_val;
    }

    if(!opers->s){
        opers->rd = rd_l;
        opers->rs1 = rs1_l;
        opers->imm = imm;
        opers->s = true;
    }
    if(jitd != NULL) jit_ZICSR_R(hart, opers, std::get<0>(*jitd), std::get<1>(*jitd), std::get<2>(*jitd), 1);
	if(hart->dbg) hart->print_d("{0x%.8X} [CSRRS] rs1: %d; rd: %d; imm: int %d uint %u",hart->pc,rs1_l,rd_l,(int64_t) imm, imm);
}
void exec_CSRRC(struct HART *hart, uint32_t inst, CACHE_DecodedOperands* opers, std::tuple<llvm::IRBuilder<>*, llvm::Function*, llvm::Value*>* jitd) {
    uint64_t rd_l = opers->s ? opers->rd : rd(inst);
    uint64_t rs1_l = opers->s ? opers->rs1 : rs1(inst);
    uint64_t imm = opers->s ? opers->imm : imm_Zicsr(inst);

    if(!csr_accessible(imm,hart->mode,(rs1_l != 0))) {
        if(hart->dbg) std::cout << "MODE: " << hart->mode << std::endl;
        hart->cpu_trap(EXC_ILLEGAL_INSTRUCTION,hart->mode,false);
    }
    if(jitd == NULL) {
        uint64_t init_val = hart->csrs[imm];
        if (rs1_l != 0) {
            uint64_t mask = hart->regs[rs1_l];
            hart->csrs[imm] = hart->csrs[imm] & ~mask;
        }
        hart->regs[rd_l] = init_val;
    }

    if(!opers->s){
        opers->rd = rd_l;
        opers->rs1 = rs1_l;
        opers->imm = imm;
        opers->s = true;
    }
    if(jitd != NULL) jit_ZICSR_R(hart, opers, std::get<0>(*jitd), std::get<1>(*jitd), std::get<2>(*jitd), 2);
	if(hart->dbg) hart->print_d("{0x%.8X} [CSRRC] rs1: %d; rd: %d; imm: int %d uint %u",hart->pc,rs1_l,rd_l,(int64_t) imm, imm);
}

void exec_CSRRWI(struct HART *hart, uint32_t inst, CACHE_DecodedOperands* opers, std::tuple<llvm::IRBuilder<>*, llvm::Function*, llvm::Value*>* jitd) {
    uint64_t rd_l = opers->s ? opers->rd : rd(inst);
    uint64_t rs1_l = opers->s ? opers->rs1 : rs1(inst);
    uint64_t imm = opers->s ? opers->imm : imm_Zicsr(inst);

    if(!csr_accessible(imm,hart->mode,true)) {
        if(hart->dbg) std::cout << "MODE: " << hart->mode << std::endl;
        hart->cpu_trap(EXC_ILLEGAL_INSTRUCTION,hart->mode,false);
    }
    if(jitd == NULL) {
        if (rd_l != 0) {
            hart->regs[rd_l] = hart->csrs[imm];
        }
        hart->csrs[imm] = rs1_l;
    }

    if(!opers->s){
        opers->rd = rd_l;
        opers->rs1 = rs1_l;
        opers->imm = imm;
        opers->s = true;
    }
    if(jitd != NULL) jit_ZICSR_I(hart, opers, std::get<0>(*jitd), std::get<1>(*jitd), std::get<2>(*jitd), 0);
	if(hart->dbg) hart->print_d("{0x%.8X} [CSRRWI] rs1: %d; rd: %d; imm: int %d uint %u",hart->pc,rs1_l,rd_l,(int64_t) imm, imm);
}
void exec_CSRRSI(struct HART *hart, uint32_t inst, CACHE_DecodedOperands* opers, std::tuple<llvm::IRBuilder<>*, llvm::Function*, llvm::Value*>* jitd) {
    uint64_t rd_l = opers->s ? opers->rd : rd(inst);
    uint64_t rs1_l = opers->s ? opers->rs1 : rs1(inst);
    uint64_t imm = opers->s ? opers->imm : imm_Zicsr(inst);

    if(!csr_accessible(imm,hart->mode,(rs1_l != 0))) {
        if(hart->dbg) std::cout << "MODE: " << hart->mode << std::endl;
        hart->cpu_trap(EXC_ILLEGAL_INSTRUCTION,hart->mode,false);
    }
    if(jitd == NULL) {
        uint64_t init_val = hart->csrs[imm];
        if (rs1_l != 0) {
            uint64_t mask = rs1_l;
            hart->csrs[imm] = hart->csrs[imm] | mask;
        }
        hart->regs[rd_l] = init_val;
    }

    if(!opers->s){
        opers->rd = rd_l;
        opers->rs1 = rs1_l;
        opers->imm = imm;
        opers->s = true;
    }
    if(jitd != NULL) jit_ZICSR_I(hart, opers, std::get<0>(*jitd), std::get<1>(*jitd), std::get<2>(*jitd), 1);
	if(hart->dbg) hart->print_d("{0x%.8X} [CSRRSI] rs1: %d; rd: %d; imm: int %d uint %u",hart->pc,rs1_l,rd_l,(int64_t) imm, imm);
}
void exec_CSRRCI(struct HART *hart, uint32_t inst, CACHE_DecodedOperands* opers, std::tuple<llvm::IRBuilder<>*, llvm::Function*, llvm::Value*>* jitd) {
    uint64_t rd_l = opers->s ? opers->rd : rd(inst);
    uint64_t rs1_l = opers->s ? opers->rs1 : rs1(inst);
    uint64_t imm = opers->s ? opers->imm : imm_Zicsr(inst);

    if(!csr_accessible(imm,hart->mode,(rs1_l != 0))) {
        if(hart->dbg) std::cout << "MODE: " << hart->mode << std::endl;
        hart->cpu_trap(EXC_ILLEGAL_INSTRUCTION,hart->mode,false);
    }
    if(jitd == NULL) {
        uint64_t init_val = hart->csrs[imm];
        if (rs1_l != 0) {
            uint64_t mask = rs1_l;
            hart->csrs[imm] = hart->csrs[imm] & ~mask;
        }
        hart->regs[rd_l] = init_val;
    }

    if(!opers->s){
        opers->rd = rd_l;
        opers->rs1 = rs1_l;
        opers->imm = imm;
        opers->s = true;
    }
    if(jitd != NULL) jit_ZICSR_I(hart, opers, std::get<0>(*jitd), std::get<1>(*jitd), std::get<2>(*jitd), 2);
	if(hart->dbg) hart->print_d("{0x%.8X} [CSRRCI] rs1: %d; rd: %d; imm: int %d uint %u",hart->pc,rs1_l,rd_l,(int64_t) imm, imm);
}