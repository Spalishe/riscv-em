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
#include <bit>
#include <cmath>
#include <cfenv>

bool is_nan(double x) {
    return std::isnan(x);
}

bool is_qnan(float d) {
    if(!is_nan(d)) return false;

    uint32_t bits = std::bit_cast<uint32_t>(d);

    return (bits >= 0x7FC00000 && bits <= 0x7FFFFFFF) || (bits >= 0xFFC00000 && bits <= 0xFFFFFFFF);
}

bool is_snan(float d) {
    if(!is_nan(d)) return false;

    uint32_t bits = std::bit_cast<uint32_t>(d);

    return (bits >= 0x7F800001 && bits <= 0x7FBFFFFF) || (bits >= 0xFF800001 && bits <= 0xFFBFFFFF);
}

float update_fflags(HART *hart, float res) {
    uint32_t fflags = 0;
    if(std::fetestexcept(FE_INEXACT)) {
        fflags |= 0x1;
    }
    if(std::fetestexcept(FE_UNDERFLOW)) {
        fflags |= 0x2;
    }
    if(std::fetestexcept(FE_OVERFLOW)) {
        fflags |= 0x4;
    }
    if(std::fetestexcept(FE_DIVBYZERO)) {
        fflags |= 0x8;
    }
    if(std::fetestexcept(FE_INVALID)) {
        fflags |= 0x10;
    }
    csr_write(hart,FFLAGS,fflags);
    return res;
}

inst_ret exec_FLW(HART *hart, inst_data& inst) {
    uint64_t addr = hart->GPR[inst.rs1] + (int64_t) inst.imm;
    std::optional<uint64_t> val = hart->mmio->load(hart,addr,32);
    if(val.has_value()) {
        hart->FPR[inst.rd] = std::bit_cast<float>((uint32_t) *val);
    }
    return val.has_value();
}
inst_ret exec_FSW(HART *hart, inst_data& inst) {
    uint64_t addr = hart->GPR[inst.rs1] + (int64_t) inst.imm;
    return hart->mmio->store(hart,addr,32,std::bit_cast<uint32_t>((float)hart->FPR[inst.rs2]));
}

inst_ret exec_FADD_S(HART *hart, inst_data& inst) {
    std::feclearexcept(FE_ALL_EXCEPT);
    hart->FPR[inst.rd] = update_fflags(hart,(double)((float)hart->FPR[inst.rs1] + (float)hart->FPR[inst.rs2]));
    return true;
}
inst_ret exec_FSUB_S(HART *hart, inst_data& inst) {
    std::feclearexcept(FE_ALL_EXCEPT);
    hart->FPR[inst.rd] = update_fflags(hart,(double)((float)hart->FPR[inst.rs1] - (float)hart->FPR[inst.rs2]));
    return true;
}
inst_ret exec_FMUL_S(HART *hart, inst_data& inst) {
    std::feclearexcept(FE_ALL_EXCEPT);
    hart->FPR[inst.rd] = update_fflags(hart,(double)((float)hart->FPR[inst.rs1] * (float)hart->FPR[inst.rs2]));
    return true;
}
inst_ret exec_FDIV_S(HART *hart, inst_data& inst) {
    std::feclearexcept(FE_ALL_EXCEPT);
    hart->FPR[inst.rd] = update_fflags(hart,(double)((float)hart->FPR[inst.rs1] / (float)hart->FPR[inst.rs2]));
    return true;
}

inst_ret exec_FSQRT_S(HART *hart, inst_data& inst) {
    std::feclearexcept(FE_ALL_EXCEPT);
    if(hart->FPR[inst.rs1] < 0) {
        std::feraiseexcept(FE_INVALID);
        hart->FPR[inst.rd] = update_fflags(hart,std::bit_cast<float>(F_QNAN));
    }
    else hart->FPR[inst.rd] = update_fflags(hart,sqrtf((float)hart->FPR[inst.rs1]));
    return true;
}

// FIXME: Those funcs dont set right invalid exception(fix signalled nan detection)
inst_ret exec_FMIN_S(HART *hart, inst_data& inst) {
    std::feclearexcept(FE_ALL_EXCEPT);
    if(isnan((float)hart->FPR[inst.rs1])) {
        if(is_snan((float)hart->FPR[inst.rs1])) std::feraiseexcept(FE_INVALID);
        hart->FPR[inst.rd] = update_fflags(hart,(float)hart->FPR[inst.rs2]);
    }
    else if(isnan((float)hart->FPR[inst.rs2])) {
        if(is_snan((float)hart->FPR[inst.rs2])) std::feraiseexcept(FE_INVALID);
        hart->FPR[inst.rd] = update_fflags(hart,(float)hart->FPR[inst.rs1]);
    }
    else hart->FPR[inst.rd] = update_fflags(hart,(double)std::min((float)hart->FPR[inst.rs1],(float)hart->FPR[inst.rs2]));
    return true;
}
inst_ret exec_FMAX_S(HART *hart, inst_data& inst) {
    std::feclearexcept(FE_ALL_EXCEPT);
    if(isnan((float)hart->FPR[inst.rs1])) {
        if(is_snan((float)hart->FPR[inst.rs1])) std::feraiseexcept(FE_INVALID);
        hart->FPR[inst.rd] = update_fflags(hart,(float)hart->FPR[inst.rs2]);
    }
    else if(isnan((float)hart->FPR[inst.rs2])) {
        if(is_snan((float)hart->FPR[inst.rs2])) std::feraiseexcept(FE_INVALID);
        hart->FPR[inst.rd] = update_fflags(hart,(float)hart->FPR[inst.rs1]);
    }
    else hart->FPR[inst.rd] = update_fflags(hart,(double)std::max((float)hart->FPR[inst.rs1],(float)hart->FPR[inst.rs2]));
    return true;
}

inst_ret exec_FMADD_S(HART *hart, inst_data& inst) {
    std::feclearexcept(FE_ALL_EXCEPT);
    hart->FPR[inst.rd] = update_fflags(hart,(double)(((float)hart->FPR[inst.rs1] * (float)hart->FPR[inst.rs2]) + (float)hart->FPR[inst.rs3]));
    return true;
}
inst_ret exec_FMSUB_S(HART *hart, inst_data& inst) {
    std::feclearexcept(FE_ALL_EXCEPT);
    hart->FPR[inst.rd] = update_fflags(hart,(double)(((float)hart->FPR[inst.rs1] * (float)hart->FPR[inst.rs2]) - (float)hart->FPR[inst.rs3]));
    return true;
}

inst_ret exec_FNMADD_S(HART *hart, inst_data& inst) {
    std::feclearexcept(FE_ALL_EXCEPT);
    hart->FPR[inst.rd] = update_fflags(hart,(double)(-((float)hart->FPR[inst.rs1] * (float)hart->FPR[inst.rs2]) - (float)hart->FPR[inst.rs3]));
    return true;
}
inst_ret exec_FNMSUB_S(HART *hart, inst_data& inst) {
    std::feclearexcept(FE_ALL_EXCEPT);
    hart->FPR[inst.rd] = update_fflags(hart,(double)(-((float)hart->FPR[inst.rs1] * (float)hart->FPR[inst.rs2]) + (float)hart->FPR[inst.rs3]));
    return true;
}

//FIXME: those instructions sets incorrect fflags
inst_ret exec_FCVT_W_S(HART *hart, inst_data& inst) {
    std::feclearexcept(FE_ALL_EXCEPT);
    hart->GPR[inst.rd] = update_fflags(hart,(uint64_t)(int32_t)round((float)hart->FPR[inst.rs1]));
    return true;
}
inst_ret exec_FCVT_WU_S(HART *hart, inst_data& inst) {
    std::feclearexcept(FE_ALL_EXCEPT);
    hart->GPR[inst.rd] = update_fflags(hart,(uint64_t)(int32_t)(uint32_t)round((float)hart->FPR[inst.rs1]));
    return true;
}
inst_ret exec_FCVT_L_S(HART *hart, inst_data& inst) {
    std::feclearexcept(FE_ALL_EXCEPT);
    hart->GPR[inst.rd] = update_fflags(hart,(uint64_t)(int64_t)round((float)hart->FPR[inst.rs1]));
    return true;
}
inst_ret exec_FCVT_LU_S(HART *hart, inst_data& inst) {
    std::feclearexcept(FE_ALL_EXCEPT);
    hart->GPR[inst.rd] = update_fflags(hart,(uint64_t)round((float)hart->FPR[inst.rs1]));
    return true;
}

inst_ret exec_FCVT_S_W(HART *hart, inst_data& inst) {
    std::feclearexcept(FE_ALL_EXCEPT);
    hart->FPR[inst.rd] = update_fflags(hart,(double)(float)(int32_t)hart->GPR[inst.rs1]);
    return true;
}
inst_ret exec_FCVT_S_WU(HART *hart, inst_data& inst) {
    std::feclearexcept(FE_ALL_EXCEPT);
    hart->FPR[inst.rd] = update_fflags(hart,(double)(float)(uint32_t)hart->GPR[inst.rs1]);
    return true;
}
inst_ret exec_FCVT_S_L(HART *hart, inst_data& inst) {
    std::feclearexcept(FE_ALL_EXCEPT);
    hart->FPR[inst.rd] = update_fflags(hart,(double)(float)(int64_t)hart->GPR[inst.rs1]);
    return true;
}
inst_ret exec_FCVT_S_LU(HART *hart, inst_data& inst) {
    std::feclearexcept(FE_ALL_EXCEPT);
    hart->FPR[inst.rd] = update_fflags(hart,(double)(float)(uint64_t)hart->GPR[inst.rs1]);
    return true;
}

inst_ret exec_FSGNJ_S(HART *hart, inst_data& inst) {
    std::feclearexcept(FE_ALL_EXCEPT);
    hart->FPR[inst.rd] = update_fflags(hart,(double)copysignf(hart->FPR[inst.rs1],hart->FPR[inst.rs2]));
    return true;
}
inst_ret exec_FSGNJN_S(HART *hart, inst_data& inst) {
    std::feclearexcept(FE_ALL_EXCEPT);
    hart->FPR[inst.rd] = update_fflags(hart,(double)copysignf(hart->FPR[inst.rs1],-hart->FPR[inst.rs2]));
    return true;
}
inst_ret exec_FSGNJX_S(HART *hart, inst_data& inst) {
    std::feclearexcept(FE_ALL_EXCEPT);
    uint32_t sign1 = std::bit_cast<uint32_t>((float)hart->FPR[inst.rs1]) & 0x80000000;
    uint32_t sign2 = std::bit_cast<uint32_t>((float)hart->FPR[inst.rs2]) & 0x80000000;
    uint32_t other = std::bit_cast<uint32_t>((float)hart->FPR[inst.rs1]) & 0x7fffffff;
    hart->FPR[inst.rd] = update_fflags(hart,(double)std::bit_cast<float>(((sign1 ^ sign2) | other)));
    return true;
}

inst_ret exec_FMV_X_W(HART *hart, inst_data& inst) {
    std::feclearexcept(FE_ALL_EXCEPT);
    hart->GPR[inst.rd] = std::bit_cast<uint64_t>((int64_t)(int32_t)(std::bit_cast<uint32_t>((float)hart->FPR[inst.rs1])));
    return true;
}
inst_ret exec_FMV_W_X(HART *hart, inst_data& inst) {
    std::feclearexcept(FE_ALL_EXCEPT);
    hart->FPR[inst.rd] = update_fflags(hart,std::bit_cast<float>((uint32_t)hart->GPR[inst.rs1]));
    return true;
}

inst_ret exec_FLE_S(HART *hart, inst_data& inst) {
    hart->GPR[inst.rd] = update_fflags(hart,hart->FPR[inst.rs1] <= hart->FPR[inst.rs2]);
    return true;
}
inst_ret exec_FLT_S(HART *hart, inst_data& inst) {
    hart->GPR[inst.rd] = update_fflags(hart,hart->FPR[inst.rs1] < hart->FPR[inst.rs2]);
    return true;
}
inst_ret exec_FEQ_S(HART *hart, inst_data& inst) {
    hart->GPR[inst.rd] = update_fflags(hart,hart->FPR[inst.rs1] == hart->FPR[inst.rs2]);
    return true;
}

inst_ret exec_FCLASS_S(HART *hart, inst_data& inst) {
    float f = (float)hart->FPR[inst.rs1];
    uint64_t out = 0;

    int c = fpclassify(f);
    bool neg = std::signbit(f);

    if(c == FP_INFINITE && neg) {
        // -inf
        out |= 0x1;
    }
    if(c == FP_NORMAL && neg) {
        // negative normal
        out |= 0x2;
    }
    if(c == FP_SUBNORMAL && neg) {
        // negative subnormal
        out |= 0x4;
    }
    if(c == FP_ZERO && neg) {
        // negative zero
        out |= 0x8;
    }
    if(c == FP_ZERO && !neg) {
        // positive zero
        out |= 0x10;
    }
    if(c == FP_SUBNORMAL && !neg) {
        // positive subnormal
        out |= 0x20;
    }
    if(c == FP_NORMAL && !neg) {
        // positive normal
        out |= 0x40;
    }
    if(c == FP_INFINITE && !neg) {
        // +inf
        out |= 0x80;
    }
    // FIXME: If you write SNAN to FPR, then it automatically changes to QNAN
    if(is_snan(f)) {
        // signaling nan
        out |= 0x100;
    }
    if(is_qnan(f)) {
        // quiet nan
        out |= 0x200;
    }

    hart->GPR[inst.rd] = out;
    return true;
}