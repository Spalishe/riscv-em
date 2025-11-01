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
#include "../include/csr.h"
#include <iostream>

void exec_LR_D(struct HART *hart, uint32_t inst, CACHE_DecodedOperands* opers) {
    uint64_t rd_l = opers->s ? opers->rd : rd(inst);
    uint64_t rs1_l = opers->s ? opers->rs1 : rs1(inst);

    uint64_t addr = hart->regs[rs1_l];

    if(addr % 8 != 0) {
        hart->cpu_trap(EXC_LOAD_ADDR_MISALIGNED,addr,false);
    } else {
        std::optional<uint64_t> val = hart->mmio->load(hart,addr,64);
        if(val.has_value()) {
            hart->regs[rd_l] = *val;
            hart->reservation_addr = addr;
            hart->reservation_size = 64;
            hart->reservation_valid = true;
            hart->reservation_value = *val;
        }
    }
        
    if(!opers->s){
        opers->rd = rd_l;
        opers->rs1 = rs1_l;
        opers->s = true;
    }
    if(hart->dbg) hart->print_d("{0x%.8X} [LR.D] rd: %d; rs1: %d",hart->pc,rd_l,rs1_l);
}
void exec_SC_D(struct HART *hart, uint32_t inst, CACHE_DecodedOperands* opers) {
    uint64_t rd_l = opers->s ? opers->rd : rd(inst);
    uint64_t rs1_l = opers->s ? opers->rs1 : rs1(inst);
    uint64_t rs2_l = opers->s ? opers->rs2 : rs2(inst);

    uint64_t addr = hart->regs[rs1_l];

    if(hart->reservation_valid && hart->reservation_size == 64 && hart->reservation_addr == addr) {
        hart->mmio->store(hart,addr, 64, hart->regs[rs2_l]);
        hart->regs[rd_l] = 0;
    } else {
        hart->regs[rd_l] = 1;
    }
    hart->reservation_valid = false;
	
    if(!opers->s){
        opers->rd = rd_l;
        opers->rs1 = rs1_l;
        opers->rs2 = rs2_l;
        opers->s = true;
    }
    if(hart->dbg) hart->print_d("{0x%.8X} [SC.D] rd: %d; rs1: %d; rs2: %d",hart->pc,rd_l,rs1_l,rs2_l);
}

template<typename Functor>
std::optional<uint64_t> AMO(HART* hart, uint64_t addr, uint64_t rs2, Functor op) {
    if(addr % 8 != 0) {
        hart->cpu_trap(EXC_STORE_ADDR_MISALIGNED, addr, false);
        return std::nullopt;
    }

    auto val = hart->mmio->load(hart, addr, 64);
    if(!val.has_value()) {
        hart->cpu_trap(EXC_LOAD_ACCESS_FAULT, addr, false);
        return std::nullopt;
    }

    uint64_t t = *val;

    uint64_t new_val = op(t, rs2);

    hart->mmio->store(hart, addr, 64, new_val);

    return t;
}

void exec_AMOSWAP_D(struct HART *hart, uint32_t inst, CACHE_DecodedOperands* opers) {
    uint64_t rd_l = opers->s ? opers->rd : rd(inst);
    uint64_t rs1_l = opers->s ? opers->rs1 : rs1(inst);
    uint64_t rs2_l = opers->s ? opers->rs2 : rs2(inst);

    uint64_t addr = hart->regs[rs1_l];
    uint64_t rs2_val = hart->regs[rs2_l];

    std::optional<uint64_t> val = AMO(hart, addr, rs2_val, [](uint64_t a, uint64_t b){ return b; }); 
    if(val.has_value()) {
        hart->regs[rd_l] = *val;
    }
	
    if(!opers->s){
        opers->rd = rd_l;
        opers->rs1 = rs1_l;
        opers->rs2 = rs2_l;
        opers->s = true;
    }
    if(hart->dbg) hart->print_d("{0x%.8X} [AMOSWAP.D] rd: %d; rs1: %d; rs2: %d",hart->pc,rd_l,rs1_l,rs2_l);
}
void exec_AMOADD_D(struct HART *hart, uint32_t inst, CACHE_DecodedOperands* opers) {
    uint64_t rd_l = opers->s ? opers->rd : rd(inst);
    uint64_t rs1_l = opers->s ? opers->rs1 : rs1(inst);
    uint64_t rs2_l = opers->s ? opers->rs2 : rs2(inst);

    uint64_t addr = hart->regs[rs1_l];
    uint64_t rs2_val = hart->regs[rs2_l];

    std::optional<uint64_t> val = AMO(hart, addr, rs2_val, [](uint64_t a, uint64_t b){ return a+b; }); 
    if(val.has_value()) {
        hart->regs[rd_l] = *val;
    }
	
    if(!opers->s){
        opers->rd = rd_l;
        opers->rs1 = rs1_l;
        opers->rs2 = rs2_l;
        opers->s = true;
    }
    if(hart->dbg) hart->print_d("{0x%.8X} [AMOADD.D] rd: %d; rs1: %d; rs2: %d",hart->pc,rd_l,rs1_l,rs2_l);
}
void exec_AMOXOR_D(struct HART *hart, uint32_t inst, CACHE_DecodedOperands* opers) {
    uint64_t rd_l = opers->s ? opers->rd : rd(inst);
    uint64_t rs1_l = opers->s ? opers->rs1 : rs1(inst);
    uint64_t rs2_l = opers->s ? opers->rs2 : rs2(inst);

    uint64_t addr = hart->regs[rs1_l];
    uint64_t rs2_val = hart->regs[rs2_l];

    std::optional<uint64_t> val = AMO(hart, addr, rs2_val, [](uint64_t a, uint64_t b){ return a^b; }); 
    if(val.has_value()) {
        hart->regs[rd_l] = *val;
    }
	
    if(!opers->s){
        opers->rd = rd_l;
        opers->rs1 = rs1_l;
        opers->rs2 = rs2_l;
        opers->s = true;
    }
    if(hart->dbg) hart->print_d("{0x%.8X} [AMOXOR.D] rd: %d; rs1: %d; rs2: %d",hart->pc,rd_l,rs1_l,rs2_l);
}
void exec_AMOAND_D(struct HART *hart, uint32_t inst, CACHE_DecodedOperands* opers) {
    uint64_t rd_l = opers->s ? opers->rd : rd(inst);
    uint64_t rs1_l = opers->s ? opers->rs1 : rs1(inst);
    uint64_t rs2_l = opers->s ? opers->rs2 : rs2(inst);

    uint64_t addr = hart->regs[rs1_l];
    uint64_t rs2_val = hart->regs[rs2_l];

    std::optional<uint64_t> val = AMO(hart, addr, rs2_val, [](uint64_t a, uint64_t b){ return a&b; }); 
    if(val.has_value()) {
        hart->regs[rd_l] = *val;
    }

    if(!opers->s){
        opers->rd = rd_l;
        opers->rs1 = rs1_l;
        opers->rs2 = rs2_l;
        opers->s = true;
    }
    if(hart->dbg) hart->print_d("{0x%.8X} [AMOAND.D] rd: %d; rs1: %d; rs2: %d",hart->pc,rd_l,rs1_l,rs2_l);
}
void exec_AMOOR_D(struct HART *hart, uint32_t inst, CACHE_DecodedOperands* opers) {
    uint64_t rd_l = opers->s ? opers->rd : rd(inst);
    uint64_t rs1_l = opers->s ? opers->rs1 : rs1(inst);
    uint64_t rs2_l = opers->s ? opers->rs2 : rs2(inst);

    uint64_t addr = hart->regs[rs1_l];
    uint64_t rs2_val = hart->regs[rs2_l];

    std::optional<uint64_t> val = AMO(hart, addr, rs2_val, [](uint64_t a, uint64_t b){ return a|b; }); 
    if(val.has_value()) {
        hart->regs[rd_l] = *val;
    }
	
    if(!opers->s){
        opers->rd = rd_l;
        opers->rs1 = rs1_l;
        opers->rs2 = rs2_l;
        opers->s = true;
    }
    if(hart->dbg) hart->print_d("{0x%.8X} [AMOOR.D] rd: %d; rs1: %d; rs2: %d",hart->pc,rd_l,rs1_l,rs2_l);
}
void exec_AMOMIN_D(struct HART *hart, uint32_t inst, CACHE_DecodedOperands* opers) {
    uint64_t rd_l = opers->s ? opers->rd : rd(inst);
    uint64_t rs1_l = opers->s ? opers->rs1 : rs1(inst);
    uint64_t rs2_l = opers->s ? opers->rs2 : rs2(inst);

    uint64_t addr = hart->regs[rs1_l];
    uint64_t rs2_val = hart->regs[rs2_l];

    std::optional<uint64_t> val = AMO(hart, addr, rs2_val, [](uint64_t a, uint64_t b){ return std::min(int64_t(a), int64_t(b)); }); 
    if(val.has_value()) {
        hart->regs[rd_l] = *val;
    }
	
    if(!opers->s){
        opers->rd = rd_l;
        opers->rs1 = rs1_l;
        opers->rs2 = rs2_l;
        opers->s = true;
    }
    if(hart->dbg) hart->print_d("{0x%.8X} [AMOMIN.D] rd: %d; rs1: %d; rs2: %d",hart->pc,rd_l,rs1_l,rs2_l);
}
void exec_AMOMAX_D(struct HART *hart, uint32_t inst, CACHE_DecodedOperands* opers) {
    uint64_t rd_l = opers->s ? opers->rd : rd(inst);
    uint64_t rs1_l = opers->s ? opers->rs1 : rs1(inst);
    uint64_t rs2_l = opers->s ? opers->rs2 : rs2(inst);

    uint64_t addr = hart->regs[rs1_l];
    uint64_t rs2_val = hart->regs[rs2_l];

    std::optional<uint64_t> val = AMO(hart, addr, rs2_val, [](uint64_t a, uint64_t b){ return std::max(int64_t(a), int64_t(b)); }); 
    if(val.has_value()) {
        hart->regs[rd_l] = *val;
    }
        
    if(!opers->s){
        opers->rd = rd_l;
        opers->rs1 = rs1_l;
        opers->rs2 = rs2_l;
        opers->s = true;
    }
    if(hart->dbg) hart->print_d("{0x%.8X} [AMOMAX.D] rd: %d; rs1: %d; rs2: %d",hart->pc,rd_l,rs1_l,rs2_l);
}
void exec_AMOMINU_D(struct HART *hart, uint32_t inst, CACHE_DecodedOperands* opers) {
    uint64_t rd_l = opers->s ? opers->rd : rd(inst);
    uint64_t rs1_l = opers->s ? opers->rs1 : rs1(inst);
    uint64_t rs2_l = opers->s ? opers->rs2 : rs2(inst);

    uint64_t addr = hart->regs[rs1_l];
    uint64_t rs2_val = hart->regs[rs2_l];

    std::optional<uint64_t> val = AMO(hart, addr, rs2_val, [](uint64_t a, uint64_t b){ return std::min(a,b); }); 
    if(val.has_value()) {
        hart->regs[rd_l] = *val;
    }

    if(!opers->s){
        opers->rd = rd_l;
        opers->rs1 = rs1_l;
        opers->rs2 = rs2_l;
        opers->s = true;
    }
    if(hart->dbg) hart->print_d("{0x%.8X} [AMOMINU.D] rd: %d; rs1: %d; rs2: %d",hart->pc,rd_l,rs1_l,rs2_l);
}
void exec_AMOMAXU_D(struct HART *hart, uint32_t inst, CACHE_DecodedOperands* opers) {
    uint64_t rd_l = opers->s ? opers->rd : rd(inst);
    uint64_t rs1_l = opers->s ? opers->rs1 : rs1(inst);
    uint64_t rs2_l = opers->s ? opers->rs2 : rs2(inst);

    uint64_t addr = hart->regs[rs1_l];
    uint64_t rs2_val = hart->regs[rs2_l];

    std::optional<uint64_t> val = AMO(hart, addr, rs2_val, [](uint64_t a, uint64_t b){ return std::max(a,b); }); 
    if(val.has_value()) {
        hart->regs[rd_l] = *val;
    }
	
    if(!opers->s){
        opers->rd = rd_l;
        opers->rs1 = rs1_l;
        opers->rs2 = rs2_l;
        opers->s = true;
    }
    if(hart->dbg) hart->print_d("{0x%.8X} [AMOMAXU.D] rd: %d; rs1: %d; rs2: %d",hart->pc,rd_l,rs1_l,rs2_l);
}

/// RV32A

void exec_LR_W(struct HART *hart, uint32_t inst, CACHE_DecodedOperands* opers) {
    uint64_t rd_l = opers->s ? opers->rd : rd(inst);
    uint64_t rs1_l = opers->s ? opers->rs1 : rs1(inst);

    uint64_t addr = hart->regs[rs1_l];

    if(addr % 4 != 0) {
        hart->cpu_trap(EXC_LOAD_ADDR_MISALIGNED,addr,false);
    } else {
        std::optional<uint64_t> val = hart->mmio->load(hart,addr,32);
        if(val.has_value()) {
            hart->regs[rd_l] = (uint64_t)(uint32_t) *val;
            hart->reservation_addr = addr;
            hart->reservation_size = 32;
            hart->reservation_valid = true;
            hart->reservation_value = (uint64_t)(uint32_t) *val;
        }
    }

    if(!opers->s){
        opers->rd = rd_l;
        opers->rs1 = rs1_l;
        opers->s = true;
    }
    if(hart->dbg) hart->print_d("{0x%.8X} [LR.W] rd: %d; rs1: %d",hart->pc,rd_l,rs1_l);
}
void exec_SC_W(struct HART *hart, uint32_t inst, CACHE_DecodedOperands* opers) {
    uint64_t rd_l = opers->s ? opers->rs1 : rd(inst);
    uint64_t rs1_l = opers->s ? opers->rs1 : rs1(inst);
    uint64_t rs2_l = opers->s ? opers->rs1 : rs2(inst);

    uint64_t addr = hart->regs[rs1_l];

    if(hart->reservation_valid && hart->reservation_size == 32 && hart->reservation_addr == addr) {
        hart->mmio->store(hart,addr, 32, (uint32_t)hart->regs[rs2_l]);
        hart->regs[rd_l] = 0;
    } else {
        hart->regs[rd_l] = 1;
    }
    hart->reservation_valid = false;

    if(!opers->s){
        opers->rd = rd_l;
        opers->rs1 = rs1_l;
        opers->rs2 = rs2_l;
        opers->s = true;
    }
    if(hart->dbg) hart->print_d("{0x%.8X} [SC.W] rd: %d; rs1: %d; rs2: %d",hart->pc,rd_l,rs1_l,rs2_l);
}

template<typename Functor>
std::optional<uint32_t> AMO32(HART* hart, uint64_t addr, uint32_t rs2, Functor op) {
    if(addr % 4 != 0) {
        hart->cpu_trap(EXC_STORE_ADDR_MISALIGNED, addr, false);
        return std::nullopt;
    }

    auto val = hart->mmio->load(hart, addr, 32);
    if(!val.has_value()) {
        hart->cpu_trap(EXC_LOAD_ACCESS_FAULT, addr, false);
        return std::nullopt;
    }

    uint32_t t = *val;

    uint32_t new_val = op(t, rs2);

    hart->mmio->store(hart, addr, 32, new_val);

    return t;
}

void exec_AMOSWAP_W(struct HART *hart, uint32_t inst, CACHE_DecodedOperands* opers) {
    uint64_t rd_l = opers->s ? opers->rs1 : rd(inst);
    uint64_t rs1_l = opers->s ? opers->rs1 : rs1(inst);
    uint64_t rs2_l = opers->s ? opers->rs1 : rs2(inst);

    uint64_t addr = hart->regs[rs1_l];
    uint64_t rs2_val = hart->regs[rs2_l];

    std::optional<uint64_t> val = AMO32(hart, addr, rs2_val, [](uint32_t a, uint32_t b){ return b; }); 
    if(val.has_value()) {
        hart->regs[rd_l] = (uint64_t)(int64_t)(int32_t) *val;
    }

    if(!opers->s){
        opers->rd = rd_l;
        opers->rs1 = rs1_l;
        opers->rs2 = rs2_l;
        opers->s = true;
    }
    if(hart->dbg) hart->print_d("{0x%.8X} [AMOSWAP.W] rd: %d; rs1: %d; rs2: %d",hart->pc,rd_l,rs1_l,rs2_l);
}
void exec_AMOADD_W(struct HART *hart, uint32_t inst, CACHE_DecodedOperands* opers) {
    uint64_t rd_l = opers->s ? opers->rs1 : rd(inst);
    uint64_t rs1_l = opers->s ? opers->rs1 : rs1(inst);
    uint64_t rs2_l = opers->s ? opers->rs1 : rs2(inst);

    uint64_t addr = hart->regs[rs1_l];
    uint64_t rs2_val = hart->regs[rs2_l];

    std::optional<uint64_t> val = AMO32(hart, addr, rs2_val, [](uint32_t a, uint32_t b){ return a+b; }); 
    if(val.has_value()) {
        hart->regs[rd_l] = (uint64_t)(int64_t)(int32_t) *val;
    }

    if(!opers->s){
        opers->rd = rd_l;
        opers->rs1 = rs1_l;
        opers->rs2 = rs2_l;
        opers->s = true;
    }
    if(hart->dbg) hart->print_d("{0x%.8X} [AMOADD.W] rd: %d; rs1: %d; rs2: %d",hart->pc,rd_l,rs1_l,rs2_l);
}
void exec_AMOXOR_W(struct HART *hart, uint32_t inst, CACHE_DecodedOperands* opers) {
    uint64_t rd_l = opers->s ? opers->rs1 : rd(inst);
    uint64_t rs1_l = opers->s ? opers->rs1 : rs1(inst);
    uint64_t rs2_l = opers->s ? opers->rs1 : rs2(inst);

    uint64_t addr = hart->regs[rs1_l];
    uint64_t rs2_val = hart->regs[rs2_l];

    std::optional<uint64_t> val = AMO32(hart, addr, rs2_val, [](uint32_t a, uint32_t b){ return a^b; }); 
    if(val.has_value()) {
        hart->regs[rd_l] = (uint64_t)(int64_t)(int32_t) *val;
    }

    if(!opers->s){
        opers->rd = rd_l;
        opers->rs1 = rs1_l;
        opers->rs2 = rs2_l;
        opers->s = true;
    }
    if(hart->dbg) hart->print_d("{0x%.8X} [AMOXOR.W] rd: %d; rs1: %d; rs2: %d",hart->pc,rd_l,rs1_l,rs2_l);
}
void exec_AMOAND_W(struct HART *hart, uint32_t inst, CACHE_DecodedOperands* opers) {
    uint64_t rd_l = opers->s ? opers->rs1 : rd(inst);
    uint64_t rs1_l = opers->s ? opers->rs1 : rs1(inst);
    uint64_t rs2_l = opers->s ? opers->rs1 : rs2(inst);

    uint64_t addr = hart->regs[rs1_l];
    uint64_t rs2_val = hart->regs[rs2_l];

    std::optional<uint64_t> val = AMO32(hart, addr, rs2_val, [](uint32_t a, uint32_t b){ return a&b; }); 
    if(val.has_value()) {
        hart->regs[rd_l] = (uint64_t)(int64_t)(int32_t) *val;
    }

    if(!opers->s){
        opers->rd = rd_l;
        opers->rs1 = rs1_l;
        opers->rs2 = rs2_l;
        opers->s = true;
    }
    if(hart->dbg) hart->print_d("{0x%.8X} [AMOAND.W] rd: %d; rs1: %d; rs2: %d",hart->pc,rd_l,rs1_l,rs2_l);
}
void exec_AMOOR_W(struct HART *hart, uint32_t inst, CACHE_DecodedOperands* opers) {
    uint64_t rd_l = opers->s ? opers->rs1 : rd(inst);
    uint64_t rs1_l = opers->s ? opers->rs1 : rs1(inst);
    uint64_t rs2_l = opers->s ? opers->rs1 : rs2(inst);

    uint64_t addr = hart->regs[rs1_l];
    uint64_t rs2_val = hart->regs[rs2_l];

    std::optional<uint64_t> val = AMO32(hart, addr, rs2_val, [](uint32_t a, uint32_t b){ return a|b; }); 
    if(val.has_value()) {
        hart->regs[rd_l] = (uint64_t)(int64_t)(int32_t) *val;
    }

    if(!opers->s){
        opers->rd = rd_l;
        opers->rs1 = rs1_l;
        opers->rs2 = rs2_l;
        opers->s = true;
    }
    if(hart->dbg) hart->print_d("{0x%.8X} [AMOOR.W] rd: %d; rs1: %d; rs2: %d",hart->pc,rd_l,rs1_l,rs2_l);
}
void exec_AMOMIN_W(struct HART *hart, uint32_t inst, CACHE_DecodedOperands* opers) {
    uint64_t rd_l = opers->s ? opers->rs1 : rd(inst);
    uint64_t rs1_l = opers->s ? opers->rs1 : rs1(inst);
    uint64_t rs2_l = opers->s ? opers->rs1 : rs2(inst);

    uint64_t addr = hart->regs[rs1_l];
    uint64_t rs2_val = hart->regs[rs2_l];

    std::optional<uint64_t> val = AMO32(hart, addr, rs2_val, [](uint32_t a, uint32_t b){ return std::min(int32_t(a), int32_t(b)); }); 
    if(val.has_value()) {
        hart->regs[rd_l] = (uint64_t)(int64_t)(int32_t) *val;
    }

    if(!opers->s){
        opers->rd = rd_l;
        opers->rs1 = rs1_l;
        opers->rs2 = rs2_l;
        opers->s = true;
    }
    if(hart->dbg) hart->print_d("{0x%.8X} [AMOMIN.W] rd: %d; rs1: %d; rs2: %d",hart->pc,rd_l,rs1_l,rs2_l);
}
void exec_AMOMAX_W(struct HART *hart, uint32_t inst, CACHE_DecodedOperands* opers) {
    uint64_t rd_l = opers->s ? opers->rs1 : rd(inst);
    uint64_t rs1_l = opers->s ? opers->rs1 : rs1(inst);
    uint64_t rs2_l = opers->s ? opers->rs2 : rs2(inst);

    uint64_t addr = hart->regs[rs1_l];
    uint64_t rs2_val = hart->regs[rs2_l];

    std::optional<uint64_t> val = AMO32(hart, addr, rs2_val, [](uint32_t a, uint32_t b){ return std::max(int32_t(a), int32_t(b)); }); 
    if(val.has_value()) {
        hart->regs[rd_l] = (uint64_t)(int64_t)(int32_t) *val;
    }

    if(!opers->s){
        opers->rd = rd_l;
        opers->rs1 = rs1_l;
        opers->rs2 = rs2_l;
        opers->s = true;
    }
    if(hart->dbg) hart->print_d("{0x%.8X} [AMOMAX.W] rd: %d; rs1: %d; rs2: %d",hart->pc,rd_l,rs1_l,rs2_l);
}
void exec_AMOMINU_W(struct HART *hart, uint32_t inst, CACHE_DecodedOperands* opers) {
    uint64_t rd_l = opers->s ? opers->rs1 : rd(inst);
    uint64_t rs1_l = opers->s ? opers->rs1 : rs1(inst);
    uint64_t rs2_l = opers->s ? opers->rs1 : rs2(inst);

    uint64_t addr = hart->regs[rs1_l];
    uint64_t rs2_val = hart->regs[rs2_l];

    std::optional<uint64_t> val = AMO32(hart, addr, rs2_val, [](uint32_t a, uint32_t b){ return std::min(uint32_t(a), uint32_t(b)); }); 
    if(val.has_value()) {
        hart->regs[rd_l] = (uint64_t)(int64_t)(int32_t)  *val;
    }

    if(!opers->s){
        opers->rd = rd_l;
        opers->rs1 = rs1_l;
        opers->rs2 = rs2_l;
        opers->s = true;
    }
    if(hart->dbg) hart->print_d("{0x%.8X} [AMOMINU.W] rd: %d; rs1: %d; rs2: %d",hart->pc,rd_l,rs1_l,rs2_l);
}
void exec_AMOMAXU_W(struct HART *hart, uint32_t inst, CACHE_DecodedOperands* opers) {
    uint64_t rd_l = opers->s ? opers->rs1 : rd(inst);
    uint64_t rs1_l = opers->s ? opers->rs1 : rs1(inst);
    uint64_t rs2_l = opers->s ? opers->rs1 : rs2(inst);

    uint64_t addr = hart->regs[rs1_l];
    uint64_t rs2_val = hart->regs[rs2_l];

    std::optional<uint64_t> val = AMO32(hart, addr, rs2_val, [](uint32_t a, uint32_t b){ return std::max(uint32_t(a), uint32_t(b)); }); 
    if(val.has_value()) {
        hart->regs[rd_l] = (uint64_t)(int64_t)(int32_t) *val;
    }

    if(!opers->s){
        opers->rd = rd_l;
        opers->rs1 = rs1_l;
        opers->rs2 = rs2_l;
        opers->s = true;
    }
    if(hart->dbg) hart->print_d("{0x%.8X} [AMOMAXU.W] rd: %d; rs1: %d; rs2: %d",hart->pc,rd_l,rs1_l,rs2_l);
}