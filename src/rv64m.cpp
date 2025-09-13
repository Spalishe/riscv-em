#include "../include/instset.h"
#include "../include/cpu.h"
#include "../include/csr.h"
#include <iostream>

#include <limits>

// R-Type

void exec_MULW(struct HART *hart, uint32_t inst) {
	hart->regs[rd(inst)] = (uint64_t)((int32_t)hart->regs[rs1(inst)] * (int32_t)hart->regs[rs2(inst)]);
	hart->print_d("{0x%.8X} [MULW] rs1: %d; rs2: %d; rd: %d",hart->pc,rs1(inst),rs2(inst),rd(inst));
}

void exec_DIVW(struct HART *hart, uint32_t inst) {
    if(hart->regs[rs2(inst)] == 0) {
	    hart->regs[rd(inst)] = (int64_t)-1;
    } else if(hart->regs[rs1(inst)] == std::numeric_limits<int32_t>::min() && hart->regs[rs2(inst)] == -1) {
        hart->regs[rd(inst)] = hart->regs[rs1(inst)];
    } else {
        hart->regs[rd(inst)] = (uint64_t)((int32_t)hart->regs[rs1(inst)] / (int32_t)hart->regs[rs2(inst)]);
    }
	hart->print_d("{0x%.8X} [DIVW] rs1: %d; rs2: %d; rd: %d",hart->pc,rs1(inst),rs2(inst),rd(inst));
}
void exec_DIVUW(struct HART *hart, uint32_t inst) {
    if(hart->regs[rs2(inst)] == 0) {
	    hart->regs[rd(inst)] = std::numeric_limits<uint64_t>::max();
    } else {
        hart->regs[rd(inst)] = (uint64_t)(int64_t)(int32_t)(hart->regs[rs1(inst)] / hart->regs[rs2(inst)]);
    }
	hart->print_d("{0x%.8X} [DIVUW] rs1: %d; rs2: %d; rd: %d",hart->pc,rs1(inst),rs2(inst),rd(inst));
}
void exec_REMW(struct HART *hart, uint32_t inst) {
    if(hart->regs[rs2(inst)] == 0) {
	    hart->regs[rd(inst)] = hart->regs[rs1(inst)];
    } else if(hart->regs[rs1(inst)] == std::numeric_limits<int32_t>::min() && hart->regs[rs2(inst)] == -1) {
        hart->regs[rd(inst)] = 0;
    } else {
        hart->regs[rd(inst)] = (uint64_t)(int64_t)((int32_t)hart->regs[rs1(inst)] % (int32_t)hart->regs[rs2(inst)]);
    }
	hart->print_d("{0x%.8X} [REMW] rs1: %d; rs2: %d; rd: %d",hart->pc,rs1(inst),rs2(inst),rd(inst));
}
void exec_REMUW(struct HART *hart, uint32_t inst) {
    if(hart->regs[rs2(inst)] == 0) {
	    hart->regs[rd(inst)] = hart->regs[rs1(inst)];
    } else {
        hart->regs[rd(inst)] = (uint64_t)(int64_t)(int32_t)((uint32_t)hart->regs[rs1(inst)] % (uint32_t)hart->regs[rs2(inst)]);
    }
	hart->print_d("{0x%.8X} [REMUW] rs1: %d; rs2: %d; rd: %d",hart->pc,rs1(inst),rs2(inst),rd(inst));
}