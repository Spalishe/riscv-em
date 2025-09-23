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

	uint64_t addr = hart->regs[rs1_l] + (int64_t) imm;
	std::optional<uint64_t> val = hart->mmio->load(hart,addr,64);
	if(val.has_value()) {
		hart->regs[rd_l] = (int64_t) *val;
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

	uint64_t addr = hart->regs[rs1_l] + (int64_t) imm;
	std::optional<uint64_t> val = hart->mmio->load(hart,addr,32);
	if(val.has_value()) {
		hart->regs[rd_l] = (uint32_t) *val;
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

	uint64_t addr = hart->regs[rs1_l] + (int64_t) imm;
	hart->mmio->store(hart,addr,64,hart->regs[rs2_l]);

    if(!opers->s){
        opers->rs1 = rs1_l;
        opers->rs2 = rs2_l;
        opers->imm = imm;
        opers->s = true;
    }
	if(hart->dbg) hart->print_d("{0x%.8X} [SD] rs1: %d; rs2: %d; imm: int %d uint %u",hart->pc,rs1_l,rs2_l,(int64_t) imm,imm);
}