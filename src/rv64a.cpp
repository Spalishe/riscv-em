#include "../include/instset.h"
#include "../include/cpu.h"
#include "../include/csr.h"
#include <iostream>

void exec_LR_D(struct HART *hart, uint32_t inst) {
    uint64_t addr = hart->regs[rs1(inst)];

    if(addr % 8 != 0) {
        cpu_trap(hart,EXC_LOAD_ADDR_MISALIGNED,addr,false);
    } else {
        std::optional<uint64_t> val = hart->mmio->load(hart,addr,64);
        if(val.has_value()) {
            hart->regs[rd(inst)] = *val;
            hart->reservation_addr = addr;
            hart->reservation_size = 64;
            hart->reservation_valid = true;
            hart->reservation_value = *val;
        }
    }

    print_d(hart,"{0x%.8X} [LR.D] rd: %d; rs1: %d",hart->pc,rd(inst),rs1(inst));
}
void exec_SC_D(struct HART *hart, uint32_t inst) {
    uint64_t addr = hart->regs[rs1(inst)];

    if(hart->reservation_valid && hart->reservation_size == 64 && hart->reservation_addr == addr) {
        hart->mmio->store(hart,addr, 64, hart->regs[rs2(inst)]);
        hart->regs[rd(inst)] = 0;
    } else {
        hart->regs[rd(inst)] = 1;
    }
    hart->reservation_valid = false;

    print_d(hart,"{0x%.8X} [SC.D] rd: %d; rs1: %d; rs2: %d",hart->pc,rd(inst),rs1(inst),rs2(inst));
}

template<typename Functor>
std::optional<uint64_t> AMO(HART* hart, uint64_t addr, uint64_t rs2, Functor op) {
    if(addr % 8 != 0) {
        cpu_trap(hart, EXC_STORE_ADDR_MISALIGNED, addr, false);
        return std::nullopt;
    }

    auto val = hart->mmio->load(hart, addr, 64);
    if(!val.has_value()) {
        cpu_trap(hart, EXC_LOAD_ACCESS_FAULT, addr, false);
        return std::nullopt;
    }

    uint64_t t = *val;

    uint64_t new_val = op(t, rs2);

    hart->mmio->store(hart, addr, 64, new_val);

    return t;
}

void exec_AMOSWAP_D(struct HART *hart, uint32_t inst) {
    uint64_t addr = hart->regs[rs1(inst)];
    uint64_t rs2_val = hart->regs[rs2(inst)];

    std::optional<uint64_t> val = AMO(hart, addr, rs2_val, [](uint64_t a, uint64_t b){ return b; }); 
    if(val.has_value()) {
        hart->regs[rd(inst)] = *val;
    }

    print_d(hart,"{0x%.8X} [AMOSWAP.D] rd: %d; rs1: %d; rs2: %d",hart->pc,rd(inst),rs1(inst),rs2(inst));
}
void exec_AMOADD_D(struct HART *hart, uint32_t inst) {
    uint64_t addr = hart->regs[rs1(inst)];
    uint64_t rs2_val = hart->regs[rs2(inst)];

    std::optional<uint64_t> val = AMO(hart, addr, rs2_val, [](uint64_t a, uint64_t b){ return a+b; }); 
    if(val.has_value()) {
        hart->regs[rd(inst)] = *val;
    }

    print_d(hart,"{0x%.8X} [AMOADD.D] rd: %d; rs1: %d; rs2: %d",hart->pc,rd(inst),rs1(inst),rs2(inst));
}
void exec_AMOXOR_D(struct HART *hart, uint32_t inst) {
    uint64_t addr = hart->regs[rs1(inst)];
    uint64_t rs2_val = hart->regs[rs2(inst)];

    std::optional<uint64_t> val = AMO(hart, addr, rs2_val, [](uint64_t a, uint64_t b){ return a^b; }); 
    if(val.has_value()) {
        hart->regs[rd(inst)] = *val;
    }

    print_d(hart,"{0x%.8X} [AMOXOR.D] rd: %d; rs1: %d; rs2: %d",hart->pc,rd(inst),rs1(inst),rs2(inst));
}
void exec_AMOAND_D(struct HART *hart, uint32_t inst) {
    uint64_t addr = hart->regs[rs1(inst)];
    uint64_t rs2_val = hart->regs[rs2(inst)];

    std::optional<uint64_t> val = AMO(hart, addr, rs2_val, [](uint64_t a, uint64_t b){ return a&b; }); 
    if(val.has_value()) {
        hart->regs[rd(inst)] = *val;
    }

    print_d(hart,"{0x%.8X} [AMOAND.D] rd: %d; rs1: %d; rs2: %d",hart->pc,rd(inst),rs1(inst),rs2(inst));
}
void exec_AMOOR_D(struct HART *hart, uint32_t inst) {
    uint64_t addr = hart->regs[rs1(inst)];
    uint64_t rs2_val = hart->regs[rs2(inst)];

    std::optional<uint64_t> val = AMO(hart, addr, rs2_val, [](uint64_t a, uint64_t b){ return a|b; }); 
    if(val.has_value()) {
        hart->regs[rd(inst)] = *val;
    }

    print_d(hart,"{0x%.8X} [AMOOR.D] rd: %d; rs1: %d; rs2: %d",hart->pc,rd(inst),rs1(inst),rs2(inst));
}
void exec_AMOMIN_D(struct HART *hart, uint32_t inst) {
    uint64_t addr = hart->regs[rs1(inst)];
    uint64_t rs2_val = hart->regs[rs2(inst)];

    std::optional<uint64_t> val = AMO(hart, addr, rs2_val, [](uint64_t a, uint64_t b){ return std::min(int64_t(a), int64_t(b)); }); 
    if(val.has_value()) {
        hart->regs[rd(inst)] = *val;
    }

    print_d(hart,"{0x%.8X} [AMOMIN.D] rd: %d; rs1: %d; rs2: %d",hart->pc,rd(inst),rs1(inst),rs2(inst));
}
void exec_AMOMAX_D(struct HART *hart, uint32_t inst) {
    uint64_t addr = hart->regs[rs1(inst)];
    uint64_t rs2_val = hart->regs[rs2(inst)];

    std::optional<uint64_t> val = AMO(hart, addr, rs2_val, [](uint64_t a, uint64_t b){ return std::max(int64_t(a), int64_t(b)); }); 
    if(val.has_value()) {
        hart->regs[rd(inst)] = *val;
    }

    print_d(hart,"{0x%.8X} [AMOMAX.D] rd: %d; rs1: %d; rs2: %d",hart->pc,rd(inst),rs1(inst),rs2(inst));
}
void exec_AMOMINU_D(struct HART *hart, uint32_t inst) {
    uint64_t addr = hart->regs[rs1(inst)];
    uint64_t rs2_val = hart->regs[rs2(inst)];

    std::optional<uint64_t> val = AMO(hart, addr, rs2_val, [](uint64_t a, uint64_t b){ return std::min(a,b); }); 
    if(val.has_value()) {
        hart->regs[rd(inst)] = *val;
    }

    print_d(hart,"{0x%.8X} [AMOMINU.D] rd: %d; rs1: %d; rs2: %d",hart->pc,rd(inst),rs1(inst),rs2(inst));
}
void exec_AMOMAXU_D(struct HART *hart, uint32_t inst) {
    uint64_t addr = hart->regs[rs1(inst)];
    uint64_t rs2_val = hart->regs[rs2(inst)];

    std::optional<uint64_t> val = AMO(hart, addr, rs2_val, [](uint64_t a, uint64_t b){ return std::max(a,b); }); 
    if(val.has_value()) {
        hart->regs[rd(inst)] = *val;
    }

    print_d(hart,"{0x%.8X} [AMOMAXU.D] rd: %d; rs1: %d; rs2: %d",hart->pc,rd(inst),rs1(inst),rs2(inst));
}