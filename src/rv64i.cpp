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
#include <iostream>

// R-Type

void exec_ADDW(struct HART *hart, uint32_t inst, CACHE_DecodedOperands* opers) {
    uint64_t rd_l = opers->s ? opers->rd : rd(inst);
    uint64_t rs1_l = opers->s ? opers->rs1 : rs1(inst);
    uint64_t rs2_l = opers->s ? opers->rs2 : rs2(inst);

	hart->regs[rd_l] = (int32_t) ((int32_t) hart->regs[rs1_l] + (int32_t) hart->regs[rs2_l]);
	
    if(!opers->s){
        opers->rd = rd_l;
        opers->rs1 = rs1_l;
        opers->rs2 = rs2_l;
        opers->s = true;
    }
	if(hart->dbg) hart->print_d("{0x%.8X} [ADDW] rs1: %d; rs2: %d; rd: %d",hart->pc,rs1_l,rs2_l,rd_l);
}
void exec_SUBW(struct HART *hart, uint32_t inst, CACHE_DecodedOperands* opers) {
    uint64_t rd_l = opers->s ? opers->rd : rd(inst);
    uint64_t rs1_l = opers->s ? opers->rs1 : rs1(inst);
    uint64_t rs2_l = opers->s ? opers->rs2 : rs2(inst);

	hart->regs[rd_l] = (int32_t) ((int32_t) hart->regs[rs1_l] - (int32_t) hart->regs[rs2_l]);
	
    if(!opers->s){
        opers->rd = rd_l;
        opers->rs1 = rs1_l;
        opers->rs2 = rs2_l;
        opers->s = true;
    }
    
	if(hart->dbg) hart->print_d("{0x%.8X} [SUBW] rs1: %d; rs2: %d; rd: %d",hart->pc,rs1_l,rs2_l,rd_l);
}
void exec_SLLW(struct HART *hart, uint32_t inst, CACHE_DecodedOperands* opers) {
    uint64_t rd_l = opers->s ? opers->rd : rd(inst);
    uint64_t rs1_l = opers->s ? opers->rs1 : rs1(inst);
    uint64_t rs2_l = opers->s ? opers->rs2 : rs2(inst);

	hart->regs[rd_l] = (int32_t) ((int32_t)hart->regs[rs1_l] << (int32_t)(hart->regs[rs2_l] & 0x1F));
	
    if(!opers->s){
        opers->rd = rd_l;
        opers->rs1 = rs1_l;
        opers->rs2 = rs2_l;
        opers->s = true;
    }
    
	if(hart->dbg) hart->print_d("{0x%.8X} [SLLW] rs1: %d; rs2: %d; rd: %d",hart->pc,rs1_l,rs2_l,rd_l);
}
void exec_SRLW(struct HART *hart, uint32_t inst, CACHE_DecodedOperands* opers) {
    uint64_t rd_l = opers->s ? opers->rd : rd(inst);
    uint64_t rs1_l = opers->s ? opers->rs1 : rs1(inst);
    uint64_t rs2_l = opers->s ? opers->rs2 : rs2(inst);

	hart->regs[rd_l] = (int32_t) ((uint32_t)hart->regs[rs1_l] >> (int32_t)(hart->regs[rs2_l] & 0x1F));
	
    if(!opers->s){
        opers->rd = rd_l;
        opers->rs1 = rs1_l;
        opers->rs2 = rs2_l;
        opers->s = true;
    }
    
	if(hart->dbg) hart->print_d("{0x%.8X} [SRLW] rs1: %d; rs2: %d; rd: %d",hart->pc,rs1_l,rs2_l,rd_l);
}
void exec_SRAW(struct HART *hart, uint32_t inst, CACHE_DecodedOperands* opers) {
    uint64_t rd_l = opers->s ? opers->rd : rd(inst);
    uint64_t rs1_l = opers->s ? opers->rs1 : rs1(inst);
    uint64_t rs2_l = opers->s ? opers->rs2 : rs2(inst);

	hart->regs[rd_l] = (int64_t) ((int32_t) hart->regs[rs1_l] >> (int64_t) hart->regs[rs2_l]);
	
    if(!opers->s){
        opers->rd = rd_l;
        opers->rs1 = rs1_l;
        opers->rs2 = rs2_l;
        opers->s = true;
    }
    
	if(hart->dbg) hart->print_d("{0x%.8X} [SRAW] rs1: %d; rs2: %d; rd: %d",hart->pc,rs1_l,rs2_l,rd_l);
}

// I-Type
void exec_ADDIW(struct HART *hart, uint32_t inst, CACHE_DecodedOperands* opers) {
    uint64_t rd_l = opers->s ? opers->rd : rd(inst);
    uint64_t rs1_l = opers->s ? opers->rs1 : rs1(inst);
    uint64_t imm = opers->s ? opers->imm : imm_I(inst);

    hart->regs[rd_l] = (int32_t) ((int32_t)hart->regs[rs1_l] + (int32_t) imm);
	
    if(!opers->s){
        opers->rd = rd_l;
        opers->rs1 = rs1_l;
        opers->imm = imm;
        opers->s = true;
    }
    
	if(hart->dbg) hart->print_d("{0x%.8X} [ADDIW] rs1: %d; rd: %d; imm: int %d uint %u",hart->pc,rs1_l,rd_l,(int64_t) imm, imm);
}
void exec_SLLIW(struct HART *hart, uint32_t inst, CACHE_DecodedOperands* opers) {
    uint64_t rd_l = opers->s ? opers->rd : rd(inst);
    uint64_t rs1_l = opers->s ? opers->rs1 : rs1(inst);
    uint64_t imm = opers->s ? opers->imm : shamt(inst);

    hart->regs[rd_l] = (int32_t) ((int32_t)hart->regs[rs1_l] << (uint32_t)imm);
	
    if(!opers->s){
        opers->rd = rd_l;
        opers->rs1 = rs1_l;
        opers->imm = imm;
        opers->s = true;
    }
    
	if(hart->dbg) hart->print_d("{0x%.8X} [SLLIW] rs1: %d; rd: %d; imm: int %d uint %u",hart->pc,rs1_l,rd_l,(int64_t) imm,imm);
}
void exec_SRLIW(struct HART *hart, uint32_t inst, CACHE_DecodedOperands* opers) {
    uint64_t rd_l = opers->s ? opers->rd : rd(inst);
    uint64_t rs1_l = opers->s ? opers->rs1 : rs1(inst);
    uint64_t imm = opers->s ? opers->imm : shamt(inst);

    hart->regs[rd_l] = (int32_t) ((uint32_t)hart->regs[rs1_l] >> (uint32_t)shamt(inst));
	
    if(!opers->s){
        opers->rd = rd_l;
        opers->rs1 = rs1_l;
        opers->imm = imm;
        opers->s = true;
    }
    
	if(hart->dbg) hart->print_d("{0x%.8X} [SRLIW] rs1: %d; rd: %d; imm: int %d uint %u",hart->pc,rs1_l,rd_l,(int64_t) imm,imm);
}
void exec_SRAIW(struct HART *hart, uint32_t inst, CACHE_DecodedOperands* opers) {
    uint64_t rd_l = opers->s ? opers->rd : rd(inst);
    uint64_t rs1_l = opers->s ? opers->rs1 : rs1(inst);
    uint64_t imm = opers->s ? opers->imm : shamt(inst);

    hart->regs[rd_l] = (int64_t) ((int32_t)hart->regs[rs1_l] >> (uint32_t)shamt(inst));
	
    if(!opers->s){
        opers->rd = rd_l;
        opers->rs1 = rs1_l;
        opers->imm = imm;
        opers->s = true;
    }
    
	if(hart->dbg) hart->print_d("{0x%.8X} [SRAIW] rs1: %d; rd: %d; imm: int %d uint %u",hart->pc,rs1_l,rd_l,(int64_t) imm,imm);
}

void exec_LD(struct HART *hart, uint32_t inst, CACHE_DecodedOperands* opers) {
    uint64_t rd_l = opers->s ? opers->rd : rd(inst);
    uint64_t rs1_l = opers->s ? opers->rs1 : rs1(inst);
    uint64_t imm = opers->s ? opers->imm : imm_I(inst);

    {
        uint64_t addr = hart->regs[rs1_l] + (int64_t) imm;
        std::optional<uint64_t> val = hart->mmio->load(hart,addr,64);
        if(val.has_value()) {
            hart->regs[rd_l] = (int64_t) *val;
        }
    }

    if(!opers->s){
        opers->rd = rd_l;
        opers->rs1 = rs1_l;
        opers->imm = imm;
        opers->s = true;
    }
    
	if(hart->dbg) hart->print_d("{0x%.8X} [LD] rs1: %d; rd: %d; imm: int %d uint %u",hart->pc,rs1_l,rd_l,(int64_t) imm,imm);
}
void exec_LWU(struct HART *hart, uint32_t inst, CACHE_DecodedOperands* opers) {
    uint64_t rd_l = opers->s ? opers->rd : rd(inst);
    uint64_t rs1_l = opers->s ? opers->rs1 : rs1(inst);
    uint64_t imm = opers->s ? opers->imm : imm_I(inst);

    {
        uint64_t addr = hart->regs[rs1_l] + (int64_t) imm;
        std::optional<uint64_t> val = hart->mmio->load(hart,addr,32);
        if(val.has_value()) {
            hart->regs[rd_l] = (uint32_t) *val;
        }
    }

    if(!opers->s){
        opers->rd = rd_l;
        opers->rs1 = rs1_l;
        opers->imm = imm;
        opers->s = true;
    }
    
	if(hart->dbg) hart->print_d("{0x%.8X} [LWU] rs1: %d; rd: %d; imm: int %d uint %u",hart->pc,rs1_l,rd_l,(int64_t) imm,imm);
}

// S-Type
void exec_SD(struct HART *hart, uint32_t inst, CACHE_DecodedOperands* opers) {
    uint64_t rs1_l = opers->s ? opers->rs1 : rs1(inst);
    uint64_t rs2_l = opers->s ? opers->rs2 : rs2(inst);
    uint64_t imm = opers->s ? opers->imm : imm_S(inst);
    
    {
	    uint64_t addr = hart->regs[rs1_l] + (int64_t) imm;
	    hart->mmio->store(hart,addr,64,hart->regs[rs2_l]);
    }

    if(!opers->s){
        opers->rs1 = rs1_l;
        opers->rs2 = rs2_l;
        opers->imm = imm;
        opers->s = true;
    }
    
	if(hart->dbg) hart->print_d("{0x%.8X} [SD] rs1: %d; rs2: %d; imm: int %d uint %u",hart->pc,rs1_l,rs2_l,(int64_t) imm,imm);
}

/// RV32I

// R-Type

void exec_ADD(struct HART *hart, uint32_t inst, CACHE_DecodedOperands* opers) {
    uint64_t rd_l = opers->s ? opers->rd : rd(inst);
    uint64_t rs1_l = opers->s ? opers->rs1 : rs1(inst);
    uint64_t rs2_l = opers->s ? opers->rs2 : rs2(inst);

	hart->regs[rd_l] = hart->regs[rs1_l] + hart->regs[rs2_l];

    if(!opers->s){
        opers->rd = rd_l;
        opers->rs1 = rs1_l;
        opers->rs2 = rs2_l;
        opers->s = true;
    }
    
	if(hart->dbg) hart->print_d("{0x%.8X} [ADD] rs1: %d; rs2: %d; rd: %d",hart->pc,rs1_l,rs2_l,rd_l);
}
void exec_SUB(struct HART *hart, uint32_t inst, CACHE_DecodedOperands* opers) {
    uint64_t rd_l = opers->s ? opers->rd : rd(inst);
    uint64_t rs1_l = opers->s ? opers->rs1 : rs1(inst);
    uint64_t rs2_l = opers->s ? opers->rs2 : rs2(inst);

	hart->regs[rd_l] = hart->regs[rs1_l] - hart->regs[rs2_l];
	
    if(!opers->s){
        opers->rd = rd_l;
        opers->rs1 = rs1_l;
        opers->rs2 = rs2_l;
        opers->s = true;
    }
    
	if(hart->dbg) hart->print_d("{0x%.8X} [SUB] rs1: %d; rs2: %d; rd: %d",hart->pc,rs1_l,rs2_l,rd_l);
}
void exec_XOR(struct HART *hart, uint32_t inst, CACHE_DecodedOperands* opers) {
    uint64_t rd_l = opers->s ? opers->rd : rd(inst);
    uint64_t rs1_l = opers->s ? opers->rs1 : rs1(inst);
    uint64_t rs2_l = opers->s ? opers->rs2 : rs2(inst);

	hart->regs[rd_l] = hart->regs[rs1_l] ^ hart->regs[rs2_l];
	
    if(!opers->s){
        opers->rd = rd_l;
        opers->rs1 = rs1_l;
        opers->rs2 = rs2_l;
        opers->s = true;
    }
    
	if(hart->dbg) hart->print_d("{0x%.8X} [XOR] rs1: %d; rs2: %d; rd: %d",hart->pc,rs1_l,rs2_l,rd_l);
}
void exec_OR(struct HART *hart, uint32_t inst, CACHE_DecodedOperands* opers) {
    uint64_t rd_l = opers->s ? opers->rd : rd(inst);
    uint64_t rs1_l = opers->s ? opers->rs1 : rs1(inst);
    uint64_t rs2_l = opers->s ? opers->rs2 : rs2(inst);

	hart->regs[rd_l] = hart->regs[rs1_l] | hart->regs[rs2_l];
	
    if(!opers->s){
        opers->rd = rd_l;
        opers->rs1 = rs1_l;
        opers->rs2 = rs2_l;
        opers->s = true;
    }
    
	if(hart->dbg) hart->print_d("{0x%.8X} [OR] rs1: %d; rs2: %d; rd: %d",hart->pc,rs1_l,rs2_l,rd_l);
}
void exec_AND(struct HART *hart, uint32_t inst, CACHE_DecodedOperands* opers) {
    uint64_t rd_l = opers->s ? opers->rd : rd(inst);
    uint64_t rs1_l = opers->s ? opers->rs1 : rs1(inst);
    uint64_t rs2_l = opers->s ? opers->rs2 : rs2(inst);

	hart->regs[rd_l] = hart->regs[rs1_l] & hart->regs[rs2_l];
	
    if(!opers->s){
        opers->rd = rd_l;
        opers->rs1 = rs1_l;
        opers->rs2 = rs2_l;
        opers->s = true;
    }
    
	if(hart->dbg) hart->print_d("{0x%.8X} [AND] rs1: %d; rs2: %d; rd: %d",hart->pc,rs1_l,rs2_l,rd_l);
}
void exec_SLL(struct HART *hart, uint32_t inst, CACHE_DecodedOperands* opers) {
    uint64_t rd_l = opers->s ? opers->rd : rd(inst);
    uint64_t rs1_l = opers->s ? opers->rs1 : rs1(inst);
    uint64_t rs2_l = opers->s ? opers->rs2 : rs2(inst);

	hart->regs[rd_l] = hart->regs[rs1_l] << (int64_t) hart->regs[rs2_l];
	
    if(!opers->s){
        opers->rd = rd_l;
        opers->rs1 = rs1_l;
        opers->rs2 = rs2_l;
        opers->s = true;
    }
    
	if(hart->dbg) hart->print_d("{0x%.8X} [SLL] rs1: %d; rs2: %d; rd: %d",hart->pc,rs1_l,rs2_l,rd_l);
}
void exec_SRL(struct HART *hart, uint32_t inst, CACHE_DecodedOperands* opers) {
    uint64_t rd_l = opers->s ? opers->rd : rd(inst);
    uint64_t rs1_l = opers->s ? opers->rs1 : rs1(inst);
    uint64_t rs2_l = opers->s ? opers->rs2 : rs2(inst);

	hart->regs[rd_l] = hart->regs[rs1_l] >> hart->regs[rs2_l];
	
    if(!opers->s){
        opers->rd = rd_l;
        opers->rs1 = rs1_l;
        opers->rs2 = rs2_l;
        opers->s = true;
    }
    
	if(hart->dbg) hart->print_d("{0x%.8X} [SRL] rs1: %d; rs2: %d; rd: %d",hart->pc,rs1_l,rs2_l,rd_l);
}
void exec_SRA(struct HART *hart, uint32_t inst, CACHE_DecodedOperands* opers) {
    uint64_t rd_l = opers->s ? opers->rd : rd(inst);
    uint64_t rs1_l = opers->s ? opers->rs1 : rs1(inst);
    uint64_t rs2_l = opers->s ? opers->rs2 : rs2(inst);

	hart->regs[rd_l] = (int32_t) hart->regs[rs1_l] >> (int64_t) hart->regs[rs2_l];
	
    if(!opers->s){
        opers->rd = rd_l;
        opers->rs1 = rs1_l;
        opers->rs2 = rs2_l;
        opers->s = true;
    }
    
	if(hart->dbg) hart->print_d("{0x%.8X} [SRA] rs1: %d; rs2: %d; rd: %d",hart->pc,rs1_l,rs2_l,rd_l);
}
void exec_SLT(struct HART *hart, uint32_t inst, CACHE_DecodedOperands* opers) {
    uint64_t rd_l = opers->s ? opers->rd : rd(inst);
    uint64_t rs1_l = opers->s ? opers->rs1 : rs1(inst);
    uint64_t rs2_l = opers->s ? opers->rs2 : rs2(inst);

	hart->regs[rd_l] = ((int64_t) hart->regs[rs1_l] < (int64_t) hart->regs[rs2_l]) ? 1 : 0;
	
    if(!opers->s){
        opers->rd = rd_l;
        opers->rs1 = rs1_l;
        opers->rs2 = rs2_l;
        opers->s = true;
    }
    
	if(hart->dbg) hart->print_d("{0x%.8X} [SLT] rs1: %d; rs2: %d; rd: %d",hart->pc,rs1_l,rs2_l,rd_l);
}
void exec_SLTU(struct HART *hart, uint32_t inst, CACHE_DecodedOperands* opers) {
    uint64_t rd_l = opers->s ? opers->rd : rd(inst);
    uint64_t rs1_l = opers->s ? opers->rs1 : rs1(inst);
    uint64_t rs2_l = opers->s ? opers->rs2 : rs2(inst);

	hart->regs[rd_l] = (hart->regs[rs1_l] < hart->regs[rs2_l]) ? 1 : 0;
	
    if(!opers->s){
        opers->rd = rd_l;
        opers->rs1 = rs1_l;
        opers->rs2 = rs2_l;
        opers->s = true;
    }
    
	if(hart->dbg) hart->print_d("{0x%.8X} [SLTU] rs1: %d; rs2: %d; rd: %d",hart->pc,rs1_l,rs2_l,rd_l);
}

// I-Type
void exec_ADDI(struct HART *hart, uint32_t inst, CACHE_DecodedOperands* opers) {
    uint64_t rd_l = opers->s ? opers->rd : rd(inst);
    uint64_t rs1_l = opers->s ? opers->rs1 : rs1(inst);
    uint64_t imm = opers->s ? opers->imm : imm_I(inst);

    hart->regs[rd_l] = hart->regs[rs1_l] + (int64_t) imm;
	
    if(!opers->s){
        opers->rd = rd_l;
        opers->rs1 = rs1_l;
        opers->imm = imm;
        opers->s = true;
    }
    
	if(hart->dbg) hart->print_d("{0x%.8X} [ADDI] rs1: %d; rd: %d; imm: int %d uint %u",hart->pc,rs1_l,rd_l,(int64_t) imm, imm);
}
void exec_XORI(struct HART *hart, uint32_t inst, CACHE_DecodedOperands* opers) {
    uint64_t rd_l = opers->s ? opers->rd : rd(inst);
    uint64_t rs1_l = opers->s ? opers->rs1 : rs1(inst);
    uint64_t imm = opers->s ? opers->imm : imm_I(inst);

    hart->regs[rd_l] = hart->regs[rs1_l] ^ imm;
	
    if(!opers->s){
        opers->rd = rd_l;
        opers->rs1 = rs1_l;
        opers->imm = imm;
        opers->s = true;
    }
    
	if(hart->dbg) hart->print_d("{0x%.8X} [XORI] rs1: %d; rd: %d; imm: int %d uint %u",hart->pc,rs1_l,rd_l,(int64_t) imm, imm);
}
void exec_ORI(struct HART *hart, uint32_t inst, CACHE_DecodedOperands* opers) {
    uint64_t rd_l = opers->s ? opers->rd : rd(inst);
    uint64_t rs1_l = opers->s ? opers->rs1 : rs1(inst);
    uint64_t imm = opers->s ? opers->imm : imm_I(inst);

    hart->regs[rd_l] = hart->regs[rs1_l] | imm;
	
    if(!opers->s){
        opers->rd = rd_l;
        opers->rs1 = rs1_l;
        opers->imm = imm;
        opers->s = true;
    }
    
	if(hart->dbg) hart->print_d("{0x%.8X} [ORI] rs1: %d; rd: %d; imm: int %d uint %u",hart->pc,rs1_l,rd_l,(int64_t) imm, imm);
}
void exec_ANDI(struct HART *hart, uint32_t inst, CACHE_DecodedOperands* opers) {
    uint64_t rd_l = opers->s ? opers->rd : rd(inst);
    uint64_t rs1_l = opers->s ? opers->rs1 : rs1(inst);
    uint64_t imm = opers->s ? opers->imm : imm_I(inst);

    hart->regs[rd_l] = hart->regs[rs1_l] & imm;
	
    if(!opers->s){
        opers->rd = rd_l;
        opers->rs1 = rs1_l;
        opers->imm = imm;
        opers->s = true;
    }
    
	if(hart->dbg) hart->print_d("{0x%.8X} [ANDI] rs1: %d; rd: %d; imm: int %d uint %u",hart->pc,rs1_l,rd_l,(int64_t) imm, imm);
}
void exec_SLLI(struct HART *hart, uint32_t inst, CACHE_DecodedOperands* opers) {
    uint64_t rd_l = opers->s ? opers->rd : rd(inst);
    uint64_t rs1_l = opers->s ? opers->rs1 : rs1(inst);
    uint64_t imm = opers->s ? opers->imm : shamt64(inst);

    hart->regs[rd_l] = hart->regs[rs1_l] << imm;
	
    if(!opers->s){
        opers->rd = rd_l;
        opers->rs1 = rs1_l;
        opers->imm = imm;
        opers->s = true;
    }
    
	if(hart->dbg) hart->print_d("{0x%.8X} [SLLI] rs1: %d; rd: %d; imm: int %d uint %u",hart->pc,rs1_l,rd_l,(int64_t) imm, imm);
}
void exec_SRLI(struct HART *hart, uint32_t inst, CACHE_DecodedOperands* opers) {
    uint64_t rd_l = opers->s ? opers->rd : rd(inst);
    uint64_t rs1_l = opers->s ? opers->rs1 : rs1(inst);
    uint64_t imm = opers->s ? opers->imm : shamt64(inst);

    hart->regs[rd_l] = hart->regs[rs1_l] >> imm;
	
    if(!opers->s){
        opers->rd = rd_l;
        opers->rs1 = rs1_l;
        opers->imm = imm;
        opers->s = true;
    }
    
	if(hart->dbg) hart->print_d("{0x%.8X} [SRLI] rs1: %d; rd: %d; imm: int %d uint %u",hart->pc,rs1_l,rd_l,(int64_t) imm, imm);
}
void exec_SRAI(struct HART *hart, uint32_t inst, CACHE_DecodedOperands* opers) {
    uint64_t rd_l = opers->s ? opers->rd : rd(inst);
    uint64_t rs1_l = opers->s ? opers->rs1 : rs1(inst);
    uint64_t imm = opers->s ? opers->imm : shamt64(inst);

    hart->regs[rd_l] = (int64_t)hart->regs[rs1_l] >> imm;
	
    if(!opers->s){
        opers->rd = rd_l;
        opers->rs1 = rs1_l;
        opers->imm = imm;
        opers->s = true;
    }
    
	if(hart->dbg) hart->print_d("{0x%.8X} [SRAI] rs1: %d; rd: %d; imm: int %d uint %u",hart->pc,rs1_l,rd_l,(int64_t) imm, imm);
}
void exec_SLTI(struct HART *hart, uint32_t inst, CACHE_DecodedOperands* opers) {
    uint64_t rd_l = opers->s ? opers->rd : rd(inst);
    uint64_t rs1_l = opers->s ? opers->rs1 : rs1(inst);
    uint64_t imm = opers->s ? opers->imm : imm_I(inst);

    hart->regs[rd_l] = ((int64_t) hart->regs[rs1_l] < (int64_t) imm) ? 1 : 0;
	
    if(!opers->s){
        opers->rd = rd_l;
        opers->rs1 = rs1_l;
        opers->imm = imm;
        opers->s = true;
    }
    
	if(hart->dbg) hart->print_d("{0x%.8X} [SLTI] rs1: %d; rd: %d; imm: int %d uint %u",hart->pc,rs1_l,rd_l,(int64_t) imm, imm);
}
void exec_SLTIU(struct HART *hart, uint32_t inst, CACHE_DecodedOperands* opers) {
    uint64_t rd_l = opers->s ? opers->rd : rd(inst);
    uint64_t rs1_l = opers->s ? opers->rs1 : rs1(inst);
    uint64_t imm = opers->s ? opers->imm : imm_I(inst);

    hart->regs[rd_l] = (hart->regs[rs1_l] < imm) ? 1 : 0;
	
    if(!opers->s){
        opers->rd = rd_l;
        opers->rs1 = rs1_l;
        opers->imm = imm;
        opers->s = true;
    }
    
	if(hart->dbg) hart->print_d("{0x%.8X} [SLTIU] rs1: %d; rd: %d; imm: int %d uint %u",hart->pc,rs1_l,rd_l,(int64_t) imm, imm);
}
void exec_LB(struct HART *hart, uint32_t inst, CACHE_DecodedOperands* opers) {
    uint64_t rd_l = opers->s ? opers->rd : rd(inst);
    uint64_t rs1_l = opers->s ? opers->rs1 : rs1(inst);
    uint64_t imm = opers->s ? opers->imm : imm_I(inst);
	
    {
        uint64_t addr = hart->regs[rs1_l] + (int64_t) imm;
        std::optional<uint64_t> val = hart->mmio->load(hart,addr,8);
        if(val.has_value()) {
            hart->regs[rd_l] = (int64_t)(int8_t) *val;
        }
    }
	
    if(!opers->s){
        opers->rd = rd_l;
        opers->rs1 = rs1_l;
        opers->imm = imm;
        opers->s = true;
    }
    
	if(hart->dbg) hart->print_d("{0x%.8X} [LB] rs1: %d; rd: %d; imm: int %d uint %u",hart->pc,rs1_l,rd_l,(int64_t) imm, imm);
}
void exec_LH(struct HART *hart, uint32_t inst, CACHE_DecodedOperands* opers) {
    uint64_t rd_l = opers->s ? opers->rd : rd(inst);
    uint64_t rs1_l = opers->s ? opers->rs1 : rs1(inst);
    uint64_t imm = opers->s ? opers->imm : imm_I(inst);

    {
        uint64_t addr = hart->regs[rs1_l] + (int64_t) imm;
        std::optional<uint64_t> val = hart->mmio->load(hart,addr,16);
        if(val.has_value()) {
            hart->regs[rd_l] = (int64_t)(int16_t) *val;
        }
    }
	
    if(!opers->s){
        opers->rd = rd_l;
        opers->rs1 = rs1_l;
        opers->imm = imm;
        opers->s = true;
    }
    
	if(hart->dbg) hart->print_d("{0x%.8X} [LH] rs1: %d; rd: %d; imm: int %d uint %u",hart->pc,rs1_l,rd_l,(int64_t) imm, imm);
}
void exec_LW(struct HART *hart, uint32_t inst, CACHE_DecodedOperands* opers) {
    uint64_t rd_l = opers->s ? opers->rd : rd(inst);
    uint64_t rs1_l = opers->s ? opers->rs1 : rs1(inst);
    uint64_t imm = opers->s ? opers->imm : imm_I(inst);

    {
        uint64_t addr = hart->regs[rs1_l] + (int64_t) imm;
        std::optional<uint64_t> val = hart->mmio->load(hart,addr,32);
        if(val.has_value()) {
            hart->regs[rd_l] = (int64_t)(int32_t) *val;
        }
    }
	
    if(!opers->s){
        opers->rd = rd_l;
        opers->rs1 = rs1_l;
        opers->imm = imm;
        opers->s = true;
    }
    
	if(hart->dbg) hart->print_d("{0x%.8X} [LW] rs1: %d; rd: %d; imm: int %d uint %u",hart->pc,rs1_l,rd_l,(int64_t) imm, imm);
}
void exec_LBU(struct HART *hart, uint32_t inst, CACHE_DecodedOperands* opers) {
    uint64_t rd_l = opers->s ? opers->rd : rd(inst);
    uint64_t rs1_l = opers->s ? opers->rs1 : rs1(inst);
    uint64_t imm = opers->s ? opers->imm : imm_I(inst);

    {
        uint64_t addr = hart->regs[rs1_l] + (int64_t) imm;
        std::optional<uint64_t> val = hart->mmio->load(hart,addr,8);
        if(val.has_value()) {
            hart->regs[rd_l] = (uint8_t) *val;
        }
    }
	
    if(!opers->s){
        opers->rd = rd_l;
        opers->rs1 = rs1_l;
        opers->imm = imm;
        opers->s = true;
    }
    
	if(hart->dbg) hart->print_d("{0x%.8X} [LBU] rs1: %d; rd: %d; imm: int %d uint %u",hart->pc,rs1_l,rd_l,(int64_t) imm, imm);
}
void exec_LHU(struct HART *hart, uint32_t inst, CACHE_DecodedOperands* opers) {
    uint64_t rd_l = opers->s ? opers->rd : rd(inst);
    uint64_t rs1_l = opers->s ? opers->rs1 : rs1(inst);
    uint64_t imm = opers->s ? opers->imm : imm_I(inst);

    {
        uint64_t addr = hart->regs[rs1_l] + (int64_t) imm;
        std::optional<uint64_t> val = hart->mmio->load(hart,addr,16);
        if(val.has_value()) {
            hart->regs[rd_l] = (uint16_t) *val;
        }
    }
	
    if(!opers->s){
        opers->rd = rd_l;
        opers->rs1 = rs1_l;
        opers->imm = imm;
        opers->s = true;
    }
    
	if(hart->dbg) hart->print_d("{0x%.8X} [LHU] rs1: %d; rd: %d; imm: int %d uint %u",hart->pc,rs1_l,rd_l,(int64_t) imm, imm);
}

// S-Type
void exec_SB(struct HART *hart, uint32_t inst, CACHE_DecodedOperands* opers) {
    uint64_t imm = opers->s ? opers->imm : imm_S(inst);
    uint64_t rs1_l = opers->s ? opers->rs1 : rs1(inst);
    uint64_t rs2_l = opers->s ? opers->rs2 : rs2(inst);

    {
	    uint64_t addr = hart->regs[rs1_l] + (int64_t) imm;
	    hart->mmio->store(hart,addr,8,hart->regs[rs2_l]);
    }

    if(!opers->s){
        opers->rs2 = rs2_l;
        opers->rs1 = rs1_l;
        opers->imm = imm;
        opers->s = true;
    }
    
	if(hart->dbg) hart->print_d("{0x%.8X} [SB] rs1: %d; rs2: %d; imm: int %d uint %u",hart->pc,rs1_l,rs2_l,(int64_t) imm, imm);
}
void exec_SH(struct HART *hart, uint32_t inst, CACHE_DecodedOperands* opers) {
    uint64_t imm = opers->s ? opers->imm : imm_S(inst);
    uint64_t rs1_l = opers->s ? opers->rs1 : rs1(inst);
    uint64_t rs2_l = opers->s ? opers->rs2 : rs2(inst);

    {
        uint64_t addr = hart->regs[rs1_l] + (int64_t) imm;
        hart->mmio->store(hart,addr,16,hart->regs[rs2_l]);
    }
    
    if(!opers->s){
        opers->rs2 = rs2_l;
        opers->rs1 = rs1_l;
        opers->imm = imm;
        opers->s = true;
    }
    
	if(hart->dbg) hart->print_d("{0x%.8X} [SH] rs1: %d; rs2: %d; imm: int %d uint %u",hart->pc,rs1_l,rs2_l,(int64_t) imm, imm);
}
void exec_SW(struct HART *hart, uint32_t inst, CACHE_DecodedOperands* opers) {
    uint64_t imm = opers->s ? opers->imm : imm_S(inst);
    uint64_t rs1_l = opers->s ? opers->rs1 : rs1(inst);
    uint64_t rs2_l = opers->s ? opers->rs2 : rs2(inst);

    {
	    uint64_t addr = hart->regs[rs1_l] + (int64_t) imm;
	    hart->mmio->store(hart,addr,32,hart->regs[rs2_l]);
    }

    if(!opers->s){
        opers->rs2 = rs2_l;
        opers->rs1 = rs1_l;
        opers->imm = imm;
        opers->s = true;
    }
    
	if(hart->dbg) hart->print_d("{0x%.8X} [SW] rs1: %d; rs2: %d; imm: int %d uint %u",hart->pc,rs1_l,rs2_l,(int64_t) imm, imm);
}

// B-Type
void exec_BEQ(struct HART *hart, uint32_t inst, CACHE_DecodedOperands* opers) {
    uint64_t imm = opers->s ? opers->imm : imm_B(inst);
    uint64_t rs1_l = opers->s ? opers->rs1 : rs1(inst);
    uint64_t rs2_l = opers->s ? opers->rs2 : rs2(inst);

    {
        if ((int64_t) hart->regs[rs1_l] == (int64_t) hart->regs[rs2_l]) {
            hart->pc = hart->pc + (int64_t) imm;
        } else hart->pc += 4;
        opers->brb = true;    
    }

    if(!opers->s){
        opers->rs2 = rs2_l;
        opers->rs1 = rs1_l;
        opers->imm = imm;
        opers->s = true;
    }
    
	if(hart->dbg) hart->print_d("{0x%.8X} [BEQ] rs1: %d; rs2: %d; imm: int %d uint %u",hart->pc,rs1_l,rs2_l,(int64_t) imm, imm);
}
void exec_BNE(struct HART *hart, uint32_t inst, CACHE_DecodedOperands* opers) {
    uint64_t imm = opers->s ? opers->imm : imm_B(inst);
    uint64_t rs1_l = opers->s ? opers->rs1 : rs1(inst);
    uint64_t rs2_l = opers->s ? opers->rs2 : rs2(inst);

    {
	    if ((int64_t) hart->regs[rs1_l] != (int64_t) hart->regs[rs2_l]) {
            hart->pc = hart->pc + (int64_t) imm;
        } else hart->pc += 4;
        opers->brb = true;    
    }
	
    if(!opers->s){
        opers->rs2 = rs2_l;
        opers->rs1 = rs1_l;
        opers->imm = imm;
        opers->s = true;
    }
    
	if(hart->dbg) hart->print_d("{0x%.8X} [BNE] rs1: %d; rs2: %d; imm: int %d uint %u",hart->pc,rs1_l,rs2_l,(int64_t) imm, imm);
}
void exec_BLT(struct HART *hart, uint32_t inst, CACHE_DecodedOperands* opers) {
    uint64_t imm = opers->s ? opers->imm : imm_B(inst);
    uint64_t rs1_l = opers->s ? opers->rs1 : rs1(inst);
    uint64_t rs2_l = opers->s ? opers->rs2 : rs2(inst);

    {
        if ((int64_t) hart->regs[rs1_l] < (int64_t) hart->regs[rs2_l]) {
            hart->pc = hart->pc + (int64_t) imm;
        } else hart->pc += 4;
        opers->brb = true;    
    }
	
    if(!opers->s){
        opers->rs2 = rs2_l;
        opers->rs1 = rs1_l;
        opers->imm = imm;
        opers->s = true;
    }
    
	if(hart->dbg) hart->print_d("{0x%.8X} [BLT] rs1: %d; rs2: %d; imm: int %d uint %u",hart->pc,rs1_l,rs2_l,(int64_t) imm, imm);
}
void exec_BGE(struct HART *hart, uint32_t inst, CACHE_DecodedOperands* opers) {
    uint64_t imm = opers->s ? opers->imm : imm_B(inst);
    uint64_t rs1_l = opers->s ? opers->rs1 : rs1(inst);
    uint64_t rs2_l = opers->s ? opers->rs2 : rs2(inst);

    {
        if ((int64_t) hart->regs[rs1_l] >= (int64_t) hart->regs[rs2_l]) {
            hart->pc = hart->pc + (int64_t) imm;
        } else hart->pc += 4;
        opers->brb = true;    
    }
	
    if(!opers->s){
        opers->rs2 = rs2_l;
        opers->rs1 = rs1_l;
        opers->imm = imm;
        opers->s = true;
    }
    
	if(hart->dbg) hart->print_d("{0x%.8X} [BGE] rs1: %d; rs2: %d; imm: int %d uint %u",hart->pc,rs1_l,rs2_l,(int64_t) imm, imm);
}
void exec_BLTU(struct HART *hart, uint32_t inst, CACHE_DecodedOperands* opers) {
    uint64_t imm = opers->s ? opers->imm : imm_B(inst);
    uint64_t rs1_l = opers->s ? opers->rs1 : rs1(inst);
    uint64_t rs2_l = opers->s ? opers->rs2 : rs2(inst);

    {
        if (hart->regs[rs1_l] < hart->regs[rs2_l]) {
            hart->pc = hart->pc + (int64_t) imm;
        } else hart->pc += 4;
        opers->brb = true;    
    }
	
    if(!opers->s){
        opers->rs2 = rs2_l;
        opers->rs1 = rs1_l;
        opers->imm = imm;
        opers->s = true;
    }
    
	if(hart->dbg) hart->print_d("{0x%.8X} [BLTU] rs1: %d; rs2: %d; imm: int %d uint %u",hart->pc,rs1_l,rs2_l,(int64_t) imm, imm);
}
void exec_BGEU(struct HART *hart, uint32_t inst, CACHE_DecodedOperands* opers) {
    uint64_t imm = opers->s ? opers->imm : imm_B(inst);
    uint64_t rs1_l = opers->s ? opers->rs1 : rs1(inst);
    uint64_t rs2_l = opers->s ? opers->rs2 : rs2(inst);

    {
        if (hart->regs[rs1_l] >= hart->regs[rs2_l]) {
            hart->pc = hart->pc + (int64_t) imm;
        } else hart->pc += 4;
        opers->brb = true;    
    }
	
    if(!opers->s){
        opers->rs2 = rs2_l;
        opers->rs1 = rs1_l;
        opers->imm = imm;
        opers->s = true;
    }
    
	if(hart->dbg) hart->print_d("{0x%.8X} [BGEU] rs1: %d; rs2: %d; imm: int %d uint %u",hart->pc,rs1_l,rs2_l,(int64_t) imm, imm);
}

// JUMP
void exec_JAL(struct HART *hart, uint32_t inst, CACHE_DecodedOperands* opers) {
    uint64_t rd_l = opers->s ? opers->rd : rd(inst);
    uint64_t imm = opers->s ? opers->imm : imm_J(inst);

    {
        hart->regs[rd_l] = hart->pc+4;
        hart->pc = hart->pc + (int64_t) imm;
        opers->brb = true;
    }
	
    if(!opers->s){
        opers->rd = rd_l;
        opers->imm = imm;
        opers->s = true;
    }
    
	if(hart->dbg) hart->print_d("{0x%.8X} [JAL] rd: %d; imm: int %d uint %u",hart->pc,rd_l,(int64_t) imm, imm);
	/*if(ADDR_MISALIGNED(hart->pc)) {
		fprintf(stderr, "JAL PC Address misalligned");
		exit(0);
	}*/
}
void exec_JALR(struct HART *hart, uint32_t inst, CACHE_DecodedOperands* opers) {
    uint64_t rd_l = opers->s ? opers->rd : rd(inst);
    uint64_t rs1_l = opers->s ? opers->rs1 : rs1(inst);
    uint64_t imm = opers->s ? opers->imm : imm_I(inst);

	uint64_t tmp = hart->pc;
	hart->pc = (hart->regs[rs1_l] + (int64_t) imm);
	hart->regs[rd_l] = tmp+4;
	
    if(!opers->s){
        opers->rd = rd_l;
        opers->rs1 = rs1_l;
        opers->imm = imm;
        opers->s = true;
    }
	if(hart->dbg) hart->print_d("{0x%.8X} [JALR] rs1: %d; rd: %d; imm: int %d uint %u",hart->pc,rs1_l,rd_l,(int64_t) imm, imm);
	/*if(ADDR_MISALIGNED(hart->pc)) {
		fprintf(stderr, "JAL PC Address misalligned");
		exit(0);
	}*/
}

// what

void exec_LUI(struct HART *hart, uint32_t inst, CACHE_DecodedOperands* opers) {
    uint64_t rd_l = opers->s ? opers->rd : rd(inst);
    uint64_t imm = opers->s ? opers->imm : imm_U(inst);

	hart->regs[rd_l] = (int64_t) (int32_t) (imm << 12);
	
    if(!opers->s){
        opers->rd = rd_l;
        opers->imm = imm;
        opers->s = true;
    }
    
	if(hart->dbg) hart->print_d("{0x%.8X} [LUI] rd: %d; imm: int %d uint %u",hart->pc,rd_l,(int64_t) imm,imm);
}
void exec_AUIPC(struct HART *hart, uint32_t inst, CACHE_DecodedOperands* opers) {
    uint64_t rd_l = opers->s ? opers->rd : rd(inst);
    uint64_t imm = opers->s ? opers->imm : imm_U(inst);

	hart->regs[rd_l] = hart->pc + ((int64_t) (int32_t) (imm << 12));
	
    if(!opers->s){
        opers->rd = rd_l;
        opers->imm = imm;
        opers->s = true;
    }
    
	if(hart->dbg) hart->print_d("{0x%.8X} [AUIPC] rd: %d; imm: int %d uint %u",hart->pc,rd_l,(int64_t) imm,imm);
}

void exec_ECALL(struct HART *hart, uint32_t inst, CACHE_DecodedOperands* opers) {
	switch(hart->mode) {
		case 3: hart->cpu_trap(EXC_ENV_CALL_FROM_M,0,false); break;
		case 1: hart->cpu_trap(EXC_ENV_CALL_FROM_S,0,false); break;
		case 0: hart->cpu_trap(EXC_ENV_CALL_FROM_U,0,false); break;
	}
}
void exec_EBREAK(struct HART *hart, uint32_t inst, CACHE_DecodedOperands* opers) {
	hart->cpu_trap(EXC_BREAKPOINT,hart->pc,false);
}


void exec_FENCE(struct HART *hart, uint32_t inst, CACHE_DecodedOperands* opers) {
    if(hart->dbg) hart->print_d("{0x%.8X} [FENCE] there is only 1 hart so its nop rn",hart->pc);
}