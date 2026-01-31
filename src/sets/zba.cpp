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

bool exec_ADD_UW(HART *hart, inst_data& inst) {
	hart->GPR[inst.rd] = hart->GPR[inst.rs2] + (uint32_t)hart->GPR[inst.rs1];
    return true;
}
bool exec_SH1ADD(HART *hart, inst_data& inst) {
	hart->GPR[inst.rd] = hart->GPR[inst.rs2] + (hart->GPR[inst.rs1] << 1);
    return true;
}
bool exec_SH1ADD_UW(HART *hart, inst_data& inst) {
	hart->GPR[inst.rd] = hart->GPR[inst.rs2] + ((uint64_t)(uint32_t)hart->GPR[inst.rs1] << 1);
    return true;
}
bool exec_SH2ADD(HART *hart, inst_data& inst) {
	hart->GPR[inst.rd] = hart->GPR[inst.rs2] + (hart->GPR[inst.rs1] << 2);
    return true;
}
bool exec_SH2ADD_UW(HART *hart, inst_data& inst) {
	hart->GPR[inst.rd] = hart->GPR[inst.rs2] + ((uint64_t)(uint32_t)hart->GPR[inst.rs1] << 2);
    return true;
}
bool exec_SH3ADD(HART *hart, inst_data& inst) {
	hart->GPR[inst.rd] = hart->GPR[inst.rs2] + (hart->GPR[inst.rs1] << 3);
    return true;
}
bool exec_SH3ADD_UW(HART *hart, inst_data& inst) {
	hart->GPR[inst.rd] = hart->GPR[inst.rs2] + ((uint64_t)(uint32_t)hart->GPR[inst.rs1] << 3);
    return true;
}
bool exec_SLLI_UW(HART *hart, inst_data& inst) {
	hart->GPR[inst.rd] = ((uint64_t)(uint32_t)hart->GPR[inst.rs1]) << inst.imm;
    return true;
}