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

inst_ret exec_ANDN(HART *hart, inst_data& inst) {
	hart->GPR[inst.rd] = hart->GPR[inst.rs1] & ~hart->GPR[inst.rs2];
    return true;
}
inst_ret exec_ORN(HART *hart, inst_data& inst) {
	hart->GPR[inst.rd] = hart->GPR[inst.rs1] | ~hart->GPR[inst.rs2];
    return true;
}
inst_ret exec_XNOR(HART *hart, inst_data& inst) {
	hart->GPR[inst.rd] = ~(hart->GPR[inst.rs1] ^ hart->GPR[inst.rs2]);
    return true;
}
inst_ret exec_CLZ(HART *hart, inst_data& inst) {
    uint64_t c = -1;
    for(int i = 63; i >= 0; i--) {
        uint8_t bit = (hart->GPR[inst.rs1] >> i) & 1;
        if(bit) {c = i; break;}
    }
	hart->GPR[inst.rd] = 63 - c;
    
    return true;
}
inst_ret exec_CLZW(HART *hart, inst_data& inst) {
    uint64_t c = -1;
    for(int i = 31; i >= 0; i--) {
        uint8_t bit = ((uint32_t)hart->GPR[inst.rs1] >> i) & 1;
        if(bit) {c = i; break;}
    }
	hart->GPR[inst.rd] = 31 - c;
    
    return true;
}
inst_ret exec_CTZ(HART *hart, inst_data& inst) {
    uint64_t c = 64;
    for(int i = 0; i < 64; i++) {
        uint8_t bit = (hart->GPR[inst.rs1] >> i) & 1;
        if(bit) {c = i; break;}
    }
	hart->GPR[inst.rd] = c;
    
    return true;
}
inst_ret exec_CTZW(HART *hart, inst_data& inst) {
    uint64_t c = 32;
    for(int i = 0; i < 32; i++) {
        uint8_t bit = ((uint32_t)hart->GPR[inst.rs1] >> i) & 1;
        if(bit) {c = i; break;}
    }
	hart->GPR[inst.rd] = c;
    
    return true;
}

inst_ret exec_CPOP(HART *hart, inst_data& inst) {
    uint8_t bitcount = 0;
    for(int i = 0; i < 64; i++) {
        uint8_t bit = (hart->GPR[inst.rs1] >> i) & 1;
        bitcount += bit;
    }
	hart->GPR[inst.rd] = bitcount;
    return true;
}
inst_ret exec_CPOPW(HART *hart, inst_data& inst) {
    uint8_t bitcount = 0;
    for(int i = 0; i < 32; i++) {
        uint8_t bit = ((uint32_t)hart->GPR[inst.rs1] >> i) & 1;
        bitcount += bit;
    }
	hart->GPR[inst.rd] = bitcount;
    return true;
}

inst_ret exec_MAX(HART *hart, inst_data& inst) {
    hart->GPR[inst.rd] = max((int64_t)hart->GPR[inst.rs1],(int64_t)hart->GPR[inst.rs2]);
    return true;
}
inst_ret exec_MAXU(HART *hart, inst_data& inst) {
    hart->GPR[inst.rd] = max(hart->GPR[inst.rs1],hart->GPR[inst.rs2]);
    return true;
}
inst_ret exec_MIN(HART *hart, inst_data& inst) {
    hart->GPR[inst.rd] = min((int64_t)hart->GPR[inst.rs1],(int64_t)hart->GPR[inst.rs2]);
    return true;
}
inst_ret exec_MINU(HART *hart, inst_data& inst) {
    hart->GPR[inst.rd] = min(hart->GPR[inst.rs1],hart->GPR[inst.rs2]);
    return true;
}

inst_ret exec_SEXT_B(HART *hart, inst_data& inst) {
    hart->GPR[inst.rd] = (int64_t)(int8_t)hart->GPR[inst.rs1];
    return true;
}
inst_ret exec_SEXT_H(HART *hart, inst_data& inst) {
    hart->GPR[inst.rd] = (int64_t)(int16_t)hart->GPR[inst.rs1];
    return true;
}
inst_ret exec_ZEXT_H(HART *hart, inst_data& inst) {
    hart->GPR[inst.rd] = hart->GPR[inst.rs1] & 0xFFFF;
    return true;
}

inst_ret exec_ROL(HART *hart, inst_data& inst) {
    uint64_t rs1 = hart->GPR[inst.rs1];
    uint64_t shamt = hart->GPR[inst.rs2] & 0x3F;
    hart->GPR[inst.rd] = (rs1 << shamt) | (rs1 >> (64 - shamt));
    return true;
}
inst_ret exec_ROLW(HART *hart, inst_data& inst) {
    uint64_t rs1 = (uint32_t)hart->GPR[inst.rs1];
    uint64_t shamt = hart->GPR[inst.rs2] & 0x1F;
    hart->GPR[inst.rd] = (uint64_t)(int64_t)(int32_t)((rs1 << shamt) | (rs1 >> (32 - shamt)));
    return true;
}
inst_ret exec_ROR(HART *hart, inst_data& inst) {
    uint64_t rs1 = hart->GPR[inst.rs1];
    uint64_t shamt = hart->GPR[inst.rs2] & 0x3F;
    hart->GPR[inst.rd] = (rs1 >> shamt) | (rs1 << (64 - shamt));
    return true;
}
inst_ret exec_RORI(HART *hart, inst_data& inst) {
    uint64_t rs1 = hart->GPR[inst.rs1];
    hart->GPR[inst.rd] = (rs1 >> inst.imm) | (rs1 << (64 - inst.imm));
    return true;
}
inst_ret exec_RORIW(HART *hart, inst_data& inst) {
    uint64_t rs1 = (uint32_t)hart->GPR[inst.rs1];
    hart->GPR[inst.rd] = (rs1 >> inst.imm) | (rs1 << (32 - inst.imm));
    return true;
}
inst_ret exec_RORW(HART *hart, inst_data& inst) {
    uint64_t rs1 = (uint32_t)hart->GPR[inst.rs1];
    uint64_t shamt = hart->GPR[inst.rs2] & 0x1F;
    hart->GPR[inst.rd] = (uint64_t)(int64_t)(int32_t)((rs1 >> shamt) | (rs1 << (32 - shamt)));
    return true;
}

inst_ret exec_ORC_B(HART *hart, inst_data& inst) {
    uint64_t in = hart->GPR[inst.rs1];
    uint64_t out = 0;
    for(int i = 0; i < 64; i+=8) {
        uint8_t b = (in >> i) & 0xFF;
        uint64_t val = (b == 0) ? 0x00 : 0xFF;
        out |= (val << i);
    }
    hart->GPR[inst.rd] = out;
    return true;
}

inst_ret exec_REV8(HART *hart, inst_data& inst) {
    uint64_t in = hart->GPR[inst.rs1];
    uint64_t out = 0;
    for (int i = 0; i < 64 / 8; i++) {
        out |= ((in >> (i * 8)) & 0xFF) << ((64 / 8 - 1 - i) * 8);
    }
    hart->GPR[inst.rd] = out;
    return true;
}
