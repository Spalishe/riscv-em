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

#include <cstdint>
#pragma once

struct HART;

struct inst_data {
    bool valid;
    uint64_t pc;
    uint32_t inst;

    uint8_t rd;
    uint8_t rs1;
    uint8_t rs2;
    uint64_t imm;
    bool (*fn)(HART*, inst_data&);
};

uint64_t d_rd(uint32_t inst);
uint64_t d_rs1(uint32_t inst);
uint64_t d_rs2(uint32_t inst);
uint64_t imm_Zicsr(uint32_t inst);
uint64_t imm_I(uint32_t inst);
uint64_t imm_S(uint32_t inst);
uint64_t imm_B(uint32_t inst);
uint64_t imm_U(uint32_t inst);
uint64_t imm_J(uint32_t inst);
uint64_t C_imm_J(uint16_t inst);
uint32_t shamt(uint32_t inst);
uint32_t shamt64(uint32_t inst);
int32_t sext(uint32_t val, int bits);
uint32_t get_bits(uint32_t inst, int hi, int lo);
uint32_t switch_endian(uint32_t const input);

extern inst_data parse_instruction(HART* hart, uint32_t inst, uint64_t pc);

#define R_TYPE  0x33
    #define ADDSUB  0x0
	#define ADD	0x00
        #define SUB     0x20
    #define XOR     0x4
    #define OR      0x6
    #define AND     0x7
    #define SLL     0x1
    #define SR      0x5
	#define SRL	0x00
	#define SRA     0x20
    #define SLT     0x2
    #define SLTU    0x3
#define R_TYPE64 0x3B
#define I_TYPE  0x13
    #define ADDI    0x0
    #define SLLI    0x1
    #define SLTI    0x2
    #define SLTIU   0x3
    #define XORI    0x4
    #define SRI     0x5
        #define SRLI    0x00
        #define SRAI    0x10
        #define SRAIW    0x20
    #define ORI     0x6
    #define ANDI    0x7
#define I_TYPE64  0x1B
#define LOAD_TYPE 0x3
	#define LB		0x0
	#define LH		0x1
	#define LW		0x2
	#define LD		0x3
	#define LBU		0x4
	#define LHU		0x5
	#define LWU		0x6
#define S_TYPE    0x23
	#define SB		0x0
	#define SH		0x1
	#define SW		0x2
	#define SD		0x3
#define B_TYPE    0x63
	#define BEQ		0x0
	#define BNE		0x1
	#define BLT     0x4
	#define BGE     0x5
	#define BLTU    0x6
	#define BGEU    0x7
#define JAL 	  0x6F
#define JALR 	  0x67
#define LUI       0x37
#define AUIPC     0x17
#define ECALL     0x73
    #define CSRRW 0x1
    #define CSRRS 0x2
    #define CSRRC 0x3
    #define CSRRWI 0x5
    #define CSRRSI 0x6
    #define CSRRCI 0x7

#define AMO       0x2F
    #define AMO_W 0x2
    #define AMO_D 0x3

    #define AMOADD    0x0
    #define AMOSWAP    0x1
    #define LR    0x2
    #define SC    0x3
    #define AMOXOR    0x4
    #define AMOOR     0x8
    #define AMOAND    0xC
    #define AMOMIN    0x10
    #define AMOMAX    0x14
    #define AMOMINU   0x18
    #define AMOMAXU   0x1C

#define FENCE     0xF

//// RV64I
// R-Type
bool exec_ADD(HART *hart, inst_data& inst);
bool exec_SUB(HART *hart, inst_data& inst);
bool exec_XOR(HART *hart, inst_data& inst);
bool exec_OR(HART *hart, inst_data& inst);
bool exec_AND(HART *hart, inst_data& inst);
bool exec_SLL(HART *hart, inst_data& inst);
bool exec_SRL(HART *hart, inst_data& inst);
bool exec_SRA(HART *hart, inst_data& inst);
bool exec_SLT(HART *hart, inst_data& inst);
bool exec_SLTU(HART *hart, inst_data& inst);

bool exec_ADDW(HART *hart, inst_data& inst);
bool exec_SUBW(HART *hart, inst_data& inst);
bool exec_SLLW(HART *hart, inst_data& inst);
bool exec_SRLW(HART *hart, inst_data& inst);
bool exec_SRAW(HART *hart, inst_data& inst);

// I-Type
bool exec_ADDI(HART *hart, inst_data& inst);
bool exec_XORI(HART *hart, inst_data& inst);
bool exec_ORI(HART *hart, inst_data& inst);
bool exec_ANDI(HART *hart, inst_data& inst);
bool exec_SLLI(HART *hart, inst_data& inst);
bool exec_SRLI(HART *hart, inst_data& inst);
bool exec_SRAI(HART *hart, inst_data& inst);
bool exec_SLTI(HART *hart, inst_data& inst);
bool exec_SLTIU(HART *hart, inst_data& inst);
bool exec_LB(HART *hart, inst_data& inst);
bool exec_LH(HART *hart, inst_data& inst);
bool exec_LW(HART *hart, inst_data& inst);
bool exec_LBU(HART *hart, inst_data& inst);
bool exec_LHU(HART *hart, inst_data& inst);

bool exec_ADDIW(HART *hart, inst_data& inst);
bool exec_SLLIW(HART *hart, inst_data& inst);
bool exec_SRLIW(HART *hart, inst_data& inst);
bool exec_SRAIW(HART *hart, inst_data& inst);

bool exec_LWU(HART *hart, inst_data& inst);
bool exec_LD(HART *hart, inst_data& inst);

// S-Type
bool exec_SB(HART *hart, inst_data& inst);
bool exec_SH(HART *hart, inst_data& inst);
bool exec_SW(HART *hart, inst_data& inst);
bool exec_SD(HART *hart, inst_data& inst);

// B-Type
bool exec_BEQ(HART *hart, inst_data& inst);
bool exec_BNE(HART *hart, inst_data& inst);
bool exec_BLT(HART *hart, inst_data& inst);
bool exec_BGE(HART *hart, inst_data& inst);
bool exec_BLTU(HART *hart, inst_data& inst);
bool exec_BGEU(HART *hart, inst_data& inst);

// JUMP
bool exec_JAL(HART *hart, inst_data& inst);
bool exec_JALR(HART *hart, inst_data& inst);

// what
bool exec_LUI(HART *hart, inst_data& inst);
bool exec_AUIPC(HART *hart, inst_data& inst);

bool exec_FENCE(HART *hart, inst_data& inst);

bool exec_ECALL(HART *hart, inst_data& inst);
bool exec_EBREAK(HART *hart, inst_data& inst);

//// RV64M
// R-Type
bool exec_MUL(HART *hart, inst_data& inst);
bool exec_MULH(HART *hart, inst_data& inst);
bool exec_MULHSU(HART *hart, inst_data& inst);
bool exec_MULHU(HART *hart, inst_data& inst);
bool exec_DIV(HART *hart, inst_data& inst);
bool exec_DIVU(HART *hart, inst_data& inst);
bool exec_REM(HART *hart, inst_data& inst);
bool exec_REMU(HART *hart, inst_data& inst);

bool exec_MULW(HART *hart, inst_data& inst);
bool exec_DIVW(HART *hart, inst_data& inst);
bool exec_DIVUW(HART *hart, inst_data& inst);
bool exec_REMW(HART *hart, inst_data& inst);
bool exec_REMUW(HART *hart, inst_data& inst);

//// ZiCSR

bool exec_CSRRW(HART *hart, inst_data& inst);
bool exec_CSRRS(HART *hart, inst_data& inst);
bool exec_CSRRC(HART *hart, inst_data& inst);
bool exec_CSRRWI(HART *hart, inst_data& inst);
bool exec_CSRRSI(HART *hart, inst_data& inst);
bool exec_CSRRCI(HART *hart, inst_data& inst);

//// SYSTEM

bool exec_MRET(HART *hart, inst_data& inst);
bool exec_SRET(HART *hart, inst_data& inst);
bool exec_SFENCE_VMA(HART *hart, inst_data& inst);

// Zbb

bool exec_ANDN(HART *hart, inst_data& inst);
bool exec_ORN(HART *hart, inst_data& inst);
bool exec_XNOR(HART *hart, inst_data& inst);
bool exec_CLZ(HART *hart, inst_data& inst);
bool exec_CLZW(HART *hart, inst_data& inst);
bool exec_CTZ(HART *hart, inst_data& inst);
bool exec_CTZW(HART *hart, inst_data& inst);
bool exec_CPOP(HART *hart, inst_data& inst);
bool exec_CPOPW(HART *hart, inst_data& inst);
bool exec_MAX(HART *hart, inst_data& inst);
bool exec_MAXU(HART *hart, inst_data& inst);
bool exec_MIN(HART *hart, inst_data& inst);
bool exec_MINU(HART *hart, inst_data& inst);
bool exec_SEXT_B(HART *hart, inst_data& inst);
bool exec_SEXT_H(HART *hart, inst_data& inst);
bool exec_ZEXT_H(HART *hart, inst_data& inst);
bool exec_ROL(HART *hart, inst_data& inst);
bool exec_ROLW(HART *hart, inst_data& inst);
bool exec_ROR(HART *hart, inst_data& inst);
bool exec_RORI(HART *hart, inst_data& inst);
bool exec_RORIW(HART *hart, inst_data& inst);
bool exec_RORW(HART *hart, inst_data& inst);
bool exec_ORC_B(HART *hart, inst_data& inst);
bool exec_REV8(HART *hart, inst_data& inst);

// Zba

bool exec_ADD_UW(HART *hart, inst_data& inst);
bool exec_SH1ADD(HART *hart, inst_data& inst);
bool exec_SH1ADD_UW(HART *hart, inst_data& inst);
bool exec_SH2ADD(HART *hart, inst_data& inst);
bool exec_SH2ADD_UW(HART *hart, inst_data& inst);
bool exec_SH3ADD(HART *hart, inst_data& inst);
bool exec_SH3ADD_UW(HART *hart, inst_data& inst);
bool exec_SLLI_UW(HART *hart, inst_data& inst);

// Zbc

bool exec_CLMUL(HART *hart, inst_data& inst);
bool exec_CLMULH(HART *hart, inst_data& inst);
bool exec_CLMULR(HART *hart, inst_data& inst);

// Zbs

bool exec_BCLR(HART *hart, inst_data& inst);
bool exec_BCLRI(HART *hart, inst_data& inst);
bool exec_BEXT(HART *hart, inst_data& inst);
bool exec_BEXTI(HART *hart, inst_data& inst);
bool exec_BINV(HART *hart, inst_data& inst);
bool exec_BINVI(HART *hart, inst_data& inst);
bool exec_BSET(HART *hart, inst_data& inst);
bool exec_BSETI(HART *hart, inst_data& inst);