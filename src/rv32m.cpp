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

void exec_MUL(struct HART *hart, uint32_t inst) {
	hart->regs[rd(inst)] = hart->regs[rs1(inst)] * hart->regs[rs2(inst)];
	hart->print_d("{0x%.8X} [MUL] rs1: %d; rs2: %d; rd: %d",hart->pc,rs1(inst),rs2(inst),rd(inst));
}
void exec_MULH(struct HART *hart, uint32_t inst) {
    __int128_t res = (__int128_t)(int64_t)hart->regs[rs1(inst)] * (__int128_t)(int64_t)hart->regs[rs2(inst)];
	hart->regs[rd(inst)] = (uint64_t)(res>>64);
	hart->print_d("{0x%.8X} [MULH] rs1: %d; rs2: %d; rd: %d",hart->pc,rs1(inst),rs2(inst),rd(inst));
}
void exec_MULHSU(struct HART *hart, uint32_t inst) {
    __uint128_t res = (__uint128_t)(__int128_t)(int64_t)hart->regs[rs1(inst)] * (__uint128_t)hart->regs[rs2(inst)];
	hart->regs[rd(inst)] = (uint64_t)(res>>64);
	hart->print_d("{0x%.8X} [MULHSU] rs1: %d; rs2: %d; rd: %d",hart->pc,rs1(inst),rs2(inst),rd(inst));
}
void exec_MULHU(struct HART *hart, uint32_t inst) {
    __uint128_t res = (__uint128_t)hart->regs[rs1(inst)] * (__uint128_t)hart->regs[rs2(inst)];
	hart->regs[rd(inst)] = (uint64_t)(res>>64);
	hart->print_d("{0x%.8X} [MULHU] rs1: %d; rs2: %d; rd: %d",hart->pc,rs1(inst),rs2(inst),rd(inst));
}

void exec_DIV(struct HART *hart, uint32_t inst) {
    if(hart->regs[rs2(inst)] == 0) {
	    hart->regs[rd(inst)] = (uint64_t)-1;
    } else if(hart->regs[rs1(inst)] == std::numeric_limits<int64_t>::min() && hart->regs[rs2(inst)] == -1) {
        hart->regs[rd(inst)] = hart->regs[rs1(inst)];
    } else {
        hart->regs[rd(inst)] = (uint64_t)((int64_t)hart->regs[rs1(inst)] / (int64_t)hart->regs[rs2(inst)]);
    }
	hart->print_d("{0x%.8X} [DIV] rs1: %d; rs2: %d; rd: %d",hart->pc,rs1(inst),rs2(inst),rd(inst));
}
void exec_DIVU(struct HART *hart, uint32_t inst) {
    if(hart->regs[rs2(inst)] == 0) {
	    hart->regs[rd(inst)] = std::numeric_limits<uint64_t>::max();
    } else {
        hart->regs[rd(inst)] = hart->regs[rs1(inst)] / hart->regs[rs2(inst)];
    }
	hart->print_d("{0x%.8X} [DIVU] rs1: %d; rs2: %d; rd: %d",hart->pc,rs1(inst),rs2(inst),rd(inst));
}
void exec_REM(struct HART *hart, uint32_t inst) {
    if(hart->regs[rs2(inst)] == 0) {
	    hart->regs[rd(inst)] = hart->regs[rs1(inst)];
    } else if(hart->regs[rs1(inst)] == std::numeric_limits<int64_t>::min() && hart->regs[rs2(inst)] == -1) {
        hart->regs[rd(inst)] = 0;
    } else {
        hart->regs[rd(inst)] = (uint64_t)((int64_t)hart->regs[rs1(inst)] % (int64_t)hart->regs[rs2(inst)]);
    }
	hart->print_d("{0x%.8X} [REM] rs1: %d; rs2: %d; rd: %d",hart->pc,rs1(inst),rs2(inst),rd(inst));
}
void exec_REMU(struct HART *hart, uint32_t inst) {
    if(hart->regs[rs2(inst)] == 0) {
	    hart->regs[rd(inst)] = hart->regs[rs1(inst)];
    } else {
        hart->regs[rd(inst)] = hart->regs[rs1(inst)] % (int64_t)hart->regs[rs2(inst)];
    }
	hart->print_d("{0x%.8X} [REMU] rs1: %d; rs2: %d; rd: %d",hart->pc,rs1(inst),rs2(inst),rd(inst));
}