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


void amo_check_reservation(Machine& cpu, uint64_t va) {
    for(auto& h : cpu.harts) {
        if(h->reservation.valid && h->reservation.vaddr == va) {
            h->reservation.valid = false;
        }
    }
}

std::optional<int> AMO_SC(HART* hart, uint64_t va, uint8_t size, uint64_t val) {
    if(hart->reservation.valid && hart->reservation.vaddr == va && hart->reservation.size == size) {
        bool out = hart->mmio->store(hart,va,size,val);
        if(!out) return std::nullopt;
        amo_check_reservation(*hart->mmio->mmu->cpu, va);
        hart->reservation.valid = false;
        return 0;
    } else {
        hart->reservation.valid = false;
        return 1;
    }
}
std::optional<uint64_t> AMO_LR(HART* hart, uint64_t va, uint64_t size) {
    std::optional<uint64_t> p = hart->mmio->load(hart,va,size);
    if(!p.has_value()) return std::nullopt;
    hart->reservation.valid = true;
    hart->reservation.size = size;
    hart->reservation.vaddr = va;
    return *p;
}

inst_ret exec_LR_D(HART *hart, inst_data& inst) {
    std::optional<uint64_t> val = AMO_LR(hart, hart->GPR[inst.rs1],64);
    if(!val.has_value()) return false;
    hart->GPR[inst.rd] = *val;
    return true;
}
inst_ret exec_SC_D(HART *hart, inst_data& inst) {
    std::optional<int> val = AMO_SC(hart, hart->GPR[inst.rs1],64,hart->GPR[inst.rs2]);
    if(!val.has_value()) return false;
    hart->GPR[inst.rd] = *val;
    return true;
}

std::optional<uint64_t> AMO64(HART *hart, uint64_t va, uint64_t rs2, uint64_t (*func)(uint64_t a,uint64_t b)) {
    auto val = hart->mmio->load(hart, va, 64);
    if(!val.has_value()) return std::nullopt;
    uint64_t t = *val;
    uint64_t new_val = func(t, rs2);

    bool s = hart->mmio->store(hart, va, 64, new_val);
    if(!s) return std::nullopt;
    return t;
}

inst_ret exec_AMOSWAP_D(HART *hart, inst_data& inst) {
    auto val = AMO64(hart,hart->GPR[inst.rs1],hart->GPR[inst.rs2],[](uint64_t a, uint64_t b) {return b;});
    if(val.has_value()) hart->GPR[inst.rd] = *val;
    return val.has_value();
}
inst_ret exec_AMOADD_D(HART *hart, inst_data& inst) {
    auto val = AMO64(hart,hart->GPR[inst.rs1],hart->GPR[inst.rs2],[](uint64_t a, uint64_t b) {return a+b;});
    if(val.has_value()) hart->GPR[inst.rd] = *val;
    return val.has_value();
}
inst_ret exec_AMOXOR_D(HART *hart, inst_data& inst) {
    auto val = AMO64(hart,hart->GPR[inst.rs1],hart->GPR[inst.rs2],[](uint64_t a, uint64_t b) {return a^b;});
    if(val.has_value()) hart->GPR[inst.rd] = *val;
    return val.has_value();
}
inst_ret exec_AMOAND_D(HART *hart, inst_data& inst) {
    auto val = AMO64(hart,hart->GPR[inst.rs1],hart->GPR[inst.rs2],[](uint64_t a, uint64_t b) {return a&b;});
    if(val.has_value()) hart->GPR[inst.rd] = *val;
    return val.has_value();
}
inst_ret exec_AMOOR_D(HART *hart, inst_data& inst) {
    auto val = AMO64(hart,hart->GPR[inst.rs1],hart->GPR[inst.rs2],[](uint64_t a, uint64_t b) {return a|b;});
    if(val.has_value()) hart->GPR[inst.rd] = *val;
    return val.has_value();
}
inst_ret exec_AMOMIN_D(HART *hart, inst_data& inst) {
    auto val = AMO64(hart,hart->GPR[inst.rs1],hart->GPR[inst.rs2],[](uint64_t a, uint64_t b) {return (uint64_t)std::min((int64_t)a,(int64_t)b);});
    if(val.has_value()) hart->GPR[inst.rd] = *val;
    return val.has_value();
}
inst_ret exec_AMOMAX_D(HART *hart, inst_data& inst) {
    auto val = AMO64(hart,hart->GPR[inst.rs1],hart->GPR[inst.rs2],[](uint64_t a, uint64_t b) {return (uint64_t)std::max((int64_t)a,(int64_t)b);});
    if(val.has_value()) hart->GPR[inst.rd] = *val;
    return val.has_value();
}
inst_ret exec_AMOMINU_D(HART *hart, inst_data& inst) {
    auto val = AMO64(hart,hart->GPR[inst.rs1],hart->GPR[inst.rs2],[](uint64_t a, uint64_t b) {return std::min(a,b);});
    if(val.has_value()) hart->GPR[inst.rd] = *val;
    return val.has_value();
}
inst_ret exec_AMOMAXU_D(HART *hart, inst_data& inst) {
    auto val = AMO64(hart,hart->GPR[inst.rs1],hart->GPR[inst.rs2],[](uint64_t a, uint64_t b) {return std::max(a,b);});
    if(val.has_value()) hart->GPR[inst.rd] = *val;
    return val.has_value();
}

// RV32A

inst_ret exec_LR_W(HART *hart, inst_data& inst) {
    std::optional<uint64_t> val = AMO_LR(hart, hart->GPR[inst.rs1],32);
    if(!val.has_value()) return false;
    hart->GPR[inst.rd] = *val;
    return true;
}
inst_ret exec_SC_W(HART *hart, inst_data& inst) {
    std::optional<int> val = AMO_SC(hart, hart->GPR[inst.rs1],32,(uint32_t)hart->GPR[inst.rs2]);
    if(!val.has_value()) return false;
    hart->GPR[inst.rd] = *val;
    return true;
}

std::optional<uint32_t> AMO32(HART *hart, uint64_t va, uint32_t rs2, uint32_t (*func)(uint32_t a,uint32_t b)) {
    auto val = hart->mmio->load(hart, va, 32);
    if(!val.has_value()) return std::nullopt;
    uint32_t t = *val;
    uint32_t new_val = func(t, rs2);

    bool s = hart->mmio->store(hart, va, 32, new_val);
    if(!s) return std::nullopt;
    return t;
}

inst_ret exec_AMOSWAP_W(HART *hart, inst_data& inst) {
    auto val = AMO32(hart,hart->GPR[inst.rs1],(uint32_t)hart->GPR[inst.rs2],[](uint32_t a, uint32_t b) {return b;});
    if(val.has_value()) hart->GPR[inst.rd] = (int64_t)(int32_t)*val;
    return val.has_value();
}
inst_ret exec_AMOADD_W(HART *hart, inst_data& inst) {
    auto val = AMO32(hart,hart->GPR[inst.rs1],(uint32_t)hart->GPR[inst.rs2],[](uint32_t a, uint32_t b) {return a+b;});
    if(val.has_value()) hart->GPR[inst.rd] = (int64_t)(int32_t)*val;
    return val.has_value();
}
inst_ret exec_AMOXOR_W(HART *hart, inst_data& inst) {
    auto val = AMO32(hart,hart->GPR[inst.rs1],(uint32_t)hart->GPR[inst.rs2],[](uint32_t a, uint32_t b) {return a^b;});
    if(val.has_value()) hart->GPR[inst.rd] = (int64_t)(int32_t)*val;
    return val.has_value();
}
inst_ret exec_AMOAND_W(HART *hart, inst_data& inst) {
    auto val = AMO32(hart,hart->GPR[inst.rs1],(uint32_t)hart->GPR[inst.rs2],[](uint32_t a, uint32_t b) {return a&b;});
    if(val.has_value()) hart->GPR[inst.rd] = (int64_t)(int32_t)*val;
    return val.has_value();
}
inst_ret exec_AMOOR_W(HART *hart, inst_data& inst) {
    auto val = AMO32(hart,hart->GPR[inst.rs1],(uint32_t)hart->GPR[inst.rs2],[](uint32_t a, uint32_t b) {return a|b;});
    if(val.has_value()) hart->GPR[inst.rd] = (int64_t)(int32_t)*val;
    return val.has_value();
}
inst_ret exec_AMOMIN_W(HART *hart, inst_data& inst) {
    auto val = AMO32(hart,hart->GPR[inst.rs1],(uint32_t)hart->GPR[inst.rs2],[](uint32_t a, uint32_t b) {return (uint32_t)std::min((int32_t)a,(int32_t)b);});
    if(val.has_value()) hart->GPR[inst.rd] = (int64_t)(int32_t)*val;
    return val.has_value();
}
inst_ret exec_AMOMAX_W(HART *hart, inst_data& inst) {
    auto val = AMO32(hart,hart->GPR[inst.rs1],(uint32_t)hart->GPR[inst.rs2],[](uint32_t a, uint32_t b) {return (uint32_t)std::max((int32_t)a,(int32_t)b);});
    if(val.has_value()) hart->GPR[inst.rd] = (int64_t)(int32_t)*val;
    return val.has_value();
}
inst_ret exec_AMOMINU_W(HART *hart, inst_data& inst) {
    auto val = AMO32(hart,hart->GPR[inst.rs1],(uint32_t)hart->GPR[inst.rs2],[](uint32_t a, uint32_t b) {return std::min(a,b);});
    if(val.has_value()) hart->GPR[inst.rd] = (int64_t)(int32_t)*val;
    return val.has_value();
}
inst_ret exec_AMOMAXU_W(HART *hart, inst_data& inst) {
    auto val = AMO32(hart,hart->GPR[inst.rs1],(uint32_t)hart->GPR[inst.rs2],[](uint32_t a, uint32_t b) {return std::max(a,b);});
    if(val.has_value()) hart->GPR[inst.rd] = (int64_t)(int32_t)*val;
    return val.has_value();
}