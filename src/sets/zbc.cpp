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

bool exec_CLMUL(HART *hart, inst_data& inst) {
    uint64_t rs1 = hart->GPR[inst.rs1];
    uint64_t rs2 = hart->GPR[inst.rs2];
    uint64_t out = 0;

    for(int i = 0; i < 64; i++) {
        out = ((rs2 >> i) & 1) ? (out ^ (rs1 << i)) : out;
    }

    hart->GPR[inst.rd] = out;
    return true;
}
bool exec_CLMULH(HART *hart, inst_data& inst) {
    uint64_t rs1 = hart->GPR[inst.rs1];
    uint64_t rs2 = hart->GPR[inst.rs2];
    uint64_t out = 0;

    for(int i = 1; i < 64; i++) {
        out = ((rs2 >> i) & 1) ? (out ^ (rs1 >> (64-i))) : out;
    }

    hart->GPR[inst.rd] = out;
    return true;
}
bool exec_CLMULR(HART *hart, inst_data& inst) {
    uint64_t rs1 = hart->GPR[inst.rs1];
    uint64_t rs2 = hart->GPR[inst.rs2];
    uint64_t out = 0;

    for(int i = 1; i < 63; i++) {
        out = ((rs2 >> i) & 1) ? (out ^ (rs1 >> (64-i-1))) : out;
    }

    hart->GPR[inst.rd] = out;
    return true;
}