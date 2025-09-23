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

void exec_MULW(struct HART *hart, uint32_t inst, CACHE_DecodedOperands* opers) {
    uint64_t rd_l = opers->s ? opers->rd : rd(inst);
    uint64_t rs1_l = opers->s ? opers->rs1 : rs1(inst);
    uint64_t rs2_l = opers->s ? opers->rs2 : rs2(inst);

	hart->regs[rd_l] = (uint64_t)((int32_t)hart->regs[rs1_l] * (int32_t)hart->regs[rs2_l]);

    if(!opers->s){
        opers->rd = rd_l;
        opers->rs1 = rs1_l;
        opers->rs2 = rs2_l;
        opers->s = true;
    }
	if(hart->dbg) hart->print_d("{0x%.8X} [MULW] rs1: %d; rs2: %d; rd: %d",hart->pc,rs1_l,rs2_l,rd_l);
}

void exec_DIVW(struct HART *hart, uint32_t inst, CACHE_DecodedOperands* opers) {
    uint64_t rd_l = opers->s ? opers->rd : rd(inst);
    uint64_t rs1_l = opers->s ? opers->rs1 : rs1(inst);
    uint64_t rs2_l = opers->s ? opers->rs2 : rs2(inst);

    if(hart->regs[rs2_l] == 0) {
	    hart->regs[rd_l] = (int64_t)-1;
    } else if(hart->regs[rs1_l] == std::numeric_limits<int32_t>::min() && hart->regs[rs2_l] == -1) {
        hart->regs[rd_l] = hart->regs[rs1_l];
    } else {
        hart->regs[rd_l] = (uint64_t)((int32_t)hart->regs[rs1_l] / (int32_t)hart->regs[rs2_l]);
    }
    
    if(!opers->s){
        opers->rd = rd_l;
        opers->rs1 = rs1_l;
        opers->rs2 = rs2_l;
        opers->s = true;
    }
	if(hart->dbg) hart->print_d("{0x%.8X} [DIVW] rs1: %d; rs2: %d; rd: %d",hart->pc,rs1_l,rs2_l,rd_l);
}
void exec_DIVUW(struct HART *hart, uint32_t inst, CACHE_DecodedOperands* opers) {
    uint64_t rd_l = opers->s ? opers->rd : rd(inst);
    uint64_t rs1_l = opers->s ? opers->rs1 : rs1(inst);
    uint64_t rs2_l = opers->s ? opers->rs2 : rs2(inst);

    if(hart->regs[rs2_l] == 0) {
	    hart->regs[rd_l] = std::numeric_limits<uint64_t>::max();
    } else {
        hart->regs[rd_l] = (uint64_t)(int64_t)(int32_t)(hart->regs[rs1_l] / hart->regs[rs2_l]);
    }
    
    if(!opers->s){
        opers->rd = rd_l;
        opers->rs1 = rs1_l;
        opers->rs2 = rs2_l;
        opers->s = true;
    }
	if(hart->dbg) hart->print_d("{0x%.8X} [DIVUW] rs1: %d; rs2: %d; rd: %d",hart->pc,rs1_l,rs2_l,rd_l);
}
void exec_REMW(struct HART *hart, uint32_t inst, CACHE_DecodedOperands* opers) {
    uint64_t rd_l = opers->s ? opers->rd : rd(inst);
    uint64_t rs1_l = opers->s ? opers->rs1 : rs1(inst);
    uint64_t rs2_l = opers->s ? opers->rs2 : rs2(inst);

    if(hart->regs[rs2_l] == 0) {
	    hart->regs[rd_l] = hart->regs[rs1_l];
    } else if(hart->regs[rs1_l] == std::numeric_limits<int32_t>::min() && hart->regs[rs2_l] == -1) {
        hart->regs[rd_l] = 0;
    } else {
        hart->regs[rd_l] = (uint64_t)(int64_t)((int32_t)hart->regs[rs1_l] % (int32_t)hart->regs[rs2_l]);
    }
    
    if(!opers->s){
        opers->rd = rd_l;
        opers->rs1 = rs1_l;
        opers->rs2 = rs2_l;
        opers->s = true;
    }
	if(hart->dbg) hart->print_d("{0x%.8X} [REMW] rs1: %d; rs2: %d; rd: %d",hart->pc,rs1_l,rs2_l,rd_l);
}
void exec_REMUW(struct HART *hart, uint32_t inst, CACHE_DecodedOperands* opers) {
    uint64_t rd_l = opers->s ? opers->rd : rd(inst);
    uint64_t rs1_l = opers->s ? opers->rs1 : rs1(inst);
    uint64_t rs2_l = opers->s ? opers->rs2 : rs2(inst);

    if(hart->regs[rs2_l] == 0) {
	    hart->regs[rd_l] = hart->regs[rs1_l];
    } else {
        hart->regs[rd_l] = (uint64_t)(int64_t)(int32_t)((uint32_t)hart->regs[rs1_l] % (uint32_t)hart->regs[rs2_l]);
    }
    
    if(!opers->s){
        opers->rd = rd_l;
        opers->rs1 = rs1_l;
        opers->rs2 = rs2_l;
        opers->s = true;
    }
	if(hart->dbg) hart->print_d("{0x%.8X} [REMUW] rs1: %d; rs2: %d; rd: %d",hart->pc,rs1_l,rs2_l,rd_l);
}