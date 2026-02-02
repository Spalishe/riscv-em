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

#include "../../include/decode.h"
#include "../../include/cpu.hpp"

#include <limits>

// R-Type

bool exec_MULW(HART *hart, inst_data& inst) {
	hart->GPR[inst.rd] = (uint64_t)((int32_t)hart->GPR[inst.rs1] * (int32_t)hart->GPR[inst.rs2]);
    return true;
}

bool exec_DIVW(HART *hart, inst_data& inst) {
    if((int32_t)hart->GPR[inst.rs2] == 0) {
        hart->GPR[inst.rd] = (uint64_t)-1;
    } else if((int32_t)hart->GPR[inst.rs1] == 0) {
        hart->GPR[inst.rd] = (uint64_t)0;
    } else if((int32_t)hart->GPR[inst.rs1] == std::numeric_limits<int32_t>::min() && (int32_t)hart->GPR[inst.rs2] == -1) {
        hart->GPR[inst.rd] = (int32_t)hart->GPR[inst.rs1];
    } else {
        hart->GPR[inst.rd] = (uint64_t)((int32_t)hart->GPR[inst.rs1] / (int32_t)hart->GPR[inst.rs2]);
    }
    return true;
}
bool exec_DIVUW(HART *hart, inst_data& inst) {
    if((uint32_t)hart->GPR[inst.rs2] == 0) {
        hart->GPR[inst.rd] = (uint64_t)-1;
    } else if((uint32_t)hart->GPR[inst.rs1] == 0) {
        hart->GPR[inst.rd] = (uint64_t)0;
    } else {
        hart->GPR[inst.rd] = (uint64_t)(int32_t)((uint32_t)hart->GPR[inst.rs1] / (uint32_t)hart->GPR[inst.rs2]);
    }
    return true;
}
bool exec_REMW(HART *hart, inst_data& inst) {
    if((int32_t)hart->GPR[inst.rs2] == 0) {
        hart->GPR[inst.rd] = (uint64_t)(int32_t)hart->GPR[inst.rs1];
    } else if((int32_t)hart->GPR[inst.rs1] == std::numeric_limits<int32_t>::min() && (int32_t)hart->GPR[inst.rs2] == -1) {
        hart->GPR[inst.rd] = 0;
    } else {
        hart->GPR[inst.rd] = (uint64_t)(int64_t)((int32_t)hart->GPR[inst.rs1] % (int32_t)hart->GPR[inst.rs2]);
    }
    return true;
}
bool exec_REMUW(HART *hart, inst_data& inst) {
    if((uint32_t)hart->GPR[inst.rs2] == 0) {
        hart->GPR[inst.rd] = hart->GPR[inst.rs1];
    } else {
        hart->GPR[inst.rd] = (uint64_t)(int64_t)(int32_t)((uint32_t)hart->GPR[inst.rs1] % (uint32_t)hart->GPR[inst.rs2]);
    }
    return true;
}


bool exec_MUL(HART *hart, inst_data& inst) {
    hart->GPR[inst.rd] = hart->GPR[inst.rs1] * hart->GPR[inst.rs2];
    return true;
}
bool exec_MULH(HART *hart, inst_data& inst) {
    __int128_t res = (__int128_t)(int64_t)hart->GPR[inst.rs1] * (__int128_t)(int64_t)hart->GPR[inst.rs2];
    hart->GPR[inst.rd] = (uint64_t)(res>>64);
    return true;
}
bool exec_MULHSU(HART *hart, inst_data& inst) {
    __uint128_t res = (__uint128_t)(__int128_t)(int64_t)hart->GPR[inst.rs1] * (__uint128_t)hart->GPR[inst.rs2];
    hart->GPR[inst.rd] = (uint64_t)(res>>64);
    return true;
}
bool exec_MULHU(HART *hart, inst_data& inst) {
    __uint128_t res = (__uint128_t)hart->GPR[inst.rs1] * (__uint128_t)hart->GPR[inst.rs2];
    hart->GPR[inst.rd] = (uint64_t)(res>>64);
    return true;
}

bool exec_DIV(HART *hart, inst_data& inst) {
    if(hart->GPR[inst.rs2] == 0) {
        hart->GPR[inst.rd] = (uint64_t)-1;
    } else if(hart->GPR[inst.rs1] == 0) {
        hart->GPR[inst.rd] = (uint64_t)0;
    } else if(hart->GPR[inst.rs1] == std::numeric_limits<int64_t>::min() && hart->GPR[inst.rs2] == -1) {
        hart->GPR[inst.rd] = hart->GPR[inst.rs1];
    } else {
        hart->GPR[inst.rd] = (uint64_t)((int64_t)hart->GPR[inst.rs1] / (int64_t)hart->GPR[inst.rs2]);
    }
    return true;
}
bool exec_DIVU(HART *hart, inst_data& inst) {
    if(hart->GPR[inst.rs2] == 0) {
        hart->GPR[inst.rd] = (uint64_t)-1;
    } else if(hart->GPR[inst.rs1] == 0) {
        hart->GPR[inst.rd] = (uint64_t)0;
    } else {
        hart->GPR[inst.rd] = hart->GPR[inst.rs1] / hart->GPR[inst.rs2];
    }
    return true;
}
bool exec_REM(HART *hart, inst_data& inst) {
    if(hart->GPR[inst.rs2] == 0) {
        hart->GPR[inst.rd] = hart->GPR[inst.rs1];
    } else if(hart->GPR[inst.rs1] == std::numeric_limits<int64_t>::min() && hart->GPR[inst.rs2] == -1) {
        hart->GPR[inst.rd] = 0;
    } else {
        hart->GPR[inst.rd] = (uint64_t)((int64_t)hart->GPR[inst.rs1] % (int64_t)hart->GPR[inst.rs2]);
    }
    return true;
}
bool exec_REMU(HART *hart, inst_data& inst) {
    if(hart->GPR[inst.rs2] == 0) {
        hart->GPR[inst.rd] = hart->GPR[inst.rs1];
    } else {
        hart->GPR[inst.rd] = hart->GPR[inst.rs1] % (int64_t)hart->GPR[inst.rs2];
    }
    return true;
}