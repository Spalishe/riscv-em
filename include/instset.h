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

#pragma once
#include <stdint.h>

// Decoder
uint64_t rd(uint32_t inst);
uint64_t rs1(uint32_t inst);
uint64_t rs2(uint32_t inst);
uint64_t imm_Zicsr(uint32_t inst);
uint64_t imm_I(uint32_t inst);
uint64_t imm_S(uint32_t inst);
uint64_t imm_B(uint32_t inst);
uint64_t imm_U(uint32_t inst);
uint64_t imm_J(uint32_t inst);
uint32_t shamt(uint32_t inst);
uint32_t shamt64(uint32_t inst);
int32_t sext(uint32_t val, int bits);
uint32_t get_bits(uint32_t inst, int hi, int lo);
uint32_t switch_endian(uint32_t const input);

//// RV32I
// R-Type
void exec_ADD(struct HART *hart, uint32_t inst);
void exec_SUB(struct HART *hart, uint32_t inst);
void exec_XOR(struct HART *hart, uint32_t inst);
void exec_OR(struct HART *hart, uint32_t inst);
void exec_AND(struct HART *hart, uint32_t inst);
void exec_SLL(struct HART *hart, uint32_t inst);
void exec_SRL(struct HART *hart, uint32_t inst);
void exec_SRA(struct HART *hart, uint32_t inst);
void exec_SLT(struct HART *hart, uint32_t inst);
void exec_SLTU(struct HART *hart, uint32_t inst);

// I-Type
void exec_ADDI(struct HART *hart, uint32_t inst);
void exec_XORI(struct HART *hart, uint32_t inst);
void exec_ORI(struct HART *hart, uint32_t inst);
void exec_ANDI(struct HART *hart, uint32_t inst);
void exec_SLLI(struct HART *hart, uint32_t inst);
void exec_SRLI(struct HART *hart, uint32_t inst);
void exec_SRAI(struct HART *hart, uint32_t inst);
void exec_SLTI(struct HART *hart, uint32_t inst);
void exec_SLTIU(struct HART *hart, uint32_t inst);
void exec_LB(struct HART *hart, uint32_t inst);
void exec_LH(struct HART *hart, uint32_t inst);
void exec_LW(struct HART *hart, uint32_t inst);
void exec_LBU(struct HART *hart, uint32_t inst);
void exec_LHU(struct HART *hart, uint32_t inst);

// S-Type
void exec_SB(struct HART *hart, uint32_t inst);
void exec_SH(struct HART *hart, uint32_t inst);
void exec_SW(struct HART *hart, uint32_t inst);

// B-Type
void exec_BEQ(struct HART *hart, uint32_t inst);
void exec_BNE(struct HART *hart, uint32_t inst);
void exec_BLT(struct HART *hart, uint32_t inst);
void exec_BGE(struct HART *hart, uint32_t inst);
void exec_BLTU(struct HART *hart, uint32_t inst);
void exec_BGEU(struct HART *hart, uint32_t inst);

// JUMP
void exec_JAL(struct HART *hart, uint32_t inst);
void exec_JALR(struct HART *hart, uint32_t inst);

// what
void exec_LUI(struct HART *hart, uint32_t inst);
void exec_AUIPC(struct HART *hart, uint32_t inst);

void exec_ECALL(struct HART *hart, uint32_t inst);
void exec_EBREAK(struct HART *hart, uint32_t inst);


//// RV64I
// R-Type
void exec_ADDW(struct HART *hart, uint32_t inst);
void exec_SUBW(struct HART *hart, uint32_t inst);
void exec_SLLW(struct HART *hart, uint32_t inst);
void exec_SRLW(struct HART *hart, uint32_t inst);
void exec_SRAW(struct HART *hart, uint32_t inst);

// I-Type
void exec_ADDIW(struct HART *hart, uint32_t inst);
void exec_SLLIW(struct HART *hart, uint32_t inst);
void exec_SRLIW(struct HART *hart, uint32_t inst);
void exec_SRAIW(struct HART *hart, uint32_t inst);

void exec_LWU(struct HART *hart, uint32_t inst);
void exec_LD(struct HART *hart, uint32_t inst);

// S-Type
void exec_SD(struct HART *hart, uint32_t inst);


//// ZICSR

void exec_CSRRW(struct HART *hart, uint32_t inst);
void exec_CSRRS(struct HART *hart, uint32_t inst);
void exec_CSRRC(struct HART *hart, uint32_t inst);
void exec_CSRRWI(struct HART *hart, uint32_t inst);
void exec_CSRRSI(struct HART *hart, uint32_t inst);
void exec_CSRRCI(struct HART *hart, uint32_t inst);


//// RV32M
// R-Type
void exec_MUL(struct HART *hart, uint32_t inst);
void exec_MULH(struct HART *hart, uint32_t inst);
void exec_MULHSU(struct HART *hart, uint32_t inst);
void exec_MULHU(struct HART *hart, uint32_t inst);
void exec_DIV(struct HART *hart, uint32_t inst);
void exec_DIVU(struct HART *hart, uint32_t inst);
void exec_REM(struct HART *hart, uint32_t inst);
void exec_REMU(struct HART *hart, uint32_t inst);


//// RV64M
// R-Type
void exec_MULW(struct HART *hart, uint32_t inst);
void exec_DIVW(struct HART *hart, uint32_t inst);
void exec_DIVUW(struct HART *hart, uint32_t inst);
void exec_REMW(struct HART *hart, uint32_t inst);
void exec_REMUW(struct HART *hart, uint32_t inst);


//// RV32C

// CI-Type

void exec_C_LWSP(struct HART *hart, uint32_t inst);
void exec_C_LI(struct HART *hart, uint32_t inst);
void exec_C_LUI(struct HART *hart, uint32_t inst);
void exec_C_ADDI(struct HART *hart, uint32_t inst);
void exec_C_ADDI16SP(struct HART *hart, uint32_t inst);
void exec_C_SLLI(struct HART *hart, uint32_t inst);
void exec_C_NOP(struct HART *hart, uint32_t inst);

// CSS-Type / CIW-Type

void exec_C_SWSP(struct HART *hart, uint32_t inst);
void exec_C_ADDI4SPN(struct HART *hart, uint32_t inst);

// CL-Type

void exec_C_LW(struct HART *hart, uint32_t inst);

// CS-Type

void exec_C_SW(struct HART *hart, uint32_t inst);
void exec_C_AND(struct HART *hart, uint32_t inst);
void exec_C_OR(struct HART *hart, uint32_t inst);
void exec_C_XOR(struct HART *hart, uint32_t inst);
void exec_C_SUB(struct HART *hart, uint32_t inst);

// CJ-Type

void exec_C_J(struct HART *hart, uint32_t inst);
void exec_C_JAL(struct HART *hart, uint32_t inst);

// CR-Type

void exec_C_JR(struct HART *hart, uint32_t inst);
void exec_C_JALR(struct HART *hart, uint32_t inst);
void exec_C_MV(struct HART *hart, uint32_t inst);
void exec_C_ADD(struct HART *hart, uint32_t inst);
void exec_C_EBREAK(struct HART *hart, uint32_t inst);

// CB-Type

void exec_C_BEQZ(struct HART *hart, uint32_t inst);
void exec_C_BNEZ(struct HART *hart, uint32_t inst);
void exec_C_SRLI(struct HART *hart, uint32_t inst);
void exec_C_SRAI(struct HART *hart, uint32_t inst);
void exec_C_ANDI(struct HART *hart, uint32_t inst);


//// RV64C

// CI-Type

void exec_C_LDSP(struct HART *hart, uint32_t inst);
void exec_C_ADDIW(struct HART *hart, uint32_t inst);

// CSS-Type / CIW-Type

void exec_C_SDSP(struct HART *hart, uint32_t inst);

// CL-Type

void exec_C_LD(struct HART *hart, uint32_t inst);

// CS-Type

void exec_C_SD(struct HART *hart, uint32_t inst);
void exec_C_SUBW(struct HART *hart, uint32_t inst);

// CR-Type

void exec_C_ADDW(struct HART *hart, uint32_t inst);



//// RV32A

// R-Type

void exec_LR_W(struct HART *hart, uint32_t inst);
void exec_SC_W(struct HART *hart, uint32_t inst);

void exec_AMOSWAP_W(struct HART *hart, uint32_t inst);
void exec_AMOADD_W(struct HART *hart, uint32_t inst);
void exec_AMOXOR_W(struct HART *hart, uint32_t inst);
void exec_AMOAND_W(struct HART *hart, uint32_t inst);
void exec_AMOOR_W(struct HART *hart, uint32_t inst);

void exec_AMOMIN_W(struct HART *hart, uint32_t inst);
void exec_AMOMAX_W(struct HART *hart, uint32_t inst);
void exec_AMOMINU_W(struct HART *hart, uint32_t inst);
void exec_AMOMAXU_W(struct HART *hart, uint32_t inst);


//// RV64A

// R-Type

void exec_LR_D(struct HART *hart, uint32_t inst);
void exec_SC_D(struct HART *hart, uint32_t inst);

void exec_AMOSWAP_D(struct HART *hart, uint32_t inst);
void exec_AMOADD_D(struct HART *hart, uint32_t inst);
void exec_AMOXOR_D(struct HART *hart, uint32_t inst);
void exec_AMOAND_D(struct HART *hart, uint32_t inst);
void exec_AMOOR_D(struct HART *hart, uint32_t inst);

void exec_AMOMIN_D(struct HART *hart, uint32_t inst);
void exec_AMOMAX_D(struct HART *hart, uint32_t inst);
void exec_AMOMINU_D(struct HART *hart, uint32_t inst);
void exec_AMOMAXU_D(struct HART *hart, uint32_t inst);



//// Priv

void exec_WFI(struct HART *hart, uint32_t inst);
void exec_SRET(struct HART *hart, uint32_t inst);
void exec_MRET(struct HART *hart, uint32_t inst);



//// Zifencei
void exec_FENCE_I(struct HART *hart, uint32_t inst);
void exec_SFENCE_VMA(struct HART *hart, uint32_t inst);
void exec_FENCE(struct HART *hart, uint32_t inst);