#include "../include/instset.h"
#include "../include/cpu.h"
#include "../include/csr.h"
#include <iostream>

void exec_LR_W(struct HART *hart, uint32_t inst) {
    uint64_t addr = hart->regs[rs1(inst)];

    if(addr % 4 != 0) {
        hart->cpu_trap(EXC_LOAD_ADDR_MISALIGNED,addr,false);
    } else {
        std::optional<uint64_t> val = hart->mmio->load(hart,addr,32);
        if(val.has_value()) {
            hart->regs[rd(inst)] = (uint64_t)(uint32_t) *val;
            hart->reservation_addr = addr;
            hart->reservation_size = 32;
            hart->reservation_valid = true;
            hart->reservation_value = (uint64_t)(uint32_t) *val;
        }
    }

    hart->print_d("{0x%.8X} [LR.W] rd: %d; rs1: %d",hart->pc,rd(inst),rs1(inst));
}
void exec_SC_W(struct HART *hart, uint32_t inst) {
    uint64_t addr = hart->regs[rs1(inst)];

    if(hart->reservation_valid && hart->reservation_size == 32 && hart->reservation_addr == addr) {
        hart->mmio->store(hart,addr, 32, (uint32_t)hart->regs[rs2(inst)]);
        hart->regs[rd(inst)] = 0;
    } else {
        hart->regs[rd(inst)] = 1;
    }
    hart->reservation_valid = false;

    hart->print_d("{0x%.8X} [SC.W] rd: %d; rs1: %d; rs2: %d",hart->pc,rd(inst),rs1(inst),rs2(inst));
}

template<typename Functor>
std::optional<uint32_t> AMO(HART* hart, uint64_t addr, uint32_t rs2, Functor op) {
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

void exec_AMOSWAP_W(struct HART *hart, uint32_t inst) {
    uint64_t addr = hart->regs[rs1(inst)];
    uint64_t rs2_val = hart->regs[rs2(inst)];

    std::optional<uint64_t> val = AMO(hart, addr, rs2_val, [](uint32_t a, uint32_t b){ return b; }); 
    if(val.has_value()) {
        hart->regs[rd(inst)] = *val;
    }

    hart->print_d("{0x%.8X} [AMOSWAP.W] rd: %d; rs1: %d; rs2: %d",hart->pc,rd(inst),rs1(inst),rs2(inst));
}
void exec_AMOADD_W(struct HART *hart, uint32_t inst) {
    uint64_t addr = hart->regs[rs1(inst)];
    uint64_t rs2_val = hart->regs[rs2(inst)];

    std::optional<uint64_t> val = AMO(hart, addr, rs2_val, [](uint32_t a, uint32_t b){ return a+b; }); 
    if(val.has_value()) {
        hart->regs[rd(inst)] = *val;
    }

    hart->print_d("{0x%.8X} [AMOADD.W] rd: %d; rs1: %d; rs2: %d",hart->pc,rd(inst),rs1(inst),rs2(inst));
}
void exec_AMOXOR_W(struct HART *hart, uint32_t inst) {
    uint64_t addr = hart->regs[rs1(inst)];
    uint64_t rs2_val = hart->regs[rs2(inst)];

    std::optional<uint64_t> val = AMO(hart, addr, rs2_val, [](uint32_t a, uint32_t b){ return a^b; }); 
    if(val.has_value()) {
        hart->regs[rd(inst)] = *val;
    }

    hart->print_d("{0x%.8X} [AMOXOR.W] rd: %d; rs1: %d; rs2: %d",hart->pc,rd(inst),rs1(inst),rs2(inst));
}
void exec_AMOAND_W(struct HART *hart, uint32_t inst) {
    uint64_t addr = hart->regs[rs1(inst)];
    uint64_t rs2_val = hart->regs[rs2(inst)];

    std::optional<uint64_t> val = AMO(hart, addr, rs2_val, [](uint32_t a, uint32_t b){ return a&b; }); 
    if(val.has_value()) {
        hart->regs[rd(inst)] = *val;
    }

    hart->print_d("{0x%.8X} [AMOAND.W] rd: %d; rs1: %d; rs2: %d",hart->pc,rd(inst),rs1(inst),rs2(inst));
}
void exec_AMOOR_W(struct HART *hart, uint32_t inst) {
    uint64_t addr = hart->regs[rs1(inst)];
    uint64_t rs2_val = hart->regs[rs2(inst)];

    std::optional<uint64_t> val = AMO(hart, addr, rs2_val, [](uint32_t a, uint32_t b){ return a|b; }); 
    if(val.has_value()) {
        hart->regs[rd(inst)] = *val;
    }

    hart->print_d("{0x%.8X} [AMOOR.W] rd: %d; rs1: %d; rs2: %d",hart->pc,rd(inst),rs1(inst),rs2(inst));
}
void exec_AMOMIN_W(struct HART *hart, uint32_t inst) {
    uint64_t addr = hart->regs[rs1(inst)];
    uint64_t rs2_val = hart->regs[rs2(inst)];

    std::optional<uint64_t> val = AMO(hart, addr, rs2_val, [](uint32_t a, uint32_t b){ return std::min(int32_t(a), int32_t(b)); }); 
    if(val.has_value()) {
        hart->regs[rd(inst)] = *val;
    }

    hart->print_d("{0x%.8X} [AMOMIN.W] rd: %d; rs1: %d; rs2: %d",hart->pc,rd(inst),rs1(inst),rs2(inst));
}
void exec_AMOMAX_W(struct HART *hart, uint32_t inst) {
    uint64_t addr = hart->regs[rs1(inst)];
    uint64_t rs2_val = hart->regs[rs2(inst)];

    std::optional<uint64_t> val = AMO(hart, addr, rs2_val, [](uint32_t a, uint32_t b){ return std::max(int32_t(a), int32_t(b)); }); 
    if(val.has_value()) {
        hart->regs[rd(inst)] = *val;
    }

    hart->print_d("{0x%.8X} [AMOMAX.W] rd: %d; rs1: %d; rs2: %d",hart->pc,rd(inst),rs1(inst),rs2(inst));
}
void exec_AMOMINU_W(struct HART *hart, uint32_t inst) {
    uint64_t addr = hart->regs[rs1(inst)];
    uint64_t rs2_val = hart->regs[rs2(inst)];

    std::optional<uint64_t> val = AMO(hart, addr, rs2_val, [](uint32_t a, uint32_t b){ return std::min(uint32_t(a), uint32_t(b)); }); 
    if(val.has_value()) {
        hart->regs[rd(inst)] = *val;
    }

    hart->print_d("{0x%.8X} [AMOMINU.W] rd: %d; rs1: %d; rs2: %d",hart->pc,rd(inst),rs1(inst),rs2(inst));
}
void exec_AMOMAXU_W(struct HART *hart, uint32_t inst) {
    uint64_t addr = hart->regs[rs1(inst)];
    uint64_t rs2_val = hart->regs[rs2(inst)];

    std::optional<uint64_t> val = AMO(hart, addr, rs2_val, [](uint32_t a, uint32_t b){ return std::max(uint32_t(a), uint32_t(b)); }); 
    if(val.has_value()) {
        hart->regs[rd(inst)] = *val;
    }

    hart->print_d("{0x%.8X} [AMOMAXU.W] rd: %d; rs1: %d; rs2: %d",hart->pc,rd(inst),rs1(inst),rs2(inst));
}