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

void exec_C_LDSP(struct HART *hart, uint32_t inst, CACHE_DecodedOperands* opers, std::tuple<llvm::IRBuilder<>*, llvm::Function*, llvm::Value*>* jitd) {
    uint64_t rd = opers->s ? opers->rd : get_bits(inst,11,7);

    uint64_t imm = opers->s ? opers->imm : ((get_bits(inst, 4, 4) << 2) | (get_bits(inst, 5, 5) << 3) | (get_bits(inst, 6, 6) << 4) | (get_bits(inst, 12, 12) << 5) | (get_bits(inst, 2, 2) << 6) | (get_bits(inst, 3, 3) << 7));

	std::optional<uint64_t> val = hart->mmio->load(hart,(hart->regs[2]+imm),64);
	if(val.has_value()) {
		hart->regs[rd] = *val;
	}
    if(!opers->s){
        opers->rd = rd;
        opers->imm = imm;
        opers->s = true;
    }
	if(hart->dbg) hart->print_d("{0x%.8X} [C.LDSP] rd: %d; imm: int %d uint %u",hart->pc,rd,(int64_t) imm,imm);
}
void exec_C_SDSP(struct HART *hart, uint32_t inst, CACHE_DecodedOperands* opers, std::tuple<llvm::IRBuilder<>*, llvm::Function*, llvm::Value*>* jitd) {
    uint64_t rs2 = opers->s ? opers->rs2 : get_bits(inst,7,2);

    uint64_t imm = opers->s ? opers->imm : ((get_bits(inst, 9,9) << 2) | (get_bits(inst, 10,10) << 3) | (get_bits(inst, 11,11) << 4) | (get_bits(inst, 12,12) << 5) | (get_bits(inst, 7,7) << 6) | (get_bits(inst, 8,8) << 7));

	hart->mmio->store(hart,(hart->regs[2]+imm),64,hart->regs[rs2]);
    
    if(!opers->s){
        opers->rs2 = rs2;
        opers->imm = imm;
        opers->s = true;
    }
	if(hart->dbg) hart->print_d("{0x%.8X} [C.SDSP] rs2: %d; imm: int %d uint %u",hart->pc,rs2,(int64_t) imm, imm);
}
void exec_C_LD(struct HART *hart, uint32_t inst, CACHE_DecodedOperands* opers, std::tuple<llvm::IRBuilder<>*, llvm::Function*, llvm::Value*>* jitd) {
    uint64_t rd = opers->s ? opers->rd : 8+get_bits(inst,4,2);
    uint64_t rs1 = opers->s ? opers->rs1 : 8+get_bits(inst,9,7);

    uint64_t imm = opers->s ? opers->imm : (get_bits(inst,6,6) << 2) | (get_bits(inst,10,10) << 3) | (get_bits(inst,11,11) << 4) | (get_bits(inst,12,12) << 5) | (get_bits(inst,5,5) << 6);

	std::optional<uint64_t> val = hart->mmio->load(hart,(hart->regs[rs1]+imm),64);
	if(val.has_value()) {
		hart->regs[rd] = *val;
	}
    
    if(!opers->s){
        opers->rs1 = rs1;
        opers->rd = rd;
        opers->imm = imm;
        opers->s = true;
    }
	if(hart->dbg) hart->print_d("{0x%.8X} [C.LD] rs1 %d; rd: %d; imm: int %d uint %u",hart->pc,rs1,rd,(int64_t) imm, imm);
}
void exec_C_SD(struct HART *hart, uint32_t inst, CACHE_DecodedOperands* opers, std::tuple<llvm::IRBuilder<>*, llvm::Function*, llvm::Value*>* jitd) {
    uint64_t rs2 = opers->s ? opers->rs2 : 8+get_bits(inst,4,2);
    uint64_t rs1 = opers->s ? opers->rs1 : 8+get_bits(inst,9,7);

    uint64_t imm = opers->s ? opers->imm : (get_bits(inst,6,6) << 2) | (get_bits(inst,10,10) << 3) | (get_bits(inst,11,11) << 4) | (get_bits(inst,12,12) << 5) | (get_bits(inst,5,5) << 6);

	hart->mmio->store(hart,(hart->regs[rs1]+imm),64,hart->regs[rs2]);
    
    if(!opers->s){
        opers->rs1 = rs1;
        opers->rs2 = rs2;
        opers->imm = imm;
        opers->s = true;
    }
	if(hart->dbg) hart->print_d("{0x%.8X} [C.SD] rs1 %d; rs2: %d; imm: int %d uint %u",hart->pc,rs1,rs2,(int64_t) imm, imm);
}
void exec_C_ADDIW(struct HART *hart, uint32_t inst, CACHE_DecodedOperands* opers, std::tuple<llvm::IRBuilder<>*, llvm::Function*, llvm::Value*>* jitd) {
    uint64_t rd = opers->s ? opers->rd : get_bits(inst,11,7);

    int64_t imm = opers->s ? (int64_t)opers->imm : (get_bits(inst, 12, 12) << 5) | get_bits(inst, 6, 2);
    imm = sext(imm,6);

	hart->regs[rd] = (uint64_t)(int32_t)((int32_t)hart->regs[rd] + (int32_t) imm);

    if(!opers->s){
        opers->rd = rd;
        opers->imm = imm;
        opers->s = true;
    }
	if(hart->dbg) hart->print_d("{0x%.8X} [C.ADDIW] rd: %d; imm: int %d uint %u",hart->pc,rd,(int64_t) imm, imm);
}
void exec_C_SUBW(struct HART *hart, uint32_t inst, CACHE_DecodedOperands* opers, std::tuple<llvm::IRBuilder<>*, llvm::Function*, llvm::Value*>* jitd) {
    uint64_t rd = opers->s ? opers->rd : 8+get_bits(inst,9,7);
    uint64_t rs2 = opers->s ? opers->rs2 : 8+get_bits(inst,4,2);

	hart->regs[rd] = (int32_t)hart->regs[rd] - (int32_t) hart->regs[rs2];
    
	if(hart->dbg) hart->print_d("{0x%.8X} [C.SUBW] rd %d; rs2: %d",hart->pc,rd,rs2);
}
void exec_C_ADDW(struct HART *hart, uint32_t inst, CACHE_DecodedOperands* opers, std::tuple<llvm::IRBuilder<>*, llvm::Function*, llvm::Value*>* jitd) {
    uint64_t rd = opers->s ? opers->rd : 8+get_bits(inst,9,7);
    uint64_t rs2 = opers->s ? opers->rs2 : 8+get_bits(inst,4,2);

    hart->regs[rd] = (int32_t)hart->regs[rd] + (int32_t) hart->regs[rs2];
    
    if(!opers->s){
        opers->rd = rd;
        opers->rs2 = rs2;
        opers->s = true;
    }
	if(hart->dbg) hart->print_d("{0x%.8X} [C.ADDW] rd: %d; rs2: %d",hart->pc,rd,rs2);
}

/// RV32C


void exec_C_LWSP(struct HART *hart, uint32_t inst, CACHE_DecodedOperands* opers, std::tuple<llvm::IRBuilder<>*, llvm::Function*, llvm::Value*>* jitd) {
    uint64_t rd = opers->s ? opers->rd : get_bits(inst,11,7);

    uint64_t imm = opers->s ? opers->imm : ((get_bits(inst, 4, 4) << 2) | (get_bits(inst, 5, 5) << 3) | (get_bits(inst, 6, 6) << 4) | (get_bits(inst, 12, 12) << 5) | (get_bits(inst, 2, 2) << 6) | (get_bits(inst, 3, 3) << 7));

	std::optional<uint64_t> val = hart->mmio->load(hart,(hart->regs[2]+imm),32);
	if(val.has_value()) {
		hart->regs[rd] = (int64_t)(int32_t) *val;
	}
    
    if(!opers->s){
        opers->rd = rd;
        opers->imm = imm;
        opers->s = true;
    }
	if(hart->dbg) hart->print_d("{0x%.8X} [C.LWSP] rd: %d; imm: int %d uint %u",hart->pc,rd,(int64_t) imm,imm);
}
void exec_C_SWSP(struct HART *hart, uint32_t inst, CACHE_DecodedOperands* opers, std::tuple<llvm::IRBuilder<>*, llvm::Function*, llvm::Value*>* jitd) {
    uint64_t rs2 = opers->s ? opers->rs2 : get_bits(inst,7,2);

    uint64_t imm = opers->s ? opers->imm : ((get_bits(inst, 9,9) << 2) | (get_bits(inst, 10,10) << 3) | (get_bits(inst, 11,11) << 4) | (get_bits(inst, 12,12) << 5) | (get_bits(inst, 7,7) << 6) | (get_bits(inst, 8,8) << 7));
    
	hart->mmio->store(hart,(hart->regs[2]+imm),32,hart->regs[rs2]);
    
    if(!opers->s){
        opers->rs2 = rs2;
        opers->imm = imm;
        opers->s = true;
    }
	if(hart->dbg) hart->print_d("{0x%.8X} [C.SWSP] rs2: %d; imm: int %d uint %u",hart->pc,rs2,(int64_t) imm, imm);
}
void exec_C_LW(struct HART *hart, uint32_t inst, CACHE_DecodedOperands* opers, std::tuple<llvm::IRBuilder<>*, llvm::Function*, llvm::Value*>* jitd) {
    uint64_t rd = opers->s ? opers->rd : 8+get_bits(inst,4,2);
    uint64_t rs1 = opers->s ? opers->rs1 : 8+get_bits(inst,9,7);

    uint64_t imm = opers->s ? opers->imm : (get_bits(inst,6,6) << 2) | (get_bits(inst,10,10) << 3) | (get_bits(inst,11,11) << 4) | (get_bits(inst,12,12) << 5) | (get_bits(inst,5,5) << 6);

	std::optional<uint64_t> val = hart->mmio->load(hart,(hart->regs[rs1]+imm),32);
	if(val.has_value()) {
		hart->regs[rd] = (int64_t)(int32_t) *val;
	}
    
    if(!opers->s){
        opers->rd = rd;
        opers->rs1 = rs1;
        opers->imm = imm;
        opers->s = true;
    }
	if(hart->dbg) hart->print_d("{0x%.8X} [C.LW] rs1 %d; rd: %d; imm: int %d uint %u",hart->pc,rs1,rd,(int64_t) imm, imm);
}
void exec_C_SW(struct HART *hart, uint32_t inst, CACHE_DecodedOperands* opers, std::tuple<llvm::IRBuilder<>*, llvm::Function*, llvm::Value*>* jitd) {
    uint64_t rs2 = opers->s ? opers->rs2 : 8+get_bits(inst,4,2);
    uint64_t rs1 = opers->s ? opers->rs1 : 8+get_bits(inst,9,7);

    uint64_t imm = opers->s ? opers->imm : (get_bits(inst,6,6) << 2) | (get_bits(inst,10,10) << 3) | (get_bits(inst,11,11) << 4) | (get_bits(inst,12,12) << 5) | (get_bits(inst,5,5) << 6);

	hart->mmio->store(hart,(hart->regs[rs1]+imm),32,hart->regs[rs2]);
    
    if(!opers->s){
        opers->rs1 = rs1;
        opers->rs2 = rs2;
        opers->imm = imm;
        opers->s = true;
    }
	if(hart->dbg) hart->print_d("{0x%.8X} [C.SW] rs1 %d; rs2: %d; imm: int %d uint %u",hart->pc,rs1,rs2,(int64_t) imm, imm);
}
void exec_C_J(struct HART *hart, uint32_t inst, CACHE_DecodedOperands* opers, std::tuple<llvm::IRBuilder<>*, llvm::Function*, llvm::Value*>* jitd) {
    uint64_t imm = opers->s ? opers->imm : ((get_bits(inst, 12, 12) << 11) |
        (get_bits(inst, 11, 11) << 4)  |
        (get_bits(inst, 10, 9) << 8)   |
        (get_bits(inst, 8, 8) << 10)   |
        (get_bits(inst, 7, 7) << 6)    |
        (get_bits(inst, 6, 6) << 7)    |
        (get_bits(inst, 5, 3) << 1)    |
        (get_bits(inst, 2, 2) << 5));

    hart->pc = hart->pc + imm;
    opers->brb = true;

    if(!opers->s){
        opers->imm = imm;
        opers->s = true;
    }
	if(hart->dbg) hart->print_d("{0x%.8X} [C.J] offset: %d",hart->pc,imm);
}
void exec_C_JAL(struct HART *hart, uint32_t inst, CACHE_DecodedOperands* opers, std::tuple<llvm::IRBuilder<>*, llvm::Function*, llvm::Value*>* jitd) {
    uint64_t imm = opers->s ? opers->imm : ((get_bits(inst, 12, 12) << 11) |
        (get_bits(inst, 11, 11) << 4)  |
        (get_bits(inst, 10, 9) << 8)   |
        (get_bits(inst, 8, 8) << 10)   |
        (get_bits(inst, 7, 7) << 6)    |
        (get_bits(inst, 6, 6) << 7)    |
        (get_bits(inst, 5, 3) << 1)    |
        (get_bits(inst, 2, 2) << 5));

	hart->regs[1] = hart->pc + 2;
    hart->pc = hart->pc + imm;
    opers->brb = true;
    
    if(!opers->s){
        opers->imm = imm;
        opers->s = true;
    }
	if(hart->dbg) hart->print_d("{0x%.8X} [C.JAL] offset: %d",hart->pc,imm);
}
void exec_C_JR(struct HART *hart, uint32_t inst, CACHE_DecodedOperands* opers, std::tuple<llvm::IRBuilder<>*, llvm::Function*, llvm::Value*>* jitd) {
    uint64_t rs1 = opers->s ? opers->rs1 : get_bits(inst,11,7);

    hart->pc = hart->regs[rs1];
    opers->brb = true;
    
    if(!opers->s){
        opers->rs1 = rs1;
        opers->s = true;
    }
	if(hart->dbg) hart->print_d("{0x%.8X} [C.JR] rs1: %d",hart->pc,rs1);
}
void exec_C_JALR(struct HART *hart, uint32_t inst, CACHE_DecodedOperands* opers, std::tuple<llvm::IRBuilder<>*, llvm::Function*, llvm::Value*>* jitd) {
    uint64_t rs1 = opers->s ? opers->rs1 : get_bits(inst,11,7);

	hart->regs[1] = hart->pc + 2;
    hart->pc = hart->regs[rs1];
    opers->brb = true;
    
    if(!opers->s){
        opers->rs1 = rs1;
        opers->s = true;
    }
	if(hart->dbg) hart->print_d("{0x%.8X} [C.JALR] rs1: %d",hart->pc,rs1);
}
void exec_C_BEQZ(struct HART *hart, uint32_t inst, CACHE_DecodedOperands* opers, std::tuple<llvm::IRBuilder<>*, llvm::Function*, llvm::Value*>* jitd) {
    uint64_t rs1 = opers->s ? opers->rs1 : 8+get_bits(inst,11,7);
    uint64_t imm = opers->s ? opers->imm : sext((get_bits(inst, 12, 12) << 8) |
        (get_bits(inst, 6, 5) << 6)   |
        (get_bits(inst, 11, 10) << 3) |
        (get_bits(inst, 4, 3) << 1)   |
        (get_bits(inst, 2, 2) << 5),9);

	if ((int64_t) hart->regs[rs1] == 0) {
		hart->pc = hart->pc + (int64_t) imm;
        opers->brb = true;
    }
    
    if(!opers->s){
        opers->rs1 = rs1;
        opers->imm = imm;
        opers->s = true;
    }
	if(hart->dbg) hart->print_d("{0x%.8X} [C.BEQZ] rs1: %d; imm: int %d uint %u",hart->pc,rs1, (int64_t) imm, imm);
}
void exec_C_BNEZ(struct HART *hart, uint32_t inst, CACHE_DecodedOperands* opers, std::tuple<llvm::IRBuilder<>*, llvm::Function*, llvm::Value*>* jitd) {
    uint64_t rs1 = opers->s ? opers->rs1 : 8+get_bits(inst,11,7);
    uint64_t imm = opers->s ? opers->imm : sext((get_bits(inst, 12, 12) << 8) |
        (get_bits(inst, 6, 5) << 6)   |
        (get_bits(inst, 11, 10) << 3) |
        (get_bits(inst, 4, 3) << 1)   |
        (get_bits(inst, 2, 2) << 5),9);

	if ((int64_t) hart->regs[rs1] != 0) {
		hart->pc = hart->pc + (int64_t) imm;
        opers->brb = true;
    }
    
    if(!opers->s){
        opers->rs1 = rs1;
        opers->imm = imm;
        opers->s = true;
    }
	if(hart->dbg) hart->print_d("{0x%.8X} [C.BNEZ] rs1: %d; imm: int %d uint %u",hart->pc,rs1, (int64_t) imm, imm);
}
void exec_C_LI(struct HART *hart, uint32_t inst, CACHE_DecodedOperands* opers, std::tuple<llvm::IRBuilder<>*, llvm::Function*, llvm::Value*>* jitd) {
    uint64_t rd = opers->s ? opers->rd : get_bits(inst,11,7);

    int64_t imm = opers->s ? (int64_t)opers->imm : sext((get_bits(inst, 12, 12) << 5) | get_bits(inst, 6, 2),6);

    std::cout << imm << std::endl;
	hart->regs[rd] = (int64_t) imm;
    
    if(!opers->s){
        opers->rd = rd;
        opers->imm = imm;
        opers->s = true;
    }
	if(hart->dbg) hart->print_d("{0x%.8X} [C.LI] rd: %d; imm: int %d uint %u",hart->pc,rd,imm, (uint64_t) imm);
}
void exec_C_LUI(struct HART *hart, uint32_t inst, CACHE_DecodedOperands* opers, std::tuple<llvm::IRBuilder<>*, llvm::Function*, llvm::Value*>* jitd) {
    uint64_t rd = opers->s ? opers->rd : get_bits(inst,11,7);

    int64_t imm = opers->s ? (int64_t)opers->imm : sext((get_bits(inst, 12, 12) << 5) | get_bits(inst, 6, 2),6);

	hart->regs[rd] = (int64_t) imm << 12;
    
    if(!opers->s){
        opers->rd = rd;
        opers->imm = imm;
        opers->s = true;
    }
	if(hart->dbg) hart->print_d("{0x%.8X} [C.LUI] rd: %d; imm: int %d uint %u",hart->pc,rd,(int64_t) imm, imm);
}
void exec_C_ADDI(struct HART *hart, uint32_t inst, CACHE_DecodedOperands* opers, std::tuple<llvm::IRBuilder<>*, llvm::Function*, llvm::Value*>* jitd) {
    uint64_t rd = opers->s ? opers->rd : get_bits(inst,11,7);
    int64_t imm = opers->s ? (int64_t)opers->imm : sext((get_bits(inst, 12, 12) << 5) | get_bits(inst, 6, 2),6);

	hart->regs[rd] = (uint64_t)hart->regs[rd] + (int64_t) imm;
    
    if(!opers->s){
        opers->rd = rd;
        opers->imm = imm;
        opers->s = true;
    }
	if(hart->dbg) hart->print_d("{0x%.8X} [C.ADDI] rd: %d; imm: int %d uint %u",hart->pc,rd,(int64_t) imm, imm);
}
void exec_C_ADDI16SP(struct HART *hart, uint32_t inst, CACHE_DecodedOperands* opers, std::tuple<llvm::IRBuilder<>*, llvm::Function*, llvm::Value*>* jitd) {
    int64_t imm = opers->s ? (int64_t)opers->imm : sext((
        (get_bits(inst,6,6) << 4) | 
        (get_bits(inst,2,2) << 5) | 
        (get_bits(inst,5,5) << 6) | 
        (get_bits(inst,3,3) << 7) | 
        (get_bits(inst,4,4) << 8) | 
        (get_bits(inst,12,12) << 9)),10);

	hart->regs[2] += imm;
    
    if(!opers->s){
        opers->imm = imm;
        opers->s = true;
    }
	if(hart->dbg) hart->print_d("{0x%.8X} [C.ADDI16SP] imm: int %d uint %u",hart->pc,(int64_t) imm, imm);
}
void exec_C_ADDI4SPN(struct HART *hart, uint32_t inst, CACHE_DecodedOperands* opers, std::tuple<llvm::IRBuilder<>*, llvm::Function*, llvm::Value*>* jitd) {
    uint64_t rd = opers->s ? opers->rd : 8+get_bits(inst,4,2);

    uint64_t imm = opers->s ? opers->imm : ((get_bits(inst,6,6) << 2) | (get_bits(inst,5,5) << 3) | (get_bits(inst,11,11) << 4) | (get_bits(inst,12,12) << 5) | (get_bits(inst,7,7) << 6) | (get_bits(inst,8,8) << 7) | (get_bits(inst,9,9) << 8) | (get_bits(inst,10,10) << 9));

	hart->regs[rd] = hart->regs[2] + imm;
    
    if(!opers->s){
        opers->rd = rd;
        opers->imm = imm;
        opers->s = true;
    }
	if(hart->dbg) hart->print_d("{0x%.8X} [C.ADDI4SPN] rd: %d; imm: uint %d",hart->pc,rd,imm);
}
void exec_C_SLLI(struct HART *hart, uint32_t inst, CACHE_DecodedOperands* opers, std::tuple<llvm::IRBuilder<>*, llvm::Function*, llvm::Value*>* jitd) {
    uint64_t rd = opers->s ? opers->rd : get_bits(inst,11,7);

    uint64_t imm = opers->s ? opers->rd : ((get_bits(inst, 12, 12) << 5) | (( get_bits(inst, 6, 4)) << 2) + (((inst >> 2) & 3) * 64));

	hart->regs[rd] <<= imm;
    
    if(!opers->s){
        opers->rd = rd;
        opers->imm = imm;
        opers->s = true;
    }
	if(hart->dbg) hart->print_d("{0x%.8X} [C.SLLI] rd: %d; imm: int %d uint %u",hart->pc,rd,(int64_t) imm, imm);
}
void exec_C_SRLI(struct HART *hart, uint32_t inst, CACHE_DecodedOperands* opers, std::tuple<llvm::IRBuilder<>*, llvm::Function*, llvm::Value*>* jitd) {
    uint64_t rd = opers->s ? opers->rd : 8+get_bits(inst,9,7);
    uint64_t imm = opers->s ? opers->imm : sext((get_bits(inst, 12, 12) << 5) |
        (get_bits(inst, 6, 2)),7);

	hart->regs[rd] >>= imm;
    
    if(!opers->s){
        opers->rd = rd;
        opers->imm = imm;
        opers->s = true;
    }
	if(hart->dbg) hart->print_d("{0x%.8X} [C.SRLI] rd: %d; imm: int %d uint %u",hart->pc,rd, (int64_t) imm, imm);
}
void exec_C_SRAI(struct HART *hart, uint32_t inst, CACHE_DecodedOperands* opers, std::tuple<llvm::IRBuilder<>*, llvm::Function*, llvm::Value*>* jitd) {
    uint64_t rd = opers->s ? opers->rd : 8+get_bits(inst,9,7);
    uint64_t imm = opers->s ? opers->imm : sext((get_bits(inst, 12, 12) << 5) |
        (get_bits(inst, 6, 2)),7);

	hart->regs[rd] = (uint64_t)((int64_t)hart->regs[rd] >> imm);
    
    if(!opers->s){
        opers->rd = rd;
        opers->imm = imm;
        opers->s = true;
    }
	if(hart->dbg) hart->print_d("{0x%.8X} [C.SRAI] rd: %d; imm: int %d uint %u",hart->pc,rd, (int64_t) imm, imm);
}
void exec_C_ANDI(struct HART *hart, uint32_t inst, CACHE_DecodedOperands* opers, std::tuple<llvm::IRBuilder<>*, llvm::Function*, llvm::Value*>* jitd) {
    uint64_t rd = opers->s ? opers->rd : 8+get_bits(inst,9,7);
    int64_t imm = opers->s ? (int64_t)opers->imm : sext((get_bits(inst, 12, 12) << 5) |
        (get_bits(inst, 6, 2)),6);

	hart->regs[rd] = (uint64_t)hart->regs[rd] & (int64_t)imm;
    
    if(!opers->s){
        opers->rd = rd;
        opers->imm = imm;
        opers->s = true;
    }
	if(hart->dbg) hart->print_d("{0x%.8X} [C.ANDI] rd: %d; imm: int %d uint %u",hart->pc,rd, (int64_t) imm, imm);
}
void exec_C_MV(struct HART *hart, uint32_t inst, CACHE_DecodedOperands* opers, std::tuple<llvm::IRBuilder<>*, llvm::Function*, llvm::Value*>* jitd) {
    uint64_t rd = opers->s ? opers->rd : get_bits(inst,11,7);
    uint64_t rs2 = opers->s ? opers->rs2 : get_bits(inst,6,2);

    hart->regs[rd] = hart->regs[rs2];
    
    if(!opers->s){
        opers->rd = rd;
        opers->rs2 = rs2;
        opers->s = true;
    }
	if(hart->dbg) hart->print_d("{0x%.8X} [C.MV] rd: %d; rs2: %d",hart->pc,rd,rs2);
}
void exec_C_ADD(struct HART *hart, uint32_t inst, CACHE_DecodedOperands* opers, std::tuple<llvm::IRBuilder<>*, llvm::Function*, llvm::Value*>* jitd) {
    uint64_t rd = opers->s ? opers->rd : get_bits(inst,11,7);
    uint64_t rs2 = opers->s ? opers->rs2 : get_bits(inst,6,2);

    hart->regs[rd] += hart->regs[rs2];
    
    if(!opers->s){
        opers->rd = rd;
        opers->rs2 = rs2;
        opers->s = true;
    }
	if(hart->dbg) hart->print_d("{0x%.8X} [C.ADD] rd: %d; rs2: %d",hart->pc,rd,rs2);
}
void exec_C_AND(struct HART *hart, uint32_t inst, CACHE_DecodedOperands* opers, std::tuple<llvm::IRBuilder<>*, llvm::Function*, llvm::Value*>* jitd) {
    uint64_t rd = opers->s ? opers->rd : 8+get_bits(inst,9,7);
    uint64_t rs2 = opers->s ? opers->rs2 : 8+get_bits(inst,4,2);

	hart->regs[rd] &= hart->regs[rs2];
    
    if(!opers->s){
        opers->rd = rd;
        opers->rs2 = rs2;
        opers->s = true;
    }
	if(hart->dbg) hart->print_d("{0x%.8X} [C.AND] rd %d; rs2: %d",hart->pc,rd,rs2);
}
void exec_C_OR(struct HART *hart, uint32_t inst, CACHE_DecodedOperands* opers, std::tuple<llvm::IRBuilder<>*, llvm::Function*, llvm::Value*>* jitd) {
    uint64_t rd = opers->s ? opers->rd : 8+get_bits(inst,9,7);
    uint64_t rs2 = opers->s ? opers->rs2 : 8+get_bits(inst,4,2);

	hart->regs[rd] |= hart->regs[rs2];
    
    if(!opers->s){
        opers->rd = rd;
        opers->rs2 = rs2;
        opers->s = true;
    }
	if(hart->dbg) hart->print_d("{0x%.8X} [C.OR] rd %d; rs2: %d",hart->pc,rd,rs2);
}
void exec_C_XOR(struct HART *hart, uint32_t inst, CACHE_DecodedOperands* opers, std::tuple<llvm::IRBuilder<>*, llvm::Function*, llvm::Value*>* jitd) {
    uint64_t rd = opers->s ? opers->rd : 8+get_bits(inst,9,7);
    uint64_t rs2 = opers->s ? opers->rs2 : 8+get_bits(inst,4,2);

	hart->regs[rd] ^= hart->regs[rs2];
    
    if(!opers->s){
        opers->rd = rd;
        opers->rs2 = rs2;
        opers->s = true;
    }
	if(hart->dbg) hart->print_d("{0x%.8X} [C.XOR] rd %d; rs2: %d",hart->pc,rd,rs2);
}
void exec_C_SUB(struct HART *hart, uint32_t inst, CACHE_DecodedOperands* opers, std::tuple<llvm::IRBuilder<>*, llvm::Function*, llvm::Value*>* jitd) {
    uint64_t rd = opers->s ? opers->rd : 8+get_bits(inst,9,7);
    uint64_t rs2 = opers->s ? opers->rs2 : 8+get_bits(inst,4,2);

	hart->regs[rd] -= hart->regs[rs2];
    
    if(!opers->s){
        opers->rd = rd;
        opers->rs2 = rs2;
        opers->s = true;
    }
	if(hart->dbg) hart->print_d("{0x%.8X} [C.SUB] rd %d; rs2: %d",hart->pc,rd,rs2);
}
void exec_C_NOP(struct HART *hart, uint32_t inst, CACHE_DecodedOperands* opers, std::tuple<llvm::IRBuilder<>*, llvm::Function*, llvm::Value*>* jitd) {
	if(hart->dbg) hart->print_d("{0x%.8X} [C.NOP] why the fuck we actually logging nops?",hart->pc);
}
void exec_C_EBREAK(struct HART *hart, uint32_t inst, CACHE_DecodedOperands* opers, std::tuple<llvm::IRBuilder<>*, llvm::Function*, llvm::Value*>* jitd) {
	hart->cpu_trap(EXC_BREAKPOINT,hart->pc,false);
}