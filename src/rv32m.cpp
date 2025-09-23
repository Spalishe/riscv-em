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

#include <limits>

// R-Type

void exec_MUL(struct HART *hart, uint32_t inst, CACHE_DecodedOperands* opers) {
    uint64_t rd_l = opers->s ? opers->rd : rd(inst);
    uint64_t rs1_l = opers->s ? opers->rs1 : rs1(inst);
    uint64_t rs2_l = opers->s ? opers->rs2 : rs2(inst);

	hart->regs[rd_l] = hart->regs[rs1_l] * hart->regs[rs2_l];
	
    if(!opers->s){
        opers->rd = rd_l;
        opers->rs1 = rs1_l;
        opers->rs2 = rs2_l;
        opers->s = true;
    }
	if(hart->dbg) hart->print_d("{0x%.8X} [MUL] rs1: %d; rs2: %d; rd: %d",hart->pc,rs1_l,rs2_l,rd_l);
}
void exec_MULH(struct HART *hart, uint32_t inst, CACHE_DecodedOperands* opers) {
    uint64_t rd_l = opers->s ? opers->rd : rd(inst);
    uint64_t rs1_l = opers->s ? opers->rs1 : rs1(inst);
    uint64_t rs2_l = opers->s ? opers->rs2 : rs2(inst);

    __int128_t res = (__int128_t)(int64_t)hart->regs[rs1_l] * (__int128_t)(int64_t)hart->regs[rs2_l];
	hart->regs[rd_l] = (uint64_t)(res>>64);
	
    if(!opers->s){
        opers->rd = rd_l;
        opers->rs1 = rs1_l;
        opers->rs2 = rs2_l;
        opers->s = true;
    }
	if(hart->dbg) hart->print_d("{0x%.8X} [MULH] rs1: %d; rs2: %d; rd: %d",hart->pc,rs1_l,rs2_l,rd_l);
}
void exec_MULHSU(struct HART *hart, uint32_t inst, CACHE_DecodedOperands* opers) {
    uint64_t rd_l = opers->s ? opers->rd : rd(inst);
    uint64_t rs1_l = opers->s ? opers->rs1 : rs1(inst);
    uint64_t rs2_l = opers->s ? opers->rs2 : rs2(inst);

    __uint128_t res = (__uint128_t)(__int128_t)(int64_t)hart->regs[rs1_l] * (__uint128_t)hart->regs[rs2_l];
	hart->regs[rd_l] = (uint64_t)(res>>64);
	
    if(!opers->s){
        opers->rd = rd_l;
        opers->rs1 = rs1_l;
        opers->rs2 = rs2_l;
        opers->s = true;
    }
	if(hart->dbg) hart->print_d("{0x%.8X} [MULHSU] rs1: %d; rs2: %d; rd: %d",hart->pc,rs1_l,rs2_l,rd_l);
}
void exec_MULHU(struct HART *hart, uint32_t inst, CACHE_DecodedOperands* opers) {
    uint64_t rd_l = opers->s ? opers->rd : rd(inst);
    uint64_t rs1_l = opers->s ? opers->rs1 : rs1(inst);
    uint64_t rs2_l = opers->s ? opers->rs2 : rs2(inst);

    __uint128_t res = (__uint128_t)hart->regs[rs1_l] * (__uint128_t)hart->regs[rs2_l];
	hart->regs[rd_l] = (uint64_t)(res>>64);
	
    if(!opers->s){
        opers->rd = rd_l;
        opers->rs1 = rs1_l;
        opers->rs2 = rs2_l;
        opers->s = true;
    }
	if(hart->dbg) hart->print_d("{0x%.8X} [MULHU] rs1: %d; rs2: %d; rd: %d",hart->pc,rs1_l,rs2_l,rd_l);
}

void exec_DIV(struct HART *hart, uint32_t inst, CACHE_DecodedOperands* opers) {
    uint64_t rd_l = opers->s ? opers->rd : rd(inst);
    uint64_t rs1_l = opers->s ? opers->rs1 : rs1(inst);
    uint64_t rs2_l = opers->s ? opers->rs2 : rs2(inst);

    if(hart->regs[rs2_l] == 0) {
	    hart->regs[rd_l] = (uint64_t)-1;
    } else if(hart->regs[rs1_l] == std::numeric_limits<int64_t>::min() && hart->regs[rs2_l] == -1) {
        hart->regs[rd_l] = hart->regs[rs1_l];
    } else {
        hart->regs[rd_l] = (uint64_t)((int64_t)hart->regs[rs1_l] / (int64_t)hart->regs[rs2_l]);
    }
	
    if(!opers->s){
        opers->rd = rd_l;
        opers->rs1 = rs1_l;
        opers->rs2 = rs2_l;
        opers->s = true;
    }
	if(hart->dbg) hart->print_d("{0x%.8X} [DIV] rs1: %d; rs2: %d; rd: %d",hart->pc,rs1_l,rs2_l,rd_l);
}
void exec_DIVU(struct HART *hart, uint32_t inst, CACHE_DecodedOperands* opers) {
    uint64_t rd_l = opers->s ? opers->rd : rd(inst);
    uint64_t rs1_l = opers->s ? opers->rs1 : rs1(inst);
    uint64_t rs2_l = opers->s ? opers->rs2 : rs2(inst);

    if(hart->regs[rs2_l] == 0) {
	    hart->regs[rd_l] = std::numeric_limits<uint64_t>::max();
    } else {
        hart->regs[rd_l] = hart->regs[rs1_l] / hart->regs[rs2_l];
    }
	
    if(!opers->s){
        opers->rd = rd_l;
        opers->rs1 = rs1_l;
        opers->rs2 = rs2_l;
        opers->s = true;
    }
	if(hart->dbg) hart->print_d("{0x%.8X} [DIVU] rs1: %d; rs2: %d; rd: %d",hart->pc,rs1_l,rs2_l,rd_l);
}
void exec_REM(struct HART *hart, uint32_t inst, CACHE_DecodedOperands* opers) {
    uint64_t rd_l = opers->s ? opers->rd : rd(inst);
    uint64_t rs1_l = opers->s ? opers->rs1 : rs1(inst);
    uint64_t rs2_l = opers->s ? opers->rs2 : rs2(inst);

    if(hart->regs[rs2_l] == 0) {
	    hart->regs[rd_l] = hart->regs[rs1_l];
    } else if(hart->regs[rs1_l] == std::numeric_limits<int64_t>::min() && hart->regs[rs2_l] == -1) {
        hart->regs[rd_l] = 0;
    } else {
        hart->regs[rd_l] = (uint64_t)((int64_t)hart->regs[rs1_l] % (int64_t)hart->regs[rs2_l]);
    }
	
    if(!opers->s){
        opers->rd = rd_l;
        opers->rs1 = rs1_l;
        opers->rs2 = rs2_l;
        opers->s = true;
    }
	if(hart->dbg) hart->print_d("{0x%.8X} [REM] rs1: %d; rs2: %d; rd: %d",hart->pc,rs1_l,rs2_l,rd_l);
}
void exec_REMU(struct HART *hart, uint32_t inst, CACHE_DecodedOperands* opers) {
    uint64_t rd_l = opers->s ? opers->rd : rd(inst);
    uint64_t rs1_l = opers->s ? opers->rs1 : rs1(inst);
    uint64_t rs2_l = opers->s ? opers->rs2 : rs2(inst);

    if(hart->regs[rs2_l] == 0) {
	    hart->regs[rd_l] = hart->regs[rs1_l];
    } else {
        hart->regs[rd_l] = hart->regs[rs1_l] % (int64_t)hart->regs[rs2_l];
    }
	
    if(!opers->s){
        opers->rd = rd_l;
        opers->rs1 = rs1_l;
        opers->rs2 = rs2_l;
        opers->s = true;
    }
	if(hart->dbg) hart->print_d("{0x%.8X} [REMU] rs1: %d; rs2: %d; rd: %d",hart->pc,rs1_l,rs2_l,rd_l);
}