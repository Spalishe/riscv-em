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
#include "../../include/decode.h"

// R-Type

bool exec_ADDW(HART *hart, inst_data& inst) {
	hart->GPR[inst.rd] = (int32_t) ((uint32_t)hart->GPR[inst.rs1] + (uint32_t)hart->GPR[inst.rs2]);
    return true;
}
bool exec_SUBW(HART *hart, inst_data& inst) {
	hart->GPR[inst.rd] = (int32_t) ((uint32_t)hart->GPR[inst.rs1] - (uint32_t)hart->GPR[inst.rs2]);
    return true;
}
bool exec_SLLW(HART *hart, inst_data& inst) {
	hart->GPR[inst.rd] = (int32_t) ((uint32_t)hart->GPR[inst.rs1] << ((uint32_t)hart->GPR[inst.rs2] & 0x1F));
    return true;
}
bool exec_SRLW(HART *hart, inst_data& inst) {
    hart->GPR[inst.rd] = (int32_t) ((uint32_t)hart->GPR[inst.rs1] >> ((uint32_t)hart->GPR[inst.rs2] & 0x1F));
    return true;
}
bool exec_SRAW(HART *hart, inst_data& inst) {
    hart->GPR[inst.rd] = (int32_t) (((int32_t) hart->GPR[inst.rs1]) >> ((uint32_t)hart->GPR[inst.rs2] & 0x1F));
    return true;
}

// I-Type
bool exec_ADDIW(HART *hart, inst_data& inst) {
    hart->GPR[inst.rd] = (int32_t) ((uint32_t)hart->GPR[inst.rs1] + inst.imm);
    return true;
}
bool exec_SLLIW(HART *hart, inst_data& inst) {
    hart->GPR[inst.rd] = (int32_t) ((uint32_t)hart->GPR[inst.rs1] << inst.imm);
    return true;
}
bool exec_SRLIW(HART *hart, inst_data& inst) {
    hart->GPR[inst.rd] = (int32_t) ((uint32_t)hart->GPR[inst.rs1] >> inst.imm);
    return true;
}
bool exec_SRAIW(HART *hart, inst_data& inst) {
    hart->GPR[inst.rd] = ((int32_t)hart->GPR[inst.rs1]) >> inst.imm;
    return true;
}

bool exec_LD(HART *hart, inst_data& inst) {
    uint64_t addr = hart->GPR[inst.rs1] + (int64_t) inst.imm;
    std::optional<uint64_t> val = hart->mmio->load(hart,addr,64);
    if(val.has_value()) {
        hart->GPR[inst.rd] = *val;
    }
    return val.has_value();
}
bool exec_LWU(HART *hart, inst_data& inst) {
    uint64_t addr = hart->GPR[inst.rs1] + (int64_t) inst.imm;
    std::optional<uint64_t> val = hart->mmio->load(hart,addr,32);
    if(val.has_value()) {
        hart->GPR[inst.rd] = (uint32_t) *val;
    }
    return val.has_value();
}

// S-Type
bool exec_SD(HART *hart, inst_data& inst) {
    uint64_t addr = hart->GPR[inst.rs1] + (int64_t) inst.imm;
    return hart->mmio->store(hart,addr,64,hart->GPR[inst.rs2]);
}

/// RV32I

// R-Type

bool exec_ADD(HART *hart, inst_data& inst) {
	hart->GPR[inst.rd] = hart->GPR[inst.rs1] + hart->GPR[inst.rs2];
    return true;
}
bool exec_SUB(HART *hart, inst_data& inst) {
	hart->GPR[inst.rd] = hart->GPR[inst.rs1] - hart->GPR[inst.rs2];
    return true;
}
bool exec_XOR(HART *hart, inst_data& inst) {
	hart->GPR[inst.rd] = hart->GPR[inst.rs1] ^ hart->GPR[inst.rs2];
    return true;
}
bool exec_OR(HART *hart, inst_data& inst) {
	hart->GPR[inst.rd] = hart->GPR[inst.rs1] | hart->GPR[inst.rs2];
    return true;
}
bool exec_AND(HART *hart, inst_data& inst) {
	hart->GPR[inst.rd] = hart->GPR[inst.rs1] & hart->GPR[inst.rs2];
    return true;
}
bool exec_SLL(HART *hart, inst_data& inst) {
	hart->GPR[inst.rd] = hart->GPR[inst.rs1] << (hart->GPR[inst.rs2] & 0x3f);
    return true;
}
bool exec_SRL(HART *hart, inst_data& inst) {
	hart->GPR[inst.rd] = hart->GPR[inst.rs1] >> (hart->GPR[inst.rs2] & 0x3f);
    return true;
}
bool exec_SRA(HART *hart, inst_data& inst) {
	hart->GPR[inst.rd] = ((int64_t) hart->GPR[inst.rs1]) >> (hart->GPR[inst.rs2] & 0x3f);
    return true;
}
bool exec_SLT(HART *hart, inst_data& inst) {
	hart->GPR[inst.rd] = ((int64_t) hart->GPR[inst.rs1] < (int64_t) hart->GPR[inst.rs2]) ? 1 : 0;
    return true;
}
bool exec_SLTU(HART *hart, inst_data& inst) {
	hart->GPR[inst.rd] = (hart->GPR[inst.rs1] < hart->GPR[inst.rs2]) ? 1 : 0;
    return true;
}

// I-Type
bool exec_ADDI(HART *hart, inst_data& inst) {
    hart->GPR[inst.rd] = hart->GPR[inst.rs1] + (int64_t) inst.imm;
    return true;
}
bool exec_XORI(HART *hart, inst_data& inst) {
    hart->GPR[inst.rd] = hart->GPR[inst.rs1] ^ inst.imm;
    return true;
}
bool exec_ORI(HART *hart, inst_data& inst) {
    hart->GPR[inst.rd] = hart->GPR[inst.rs1] | inst.imm;
    return true;
}
bool exec_ANDI(HART *hart, inst_data& inst) {
    hart->GPR[inst.rd] = hart->GPR[inst.rs1] & inst.imm;
    return true;
}
bool exec_SLLI(HART *hart, inst_data& inst) {
    hart->GPR[inst.rd] = hart->GPR[inst.rs1] << inst.imm;
    return true;
}
bool exec_SRLI(HART *hart, inst_data& inst) {
    hart->GPR[inst.rd] = hart->GPR[inst.rs1] >> inst.imm;
    return true;
}
bool exec_SRAI(HART *hart, inst_data& inst) {
    hart->GPR[inst.rd] = ((int64_t)hart->GPR[inst.rs1]) >> inst.imm;
    return true;
}
bool exec_SLTI(HART *hart, inst_data& inst) {
    hart->GPR[inst.rd] = ((int64_t) hart->GPR[inst.rs1] < (int64_t) inst.imm) ? 1 : 0;
    return true;
}
bool exec_SLTIU(HART *hart, inst_data& inst) {
    hart->GPR[inst.rd] = (hart->GPR[inst.rs1] < inst.imm) ? 1 : 0;
    return true;
}
bool exec_LB(HART *hart, inst_data& inst) {	
    uint64_t addr = hart->GPR[inst.rs1] + (int64_t) inst.imm;
    std::optional<uint64_t> val = hart->mmio->load(hart,addr,8);
    if(val.has_value()) {
        hart->GPR[inst.rd] = (uint64_t)(int8_t) *val;
    }
    return val.has_value();
}
bool exec_LH(HART *hart, inst_data& inst) {
    uint64_t addr = hart->GPR[inst.rs1] + (int64_t) inst.imm;
    std::optional<uint64_t> val = hart->mmio->load(hart,addr,16);
    if(val.has_value()) {
        hart->GPR[inst.rd] = (uint64_t)(int16_t) *val;
    }
    return val.has_value();
}
bool exec_LW(HART *hart, inst_data& inst) {
    uint64_t addr = hart->GPR[inst.rs1] + (int64_t) inst.imm;
    std::optional<uint64_t> val = hart->mmio->load(hart,addr,32);
    if(val.has_value()) {
        hart->GPR[inst.rd] = (uint64_t)(int32_t) *val;
    }
    return val.has_value();
}
bool exec_LBU(HART *hart, inst_data& inst) {
    uint64_t addr = hart->GPR[inst.rs1] + (int64_t) inst.imm;
    std::optional<uint64_t> val = hart->mmio->load(hart,addr,8);
    if(val.has_value()) {
        hart->GPR[inst.rd] = (uint64_t)(uint8_t) *val;
    }
    return val.has_value();
}
bool exec_LHU(HART *hart, inst_data& inst) {
    uint64_t addr = hart->GPR[inst.rs1] + (int64_t) inst.imm;
    std::optional<uint64_t> val = hart->mmio->load(hart,addr,16);
    if(val.has_value()) {
        hart->GPR[inst.rd] = (uint64_t)(uint16_t) *val;
    }
    return val.has_value();
}

// S-Type
bool exec_SB(HART *hart, inst_data& inst) {
    uint64_t addr = hart->GPR[inst.rs1] + (int64_t) inst.imm;
    return hart->mmio->store(hart,addr,8,hart->GPR[inst.rs2]);
}
bool exec_SH(HART *hart, inst_data& inst) {
    uint64_t addr = hart->GPR[inst.rs1] + (int64_t) inst.imm;
    return hart->mmio->store(hart,addr,16,hart->GPR[inst.rs2]);
}
bool exec_SW(HART *hart, inst_data& inst) {
    uint64_t addr = hart->GPR[inst.rs1] + (int64_t) inst.imm;
    return hart->mmio->store(hart,addr,32,hart->GPR[inst.rs2]);
}

// B-Type
bool exec_BEQ(HART *hart, inst_data& inst) {
    if ((int64_t) hart->GPR[inst.rs1] == (int64_t) hart->GPR[inst.rs2]) {
        if((hart->pc + (int64_t) inst.imm) % 4 != 0) {
            hart_trap(*hart,EXC_INST_ADDR_MISALIGNED,hart->pc + (int64_t) inst.imm,false);
            return false;
        } else
            hart->pc = hart->pc + (int64_t) inst.imm - 4;
    }
    return true;
}
bool exec_BNE(HART *hart, inst_data& inst) {
    if ((int64_t) hart->GPR[inst.rs1] != (int64_t) hart->GPR[inst.rs2]) {
        if((hart->pc + (int64_t) inst.imm) % 4 != 0) {
            hart_trap(*hart,EXC_INST_ADDR_MISALIGNED,hart->pc + (int64_t) inst.imm,false);
            return false;
        } else
            hart->pc = hart->pc + (int64_t) inst.imm - 4;
    }
    return true;
}
bool exec_BLT(HART *hart, inst_data& inst) {
    if ((int64_t) hart->GPR[inst.rs1] < (int64_t) hart->GPR[inst.rs2]) {
        if((hart->pc + (int64_t) inst.imm) % 4 != 0) {
            hart_trap(*hart,EXC_INST_ADDR_MISALIGNED,hart->pc + (int64_t) inst.imm,false);
            return false;
        } else
            hart->pc = hart->pc + (int64_t) inst.imm - 4;
    }
    return true;
}
bool exec_BGE(HART *hart, inst_data& inst) {
    if ((int64_t) hart->GPR[inst.rs1] >= (int64_t) hart->GPR[inst.rs2]) {
        if((hart->pc + (int64_t) inst.imm) % 4 != 0) {
            hart_trap(*hart,EXC_INST_ADDR_MISALIGNED,hart->pc + (int64_t) inst.imm,false);
            return false;
        } else
            hart->pc = hart->pc + (int64_t) inst.imm - 4;
    }
    return true;
}
bool exec_BLTU(HART *hart, inst_data& inst) {
    if (hart->GPR[inst.rs1] < hart->GPR[inst.rs2]) {
        if((hart->pc + (int64_t) inst.imm) % 4 != 0) {
            hart_trap(*hart,EXC_INST_ADDR_MISALIGNED,hart->pc + (int64_t) inst.imm,false);
            return false;
        } else
            hart->pc = hart->pc + (int64_t) inst.imm - 4;
    }
    return true;
}
bool exec_BGEU(HART *hart, inst_data& inst) {
    if (hart->GPR[inst.rs1] >= hart->GPR[inst.rs2]) {
        if((hart->pc + (int64_t) inst.imm) % 4 != 0) {
            hart_trap(*hart,EXC_INST_ADDR_MISALIGNED,hart->pc + (int64_t) inst.imm,false);
            return false;
        } else
            hart->pc = hart->pc + (int64_t) inst.imm - 4;
    }
    return true;
}

// JUMP
bool exec_JAL(HART *hart, inst_data& inst) {
	uint64_t tmp = hart->pc;
    if((hart->pc + (int64_t) inst.imm) % 4 != 0) {
        hart_trap(*hart,EXC_INST_ADDR_MISALIGNED,hart->pc + (int64_t) inst.imm,false);
        return false;
    } else {
        hart->pc = hart->pc + (int64_t) inst.imm - 4;
        hart->GPR[inst.rd] = tmp+4;
    }
    return true;
}
bool exec_JALR(HART *hart, inst_data& inst) {
	uint64_t tmp = hart->pc;
    uint64_t target = (hart->GPR[inst.rs1] + (int64_t) inst.imm) & ~1;
    if(target % 4 != 0) {
        hart_trap(*hart,EXC_INST_ADDR_MISALIGNED,target,false);
        return false;
    } else {
        hart->pc = target - 4;
	    hart->GPR[inst.rd] = tmp+4;
    }
    return true;
}

// what

bool exec_LUI(HART *hart, inst_data& inst) {
	hart->GPR[inst.rd] = (((int64_t) (int32_t) inst.imm) << 12);
    return true;
}
bool exec_AUIPC(HART *hart, inst_data& inst) {
    hart->GPR[inst.rd] = hart->pc + ((((int64_t) (int32_t) inst.imm) << 12));
    return true;
}

bool exec_ECALL(HART *hart, inst_data& inst) {
	switch(hart->mode) {
		case PrivilegeMode::Machine: hart_trap(*hart,EXC_ENV_CALL_FROM_M,0,false); break;
		case PrivilegeMode::Supervisor: hart_trap(*hart,EXC_ENV_CALL_FROM_S,0,false); break;
		case PrivilegeMode::User: hart_trap(*hart,EXC_ENV_CALL_FROM_U,0,false); break;
	}
    return false;
}
bool exec_EBREAK(HART *hart, inst_data& inst) {
	hart_trap(*hart,EXC_BREAKPOINT,hart->pc,false);
    return false;
}


bool exec_FENCE(HART *hart, inst_data& inst) {
    // nop
    // If you planning adding some memory write/read buffer, you have to implement this instruction then
    // FENCE guaranties that all cores will see all changes that have done by 1 specific core before FENCE instruction
    return true;
}