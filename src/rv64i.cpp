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

void exec_ADDW(struct HART *hart, uint32_t inst) {
	hart->regs[rd(inst)] = (int32_t) ((int32_t) hart->regs[rs1(inst)] + (int32_t) hart->regs[rs2(inst)]);
	hart->print_d("{0x%.8X} [ADDW] rs1: %d; rs2: %d; rd: %d",hart->pc,rs1(inst),rs2(inst),rd(inst));
}
void exec_SUBW(struct HART *hart, uint32_t inst) {
	hart->regs[rd(inst)] = (int32_t) ((int32_t) hart->regs[rs1(inst)] - (int32_t) hart->regs[rs2(inst)]);
	hart->print_d("{0x%.8X} [SUBW] rs1: %d; rs2: %d; rd: %d",hart->pc,rs1(inst),rs2(inst),rd(inst));
}
void exec_SLLW(struct HART *hart, uint32_t inst) {
	hart->regs[rd(inst)] = (int32_t) ((int32_t)hart->regs[rs1(inst)] << (int32_t)(hart->regs[rs2(inst)] & 0x1F));
	hart->print_d("{0x%.8X} [SLLW] rs1: %d; rs2: %d; rd: %d",hart->pc,rs1(inst),rs2(inst),rd(inst));
}
void exec_SRLW(struct HART *hart, uint32_t inst) {
	hart->regs[rd(inst)] = (int32_t) ((uint32_t)hart->regs[rs1(inst)] >> (int32_t)(hart->regs[rs2(inst)] & 0x1F));
	hart->print_d("{0x%.8X} [SRLW] rs1: %d; rs2: %d; rd: %d",hart->pc,rs1(inst),rs2(inst),rd(inst));
}
void exec_SRAW(struct HART *hart, uint32_t inst) {
	hart->regs[rd(inst)] = (int64_t) ((int32_t) hart->regs[rs1(inst)] >> (int64_t) hart->regs[rs2(inst)]);
	hart->print_d("{0x%.8X} [SRAW] rs1: %d; rs2: %d; rd: %d",hart->pc,rs1(inst),rs2(inst),rd(inst));
}

// I-Type
void exec_ADDIW(struct HART *hart, uint32_t inst) {
    hart->regs[rd(inst)] = (int32_t) ((int32_t)hart->regs[rs1(inst)] + (int32_t) imm_I(inst));
	hart->print_d("{0x%.8X} [ADDIW] rs1: %d; rd: %d; imm: int %d uint %u",hart->pc,rs1(inst),rd(inst),(int64_t) imm_I(inst), imm_I(inst));
}
void exec_SLLIW(struct HART *hart, uint32_t inst) {
    hart->regs[rd(inst)] = (int32_t) ((int32_t)hart->regs[rs1(inst)] << (uint32_t)shamt(inst));
	hart->print_d("{0x%.8X} [SLLIW] rs1: %d; rd: %d; imm: int %d uint %u",hart->pc,rs1(inst),rd(inst),(int64_t) shamt(inst), shamt(inst));
}
void exec_SRLIW(struct HART *hart, uint32_t inst) {
    hart->regs[rd(inst)] = (int32_t) ((uint32_t)hart->regs[rs1(inst)] >> (uint32_t)shamt(inst));
	hart->print_d("{0x%.8X} [SRLIW] rs1: %d; rd: %d; imm: int %d uint %u",hart->pc,rs1(inst),rd(inst),(int64_t) shamt(inst), shamt(inst));
}
void exec_SRAIW(struct HART *hart, uint32_t inst) {
    hart->regs[rd(inst)] = (int64_t) ((int32_t)hart->regs[rs1(inst)] >> (uint32_t)shamt(inst));
	hart->print_d("{0x%.8X} [SRAIW] rs1: %d; rd: %d; imm: int %d uint %u",hart->pc,rs1(inst),rd(inst),(int64_t) shamt(inst), shamt(inst));
}

void exec_LD(struct HART *hart, uint32_t inst) {
	uint64_t imm = imm_I(inst);
	uint64_t addr = hart->regs[rs1(inst)] + (int64_t) imm;
	std::optional<uint64_t> val = hart->mmio->load(hart,addr,64);
	if(val.has_value()) {
		hart->regs[rd(inst)] = (int64_t) *val;
	}
	hart->print_d("{0x%.8X} [LD] rs1: %d; rd: %d; imm: int %d uint %u",hart->pc,rs1(inst),rd(inst),(int64_t) imm_I(inst), imm_I(inst));
}
void exec_LWU(struct HART *hart, uint32_t inst) {
	uint64_t imm = imm_I(inst);
	uint64_t addr = hart->regs[rs1(inst)] + (int64_t) imm;
	std::optional<uint64_t> val = hart->mmio->load(hart,addr,32);
	if(val.has_value()) {
		hart->regs[rd(inst)] = (uint32_t) *val;
	}
	hart->print_d("{0x%.8X} [LWU] rs1: %d; rd: %d; imm: int %d uint %u",hart->pc,rs1(inst),rd(inst),(int64_t) imm_I(inst), imm_I(inst));
}

// S-Type
void exec_SD(struct HART *hart, uint32_t inst) {
	uint64_t imm = imm_S(inst);
	uint64_t addr = hart->regs[rs1(inst)] + (int64_t) imm;
	hart->mmio->store(hart,addr,64,hart->regs[rs2(inst)]);
	hart->print_d("{0x%.8X} [SD] rs1: %d; rs2: %d; imm: int %d uint %u",hart->pc,rs1(inst),rs2(inst),(int64_t) imm_S(inst), imm_S(inst));
}