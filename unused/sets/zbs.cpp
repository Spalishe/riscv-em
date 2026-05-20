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

inst_ret exec_BCLR(HART *hart, inst_data& inst) {
    uint64_t index = hart->GPR[inst.rs2] & 0x3f;
    hart->GPR[inst.rd] = hart->GPR[inst.rs1] & ~((uint64_t)1 << index);
    return true;
}
inst_ret exec_BCLRI(HART *hart, inst_data& inst) {
    uint64_t index = inst.imm & 0x3f;
    hart->GPR[inst.rd] = hart->GPR[inst.rs1] & ~((uint64_t)1 << index);
    return true;
}

inst_ret exec_BEXT(HART *hart, inst_data& inst) {
    uint64_t index = hart->GPR[inst.rs2] & 0x3f;
    hart->GPR[inst.rd] = (hart->GPR[inst.rs1] >> index) & 1;
    return true;
}
inst_ret exec_BEXTI(HART *hart, inst_data& inst) {
    uint64_t index = inst.imm & 0x3f;
    hart->GPR[inst.rd] = (hart->GPR[inst.rs1] >> index) & 1;
    return true;
}

inst_ret exec_BINV(HART *hart, inst_data& inst) {
    uint64_t index = hart->GPR[inst.rs2] & 0x3f;
    hart->GPR[inst.rd] = hart->GPR[inst.rs1] ^ ((uint64_t)1 << index);
    return true;
}
inst_ret exec_BINVI(HART *hart, inst_data& inst) {
    uint64_t index = inst.imm & 0x3f;
    hart->GPR[inst.rd] = hart->GPR[inst.rs1] ^ ((uint64_t)1 << index);
    return true;
}

inst_ret exec_BSET(HART *hart, inst_data& inst) {
    uint64_t index = hart->GPR[inst.rs2] & 0x3f;
    hart->GPR[inst.rd] = hart->GPR[inst.rs1] | ((uint64_t)1 << index);
    return true;
}
inst_ret exec_BSETI(HART *hart, inst_data& inst) {
    uint64_t index = inst.imm & 0x3f;
    hart->GPR[inst.rd] = hart->GPR[inst.rs1] | ((uint64_t)1 << index);
    return true;
}