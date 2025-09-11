#include "../include/instset.h"
#include "../include/cpu.h"
#include "../include/csr.h"
#include <iostream>

// ZiCSR

bool csr_accessible(uint16_t csr_addr, uint64_t current_priv, bool write) {
    uint64_t csr_level;

    if (csr_addr >= 0x000 && csr_addr <= 0x0FF) csr_level = 0;
    else if (csr_addr >= 0x100 && csr_addr <= 0x1FF) csr_level = 1;
    // REMOVE 0x744 AFTER TESTING!!!!!!!!!!!!!!!!!
    else if ((csr_addr >= 0x300 && csr_addr <= 0x3FF) || (csr_addr >= 0xB00 && csr_addr <= 0xBFF) || (csr_addr >= 0xF00 && csr_addr <= 0xFFF)) csr_level = 3;
    else {
        csr_level = 0;
    }

    if ((current_priv) < (csr_level)) {
        return false;
    }

    if (csr_addr == 0xF11
        || csr_addr == 0xF12
        || csr_addr == 0xF13
        || csr_addr == 0xF14
        || csr_addr == 0xB80
        || csr_addr == 0xB82
        || csr_addr == 0xC00
        || csr_addr == 0xC01
    ) {
        return !write; // RO
    }

    return true;
}
void exec_CSRRW(struct HART *hart, uint32_t inst) {
	uint64_t imm = imm_Zicsr(inst);
    if(!csr_accessible(imm,hart->mode,(rs1(inst) != 0))) {
        hart->cpu_trap(EXC_ILLEGAL_INSTRUCTION,hart->mode,false);
    }
    if (rd(inst) != 0) {
        hart->regs[rd(inst)] = hart->csrs[imm];
    }
	hart->csrs[imm] = hart->regs[rs1(inst)];
	hart->print_d("{0x%.8X} [CSRRW] rs1: %d; rd: %d; imm: int %d uint %u",hart->pc,rs1(inst),rd(inst),(int64_t) imm_Zicsr(inst), imm_Zicsr(inst));
}
void exec_CSRRS(struct HART *hart, uint32_t inst) {
	uint64_t imm = imm_Zicsr(inst);
    if(!csr_accessible(imm,hart->mode,(rs1(inst) != 0))) {
        hart->cpu_trap(EXC_ILLEGAL_INSTRUCTION,hart->mode,false);
    }
    uint64_t init_val = hart->csrs[imm];
    if (rs1(inst) != 0) {
        uint64_t mask = hart->regs[rs1(inst)];
        hart->csrs[imm] = hart->csrs[imm] | mask;
    }
    hart->regs[rd(inst)] = init_val;
	hart->print_d("{0x%.8X} [CSRRS] rs1: %d; rd: %d; imm: int %d uint %u",hart->pc,rs1(inst),rd(inst),(int64_t) imm_Zicsr(inst), imm_Zicsr(inst));
}
void exec_CSRRC(struct HART *hart, uint32_t inst) {
	uint64_t imm = imm_Zicsr(inst);
    if(!csr_accessible(imm,hart->mode,(rs1(inst) != 0))) {
        hart->cpu_trap(EXC_ILLEGAL_INSTRUCTION,hart->mode,false);
    }
    uint64_t init_val = hart->csrs[imm];
    if (rs1(inst) != 0) {
        uint64_t mask = hart->regs[rs1(inst)];
        hart->csrs[imm] = hart->csrs[imm] & ~mask;
    }
    hart->regs[rd(inst)] = init_val;
	hart->print_d("{0x%.8X} [CSRRC] rs1: %d; rd: %d; imm: int %d uint %u",hart->pc,rs1(inst),rd(inst),(int64_t) imm_Zicsr(inst), imm_Zicsr(inst));
}

void exec_CSRRWI(struct HART *hart, uint32_t inst) {
	uint64_t imm = imm_Zicsr(inst);
    if(!csr_accessible(imm,hart->mode,(rs1(inst) != 0))) {
        hart->cpu_trap(EXC_ILLEGAL_INSTRUCTION,hart->mode,false);
    }
    if (rd(inst) != 0) {
        hart->regs[rd(inst)] = hart->csrs[imm];
    }
	hart->csrs[imm] = rs1(inst);
	hart->print_d("{0x%.8X} [CSRRWI] rs1: %d; rd: %d; imm: int %d uint %u",hart->pc,rs1(inst),rd(inst),(int64_t) imm_Zicsr(inst), imm_Zicsr(inst));
}
void exec_CSRRSI(struct HART *hart, uint32_t inst) {
	uint64_t imm = imm_Zicsr(inst);
    if(!csr_accessible(imm,hart->mode,(rs1(inst) != 0))) {
        hart->cpu_trap(EXC_ILLEGAL_INSTRUCTION,hart->mode,false);
    }
    uint64_t init_val = hart->csrs[imm];
    if (rs1(inst) != 0) {
        uint64_t mask = rs1(inst);
        hart->csrs[imm] = hart->csrs[imm] | mask;
    }
    hart->regs[rd(inst)] = init_val;
	hart->print_d("{0x%.8X} [CSRRSI] rs1: %d; rd: %d; imm: int %d uint %u",hart->pc,rs1(inst),rd(inst),(int64_t) imm_Zicsr(inst), imm_Zicsr(inst));
}
void exec_CSRRCI(struct HART *hart, uint32_t inst) {
	uint64_t imm = imm_Zicsr(inst);
    if(!csr_accessible(imm,hart->mode,(rs1(inst) != 0))) {
        hart->cpu_trap(EXC_ILLEGAL_INSTRUCTION,hart->mode,false);
    }
    uint64_t init_val = hart->csrs[imm];
    if (rs1(inst) != 0) {
        uint64_t mask = rs1(inst);
        hart->csrs[imm] = hart->csrs[imm] & ~mask;
    }
    hart->regs[rd(inst)] = init_val;
	hart->print_d("{0x%.8X} [CSRRCI] rs1: %d; rd: %d; imm: int %d uint %u",hart->pc,rs1(inst),rd(inst),(int64_t) imm_Zicsr(inst), imm_Zicsr(inst));
}