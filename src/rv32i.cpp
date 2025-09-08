#include "../include/instset.h"
#include "../include/cpu.h"
#include "../include/csr.h"
#include <iostream>

#define ADDR_MISALIGNED(addr) (addr & 0x3)

// R-Type

void exec_ADD(struct HART *hart, uint32_t inst) {
	hart->regs[rd(inst)] = hart->regs[rs1(inst)] + hart->regs[rs2(inst)];
	print_d(hart,"{0x%.8X} [ADD] rs1: %d; rs2: %d; rd: %d",hart->pc,rs1(inst),rs2(inst),rd(inst));
}
void exec_SUB(struct HART *hart, uint32_t inst) {
	hart->regs[rd(inst)] = hart->regs[rs1(inst)] - hart->regs[rs2(inst)];
	print_d(hart,"{0x%.8X} [SUB] rs1: %d; rs2: %d; rd: %d",hart->pc,rs1(inst),rs2(inst),rd(inst));
}
void exec_XOR(struct HART *hart, uint32_t inst) {
	hart->regs[rd(inst)] = hart->regs[rs1(inst)] ^ hart->regs[rs2(inst)];
	print_d(hart,"{0x%.8X} [XOR] rs1: %d; rs2: %d; rd: %d",hart->pc,rs1(inst),rs2(inst),rd(inst));
}
void exec_OR(struct HART *hart, uint32_t inst) {
	hart->regs[rd(inst)] = hart->regs[rs1(inst)] | hart->regs[rs2(inst)];
	print_d(hart,"{0x%.8X} [OR] rs1: %d; rs2: %d; rd: %d",hart->pc,rs1(inst),rs2(inst),rd(inst));
}
void exec_AND(struct HART *hart, uint32_t inst) {
	hart->regs[rd(inst)] = hart->regs[rs1(inst)] & hart->regs[rs2(inst)];
	print_d(hart,"{0x%.8X} [AND] rs1: %d; rs2: %d; rd: %d",hart->pc,rs1(inst),rs2(inst),rd(inst));
}
void exec_SLL(struct HART *hart, uint32_t inst) {
	hart->regs[rd(inst)] = hart->regs[rs1(inst)] << (int64_t) hart->regs[rs2(inst)];
	print_d(hart,"{0x%.8X} [SLL] rs1: %d; rs2: %d; rd: %d",hart->pc,rs1(inst),rs2(inst),rd(inst));
}
void exec_SRL(struct HART *hart, uint32_t inst) {
	hart->regs[rd(inst)] = hart->regs[rs1(inst)] >> hart->regs[rs2(inst)];
	print_d(hart,"{0x%.8X} [SRL] rs1: %d; rs2: %d; rd: %d",hart->pc,rs1(inst),rs2(inst),rd(inst));
}
void exec_SRA(struct HART *hart, uint32_t inst) {
	hart->regs[rd(inst)] = (int32_t) hart->regs[rs1(inst)] >> (int64_t) hart->regs[rs2(inst)];
	print_d(hart,"{0x%.8X} [SRA] rs1: %d; rs2: %d; rd: %d",hart->pc,rs1(inst),rs2(inst),rd(inst));
}
void exec_SLT(struct HART *hart, uint32_t inst) {
	hart->regs[rd(inst)] = ((int64_t) hart->regs[rs1(inst)] < (int64_t) hart->regs[rs2(inst)]) ? 1 : 0;
	print_d(hart,"{0x%.8X} [SLT] rs1: %d; rs2: %d; rd: %d",hart->pc,rs1(inst),rs2(inst),rd(inst));
}
void exec_SLTU(struct HART *hart, uint32_t inst) {
	hart->regs[rd(inst)] = (hart->regs[rs1(inst)] < hart->regs[rs2(inst)]) ? 1 : 0;
	print_d(hart,"{0x%.8X} [SLTU] rs1: %d; rs2: %d; rd: %d",hart->pc,rs1(inst),rs2(inst),rd(inst));
}

// I-Type
void exec_ADDI(struct HART *hart, uint32_t inst) {
    hart->regs[rd(inst)] = hart->regs[rs1(inst)] + (int64_t) imm_I(inst);
	print_d(hart,"{0x%.8X} [ADDI] rs1: %d; rd: %d; imm: int %d uint %u",hart->pc,rs1(inst),rd(inst),(int64_t) imm_I(inst), imm_I(inst));
}
void exec_XORI(struct HART *hart, uint32_t inst) {
    hart->regs[rd(inst)] = hart->regs[rs1(inst)] ^ imm_I(inst);
	print_d(hart,"{0x%.8X} [XORI] rs1: %d; rd: %d; imm: int %d uint %u",hart->pc,rs1(inst),rd(inst),(int64_t) imm_I(inst), imm_I(inst));
}
void exec_ORI(struct HART *hart, uint32_t inst) {
    hart->regs[rd(inst)] = hart->regs[rs1(inst)] | imm_I(inst);
	print_d(hart,"{0x%.8X} [ORI] rs1: %d; rd: %d; imm: int %d uint %u",hart->pc,rs1(inst),rd(inst),(int64_t) imm_I(inst), imm_I(inst));
}
void exec_ANDI(struct HART *hart, uint32_t inst) {
    hart->regs[rd(inst)] = hart->regs[rs1(inst)] & imm_I(inst);
	print_d(hart,"{0x%.8X} [ANDI] rs1: %d; rd: %d; imm: int %d uint %u",hart->pc,rs1(inst),rd(inst),(int64_t) imm_I(inst), imm_I(inst));
}
void exec_SLLI(struct HART *hart, uint32_t inst) {
    hart->regs[rd(inst)] = hart->regs[rs1(inst)] << shamt64(inst);
	print_d(hart,"{0x%.8X} [SLLI] rs1: %d; rd: %d; imm: int %d uint %u",hart->pc,rs1(inst),rd(inst),(int64_t) imm_I(inst), imm_I(inst));
}
void exec_SRLI(struct HART *hart, uint32_t inst) {
    hart->regs[rd(inst)] = hart->regs[rs1(inst)] >> shamt64(inst);
	print_d(hart,"{0x%.8X} [SRLI] rs1: %d; rd: %d; imm: int %d uint %u",hart->pc,rs1(inst),rd(inst),(int64_t) imm_I(inst), imm_I(inst));
}
void exec_SRAI(struct HART *hart, uint32_t inst) {
    hart->regs[rd(inst)] = (int64_t)hart->regs[rs1(inst)] >> shamt64(inst);
	print_d(hart,"{0x%.8X} [SRAI] rs1: %d; rd: %d; imm: int %d uint %u",hart->pc,rs1(inst),rd(inst),(int64_t) imm_I(inst), imm_I(inst));
}
void exec_SLTI(struct HART *hart, uint32_t inst) {
    hart->regs[rd(inst)] = ((int64_t) hart->regs[rs1(inst)] < (int64_t) imm_I(inst)) ? 1 : 0;
	print_d(hart,"{0x%.8X} [SLTI] rs1: %d; rd: %d; imm: int %d uint %u",hart->pc,rs1(inst),rd(inst),(int64_t) imm_I(inst), imm_I(inst));
}
void exec_SLTIU(struct HART *hart, uint32_t inst) {
    hart->regs[rd(inst)] = (hart->regs[rs1(inst)] < imm_I(inst)) ? 1 : 0;
	print_d(hart,"{0x%.8X} [SLTIU] rs1: %d; rd: %d; imm: int %d uint %u",hart->pc,rs1(inst),rd(inst),(int64_t) imm_I(inst), imm_I(inst));
}
void exec_LB(struct HART *hart, uint32_t inst) {
	uint64_t imm = imm_I(inst);
	uint64_t addr = hart->regs[rs1(inst)] + (int64_t) imm;
	std::optional<uint64_t> val = hart->mmio->load(hart,addr,8);
	if(val.has_value()) {
		hart->regs[rd(inst)] = (int64_t)(int8_t) *val;
	}
	print_d(hart,"{0x%.8X} [LB] rs1: %d; rd: %d; imm: int %d uint %u",hart->pc,rs1(inst),rd(inst),(int64_t) imm_I(inst), imm_I(inst));
}
void exec_LH(struct HART *hart, uint32_t inst) {
	uint64_t imm = imm_I(inst);
	uint64_t addr = hart->regs[rs1(inst)] + (int64_t) imm;
	std::optional<uint64_t> val = hart->mmio->load(hart,addr,16);
	if(val.has_value()) {
		hart->regs[rd(inst)] = (int64_t)(int16_t) *val;
	}
	print_d(hart,"{0x%.8X} [LH] rs1: %d; rd: %d; imm: int %d uint %u",hart->pc,rs1(inst),rd(inst),(int64_t) imm_I(inst), imm_I(inst));
}
void exec_LW(struct HART *hart, uint32_t inst) {
	uint64_t imm = imm_I(inst);
	uint64_t addr = hart->regs[rs1(inst)] + (int64_t) imm;
	std::optional<uint64_t> val = hart->mmio->load(hart,addr,32);
	if(val.has_value()) {
		hart->regs[rd(inst)] = (int64_t)(int32_t) *val;
	}
	print_d(hart,"{0x%.8X} [LW] rs1: %d; rd: %d; imm: int %d uint %u",hart->pc,rs1(inst),rd(inst),(int64_t) imm_I(inst), imm_I(inst));
}
void exec_LBU(struct HART *hart, uint32_t inst) {
	uint64_t imm = imm_I(inst);
	uint64_t addr = hart->regs[rs1(inst)] + (int64_t) imm;
	std::optional<uint64_t> val = hart->mmio->load(hart,addr,8);
	if(val.has_value()) {
		hart->regs[rd(inst)] = (uint8_t) *val;
	}
	print_d(hart,"{0x%.8X} [LBU] rs1: %d; rd: %d; imm: int %d uint %u",hart->pc,rs1(inst),rd(inst),(int64_t) imm_I(inst), imm_I(inst));
}
void exec_LHU(struct HART *hart, uint32_t inst) {
	uint64_t imm = imm_I(inst);
	uint64_t addr = hart->regs[rs1(inst)] + (int64_t) imm;
	std::optional<uint64_t> val = hart->mmio->load(hart,addr,16);
	if(val.has_value()) {
		hart->regs[rd(inst)] = (uint16_t) *val;
	}
	print_d(hart,"{0x%.8X} [LHU] rs1: %d; rd: %d; imm: int %d uint %u",hart->pc,rs1(inst),rd(inst),(int64_t) imm_I(inst), imm_I(inst));
}

// S-Type
void exec_SB(struct HART *hart, uint32_t inst) {
	uint64_t imm = imm_S(inst);
	uint64_t addr = hart->regs[rs1(inst)] + (int64_t) imm;
	hart->mmio->store(hart,addr,8,hart->regs[rs2(inst)]);
	print_d(hart,"{0x%.8X} [SB] rs1: %d; rs2: %d; imm: int %d uint %u",hart->pc,rs1(inst),rs2(inst),(int64_t) imm_S(inst), imm_S(inst));
}
void exec_SH(struct HART *hart, uint32_t inst) {
	uint64_t imm = imm_S(inst);
	uint64_t addr = hart->regs[rs1(inst)] + (int64_t) imm;
	hart->mmio->store(hart,addr,16,hart->regs[rs2(inst)]);
	print_d(hart,"{0x%.8X} [SH] rs1: %d; rs2: %d; imm: int %d uint %u",hart->pc,rs1(inst),rs2(inst),(int64_t) imm_S(inst), imm_S(inst));
}
void exec_SW(struct HART *hart, uint32_t inst) {
	uint64_t imm = imm_S(inst);
	uint64_t addr = hart->regs[rs1(inst)] + (int64_t) imm;
	hart->mmio->store(hart,addr,32,hart->regs[rs2(inst)]);
	print_d(hart,"{0x%.8X} [SW] rs1: %d; rs2: %d; imm: int %d uint %u",hart->pc,rs1(inst),rs2(inst),(int64_t) imm_S(inst), imm_S(inst));
}

// B-Type
void exec_BEQ(struct HART *hart, uint32_t inst) {
	uint64_t imm = imm_B(inst);
	if ((int64_t) hart->regs[rs1(inst)] == (int64_t) hart->regs[rs2(inst)])
		hart->pc = hart->pc + (int64_t) imm - 4;
	print_d(hart,"{0x%.8X} [BEQ] rs1: %d; rs2: %d; imm: int %d uint %u",hart->pc,rs1(inst),rs2(inst),(int64_t) imm_B(inst), imm_B(inst));
}
void exec_BNE(struct HART *hart, uint32_t inst) {
	uint64_t imm = imm_B(inst);
	if ((int64_t) hart->regs[rs1(inst)] != (int64_t) hart->regs[rs2(inst)])
		hart->pc = hart->pc + (int64_t) imm - 4;
	print_d(hart,"{0x%.8X} [BNE] rs1: %d; rs2: %d; imm: int %d uint %u",hart->pc,rs1(inst),rs2(inst),(int64_t) imm_B(inst), imm_B(inst));
}
void exec_BLT(struct HART *hart, uint32_t inst) {
	uint64_t imm = imm_B(inst);
	if ((int64_t) hart->regs[rs1(inst)] < (int64_t) hart->regs[rs2(inst)])
		hart->pc = hart->pc + (int64_t) imm - 4;
	print_d(hart,"{0x%.8X} [BLT] rs1: %d; rs2: %d; imm: int %d uint %u",hart->pc,rs1(inst),rs2(inst),(int64_t) imm_B(inst), imm_B(inst));
}
void exec_BGE(struct HART *hart, uint32_t inst) {
	uint64_t imm = imm_B(inst);
	if ((int64_t) hart->regs[rs1(inst)] >= (int64_t) hart->regs[rs2(inst)])
		hart->pc = hart->pc + (int64_t) imm - 4;
	print_d(hart,"{0x%.8X} [BGE] rs1: %d; rs2: %d; imm: int %d uint %u",hart->pc,rs1(inst),rs2(inst),(int64_t) imm_B(inst), imm_B(inst));
}
void exec_BLTU(struct HART *hart, uint32_t inst) {
	uint64_t imm = imm_B(inst);
	if (hart->regs[rs1(inst)] < hart->regs[rs2(inst)])
		hart->pc = hart->pc + (int64_t) imm - 4;
	print_d(hart,"{0x%.8X} [BLTU] rs1: %d; rs2: %d; imm: int %d uint %u",hart->pc,rs1(inst),rs2(inst),(int64_t) imm_B(inst), imm_B(inst));
}
void exec_BGEU(struct HART *hart, uint32_t inst) {
	uint64_t imm = imm_B(inst);
	if (hart->regs[rs1(inst)] >= hart->regs[rs2(inst)])
		hart->pc = hart->pc + (int64_t) imm - 4;
	print_d(hart,"{0x%.8X} [BGEU] rs1: %d; rs2: %d; imm: int %d uint %u",hart->pc,rs1(inst),rs2(inst),(int64_t) imm_B(inst), imm_B(inst));
}

// JUMP
void exec_JAL(struct HART *hart, uint32_t inst) {
	uint64_t imm = imm_J(inst);
	hart->regs[rd(inst)] = hart->pc+4;
	hart->pc = hart->pc + (int64_t) imm;
	print_d(hart,"{0x%.8X} [JAL] rd: %d; imm: int %d uint %u",hart->pc,rd(inst),(int64_t) imm_J(inst), imm_J(inst));
	/*if(ADDR_MISALIGNED(hart->pc)) {
		fprintf(stderr, "JAL PC Address misalligned");
		exit(0);
	}*/
}
void exec_JALR(struct HART *hart, uint32_t inst) {
	uint64_t imm = imm_I(inst);
	uint64_t tmp = hart->pc;
	hart->pc = (hart->regs[rs1(inst)] + (int64_t) imm);
	hart->regs[rd(inst)] = tmp+4;
	print_d(hart,"{0x%.8X} [JALR] rs1: %d; rd: %d; imm: int %d uint %u",hart->pc,rs1(inst),rd(inst),(int64_t) imm_I(inst), imm_I(inst));
	/*if(ADDR_MISALIGNED(hart->pc)) {
		fprintf(stderr, "JAL PC Address misalligned");
		exit(0);
	}*/
}

// what

void exec_LUI(struct HART *hart, uint32_t inst) {
	uint64_t imm = imm_U(inst);
	hart->regs[rd(inst)] = (int64_t) (int32_t) (imm << 12);
	print_d(hart,"{0x%.8X} [LUI] rd: %d; imm: int %d uint %u",hart->pc,rd(inst),(int64_t) imm_U(inst), imm_U(inst));
}
void exec_AUIPC(struct HART *hart, uint32_t inst) {
	uint64_t imm = imm_U(inst);
	hart->regs[rd(inst)] = hart->pc + ((int64_t) (int32_t) (imm << 12));
	print_d(hart,"{0x%.8X} [AUIPC] rd: %d; imm: int %d uint %u",hart->pc,rd(inst),(int64_t) imm_U(inst), imm_U(inst));
}

void exec_ECALL(struct HART *hart, uint32_t inst) {
	switch(hart->mode) {
		case 3: cpu_trap(hart,EXC_ENV_CALL_FROM_M,hart->pc,false); break;
		case 1: cpu_trap(hart,EXC_ENV_CALL_FROM_S,hart->pc,false); break;
		case 0: cpu_trap(hart,EXC_ENV_CALL_FROM_U,hart->pc,false); break;
	}
}
void exec_EBREAK(struct HART *hart, uint32_t inst) {
	cpu_trap(hart,EXC_BREAKPOINT,hart->pc,false);
}