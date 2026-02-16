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

#ifdef USE_FPU

FPRValue custom_min(FPRValue t1, FPRValue t2) {
    if(isnan((float)t1) && isnan((float)t2)) {
        return std::bit_cast<float>(F_QNAN);
    }
    else if(isnan((float)t1)) {
        if(t1.is_signaling_nan) std::feraiseexcept(FE_INVALID);
        return t2;
    }
    else if(isnan((float)t2)) {
        if(t2.is_signaling_nan) std::feraiseexcept(FE_INVALID);
        return t1;
    }
    else {
        if(!std::signbit((float)t1.val) && std::signbit((float)t2.val)) {
            return t2;
        } else if(std::signbit((float)t1.val) && !std::signbit((float)t2.val)) {
            return t1;
        } else {
            return ((float)t1.val > (float)t2.val) ? t2 : t1;
        }
    }
}
FPRValue custom_max(FPRValue t1, FPRValue t2) {
    if(isnan((float)t1) && isnan((float)t2)) {
        return std::bit_cast<float>(F_QNAN);
    }
    else if(isnan((float)t1)) {
        if(t1.is_signaling_nan) std::feraiseexcept(FE_INVALID);
        return t2;
    }
    else if(isnan((float)t2)) {
        if(t2.is_signaling_nan) std::feraiseexcept(FE_INVALID);
        return t1;
    }
    else {
        if(!std::signbit((float)t1.val) && std::signbit((float)t2.val)) {
            return t1;
        } else if(std::signbit((float)t1.val) && !std::signbit((float)t2.val)) {
            return t2;
        } else {
            return ((float)t1.val < (float)t2.val) ? t2 : t1;
        }
    }
}

FPRValue accurate_round(FPRValue val, RoundingMode rm, RoundingMode frm) {
    switch(rm) {
        case RoundingMode::RNE: return std::nearbyintf(val.val);
        case RoundingMode::RTZ: return std::truncf(val.val);
        case RoundingMode::RDN: return std::floorf(val.val);
        case RoundingMode::RUP: return std::ceilf(val.val);
        case RoundingMode::RMM: {
            double i = std::floorf(val.val);
            double f = val.val - i;
            if (f < 0.5) return i;
            if (f > 0.5) return i + 1.0;
            return (val.val >= 0) ? (i + 1.0) : i;
        }
        case RoundingMode::DYN: return accurate_round(val,frm,frm);
    }
    return val;
}

FPRValue update_fflags(HART *hart, float res, inst_data inst) {
    FPRValue new_val = accurate_round(res,inst.rm,(RoundingMode)csr_read(hart,FRM));
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
    return new_val;
}
FPRValue update_fflags(HART *hart, float res) {
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

template <typename T>
T saturate_round(double f) {
    double rounded = std::round(f);

    double min_val = static_cast<double>(std::numeric_limits<T>::min());
    double max_val = static_cast<double>(std::numeric_limits<T>::max());

    if (rounded <= min_val || (isinf(f) && signbit(f))) return std::numeric_limits<T>::min();
    if (rounded >= max_val || isnan(f) || (isinf(f) && !signbit(f))) return std::numeric_limits<T>::max();
    
    return static_cast<T>(rounded);
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
    float res = ((float)hart->FPR[inst.rs1].val - (float)hart->FPR[inst.rs2]);
    if(isinff((float)hart->FPR[inst.rs1].val) && isinff((float)hart->FPR[inst.rs2].val)) {res = std::bit_cast<float>(F_QNAN); std::feraiseexcept(FE_INVALID);}
    hart->FPR[inst.rd] = update_fflags(hart,(double)res);
    return true;
}
inst_ret exec_FMUL_S(HART *hart, inst_data& inst) {
    std::feclearexcept(FE_ALL_EXCEPT);
    hart->FPR[inst.rd] = update_fflags(hart,(double)((float)hart->FPR[inst.rs1] * (float)hart->FPR[inst.rs2]));
    return true;
}
inst_ret exec_FDIV_S(HART *hart, inst_data& inst) {
    std::feclearexcept(FE_ALL_EXCEPT);
    float res = ((float)hart->FPR[inst.rs1].val / (float)hart->FPR[inst.rs2]);
    if(isinff((float)hart->FPR[inst.rs1].val) && isinff((float)hart->FPR[inst.rs2].val)) {res = std::bit_cast<float>(F_QNAN); std::feraiseexcept(FE_INVALID);}
    hart->FPR[inst.rd] = update_fflags(hart,(double)res);
    return true;
}

inst_ret exec_FSQRT_S(HART *hart, inst_data& inst) {
    std::feclearexcept(FE_ALL_EXCEPT);
    if(hart->FPR[inst.rs1].val < 0) {
        std::feraiseexcept(FE_INVALID);
        hart->FPR[inst.rd] = update_fflags(hart,std::bit_cast<float>(F_QNAN));
    }
    else hart->FPR[inst.rd] = update_fflags(hart,sqrtf((float)hart->FPR[inst.rs1]));
    return true;
}

inst_ret exec_FMIN_S(HART *hart, inst_data& inst) {
    std::feclearexcept(FE_ALL_EXCEPT);
    FPRValue res = custom_min(hart->FPR[inst.rs1],hart->FPR[inst.rs2]);
    hart->FPR[inst.rd] = update_fflags(hart,res);
    return true;
}
inst_ret exec_FMAX_S(HART *hart, inst_data& inst) {
    std::feclearexcept(FE_ALL_EXCEPT);
    hart->FPR[inst.rd] = update_fflags(hart,custom_max(hart->FPR[inst.rs1],hart->FPR[inst.rs2]));
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

inst_ret exec_FCVT_W_S(HART *hart, inst_data& inst) {
    std::feclearexcept(FE_ALL_EXCEPT);
    float rounded_val = (float)accurate_round(hart->FPR[inst.rs1],inst.rm,(RoundingMode)csr_read(hart,FRM));
    if(rounded_val != (float)hart->FPR[inst.rs1]) std::feraiseexcept(FE_INEXACT);

    if (std::isnan(hart->FPR[inst.rs1].val)) {
        std::feraiseexcept(FE_INVALID);
    }
    if (std::isinf(hart->FPR[inst.rs1].val)) {
        std::feraiseexcept(FE_INVALID);
    }
    if (rounded_val < -2147483648.0f || rounded_val > 2147483647.0f) {
        std::feraiseexcept(FE_INVALID);
    }
    
    hart->GPR[inst.rd] = (uint64_t)(uint64_t)saturate_round<int32_t>((float)update_fflags(hart,(float)hart->FPR[inst.rs1],inst));
    return true;
}
inst_ret exec_FCVT_WU_S(HART *hart, inst_data& inst) {
    std::feclearexcept(FE_ALL_EXCEPT);
    float rounded_val = (float)accurate_round(hart->FPR[inst.rs1],inst.rm,(RoundingMode)csr_read(hart,FRM));
    if(rounded_val != (float)hart->FPR[inst.rs1]) std::feraiseexcept(FE_INEXACT);

    if (std::isnan(hart->FPR[inst.rs1].val)) {
        std::feraiseexcept(FE_INVALID);
    }
    if (std::isinf(hart->FPR[inst.rs1].val)) {
        std::feraiseexcept(FE_INVALID);
    }
    if (rounded_val < 0.0f || rounded_val > 4294967295.0f) {
        std::feraiseexcept(FE_INVALID);
    }

    hart->GPR[inst.rd] = (uint64_t)(int32_t)saturate_round<uint32_t>((float)update_fflags(hart,(float)hart->FPR[inst.rs1],inst));
    return true;
}
inst_ret exec_FCVT_L_S(HART *hart, inst_data& inst) {
    std::feclearexcept(FE_ALL_EXCEPT);
    float rounded_val = (float)accurate_round(hart->FPR[inst.rs1],inst.rm,(RoundingMode)csr_read(hart,FRM));
    if(rounded_val != (float)hart->FPR[inst.rs1]) std::feraiseexcept(FE_INEXACT);

    if (std::isnan(hart->FPR[inst.rs1].val)) {
        std::feraiseexcept(FE_INVALID);
    }
    if (std::isinf(hart->FPR[inst.rs1].val)) {
        std::feraiseexcept(FE_INVALID);
    }
    if (rounded_val < -9223372036854775808.0 || rounded_val > 9223372036854775807.0) {
        std::feraiseexcept(FE_INVALID);
    }
    hart->GPR[inst.rd] = (uint64_t)saturate_round<int64_t>((float)update_fflags(hart,(float)hart->FPR[inst.rs1],inst));
    return true;
}
inst_ret exec_FCVT_LU_S(HART *hart, inst_data& inst) {
    std::feclearexcept(FE_ALL_EXCEPT);
    float rounded_val = (float)accurate_round(hart->FPR[inst.rs1],inst.rm,(RoundingMode)csr_read(hart,FRM));
    if(rounded_val != (float)hart->FPR[inst.rs1]) std::feraiseexcept(FE_INEXACT);

    if (std::isnan(hart->FPR[inst.rs1].val)) {
        std::feraiseexcept(FE_INVALID);
    }
    if (std::isinf(hart->FPR[inst.rs1].val)) {
        std::feraiseexcept(FE_INVALID);
    }
    if (rounded_val < 0.0) {
        std::feraiseexcept(FE_INVALID);
    }
    hart->GPR[inst.rd] = (uint64_t)saturate_round<uint64_t>(update_fflags(hart,(float)hart->FPR[inst.rs1],inst));
    return true;
}

inst_ret exec_FCVT_S_W(HART *hart, inst_data& inst) {
    std::feclearexcept(FE_ALL_EXCEPT);
    hart->FPR[inst.rd] = update_fflags(hart,(float)(int32_t)hart->GPR[inst.rs1],inst);
    return true;
}
inst_ret exec_FCVT_S_WU(HART *hart, inst_data& inst) {
    std::feclearexcept(FE_ALL_EXCEPT);
    hart->FPR[inst.rd] = update_fflags(hart,(float)(uint32_t)hart->GPR[inst.rs1],inst);
    return true;
}
inst_ret exec_FCVT_S_L(HART *hart, inst_data& inst) {
    std::feclearexcept(FE_ALL_EXCEPT);
    hart->FPR[inst.rd] = update_fflags(hart,(float)(int64_t)hart->GPR[inst.rs1],inst);
    return true;
}
inst_ret exec_FCVT_S_LU(HART *hart, inst_data& inst) {
    std::feclearexcept(FE_ALL_EXCEPT);
    hart->FPR[inst.rd] = update_fflags(hart,(float)(uint64_t)hart->GPR[inst.rs1],inst);
    return true;
}

inst_ret exec_FSGNJ_S(HART *hart, inst_data& inst) {
    std::feclearexcept(FE_ALL_EXCEPT);
    hart->FPR[inst.rd] = update_fflags(hart,(double)copysignf((float)hart->FPR[inst.rs1],(float)hart->FPR[inst.rs2]));
    return true;
}
inst_ret exec_FSGNJN_S(HART *hart, inst_data& inst) {
    std::feclearexcept(FE_ALL_EXCEPT);
    hart->FPR[inst.rd] = update_fflags(hart,(double)copysignf(hart->FPR[inst.rs1],-hart->FPR[inst.rs2].val));
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
    hart->GPR[inst.rd] = update_fflags(hart,hart->FPR[inst.rs1].val <= hart->FPR[inst.rs2].val);
    return true;
}
inst_ret exec_FLT_S(HART *hart, inst_data& inst) {
    hart->GPR[inst.rd] = update_fflags(hart,hart->FPR[inst.rs1].val < hart->FPR[inst.rs2].val);
    return true;
}
inst_ret exec_FEQ_S(HART *hart, inst_data& inst) {
    hart->GPR[inst.rd] = update_fflags(hart,hart->FPR[inst.rs1].val == hart->FPR[inst.rs2].val);
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
    if(hart->FPR[inst.rs1].is_signaling_nan) {
        // signaling nan
        out |= 0x100;
    }
    if(hart->FPR[inst.rs1].is_quiet_nan) {
        // quiet nan
        out |= 0x200;
    }

    hart->GPR[inst.rd] = out;
    return true;
}

#endif