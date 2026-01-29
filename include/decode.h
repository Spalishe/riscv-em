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

#pragma once

struct HART;

struct inst_data {
    bool valid;
    uint64_t pc;
    uint32_t inst;

    uint8_t rd;
    uint8_t rs1;
    uint8_t rs2;
    int64_t imm;
    void (*fn)(HART*, inst_data&);
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
void exec_ADD(HART *hart, inst_data& inst);
void exec_SUB(HART *hart, inst_data& inst);
void exec_XOR(HART *hart, inst_data& inst);
void exec_OR(HART *hart, inst_data& inst);
void exec_AND(HART *hart, inst_data& inst);
void exec_SLL(HART *hart, inst_data& inst);
void exec_SRL(HART *hart, inst_data& inst);
void exec_SRA(HART *hart, inst_data& inst);
void exec_SLT(HART *hart, inst_data& inst);
void exec_SLTU(HART *hart, inst_data& inst);

void exec_ADDW(HART *hart, inst_data& inst);
void exec_SUBW(HART *hart, inst_data& inst);
void exec_SLLW(HART *hart, inst_data& inst);
void exec_SRLW(HART *hart, inst_data& inst);
void exec_SRAW(HART *hart, inst_data& inst);

// I-Type
void exec_ADDI(HART *hart, inst_data& inst);
void exec_XORI(HART *hart, inst_data& inst);
void exec_ORI(HART *hart, inst_data& inst);
void exec_ANDI(HART *hart, inst_data& inst);
void exec_SLLI(HART *hart, inst_data& inst);
void exec_SRLI(HART *hart, inst_data& inst);
void exec_SRAI(HART *hart, inst_data& inst);
void exec_SLTI(HART *hart, inst_data& inst);
void exec_SLTIU(HART *hart, inst_data& inst);
void exec_LB(HART *hart, inst_data& inst);
void exec_LH(HART *hart, inst_data& inst);
void exec_LW(HART *hart, inst_data& inst);
void exec_LBU(HART *hart, inst_data& inst);
void exec_LHU(HART *hart, inst_data& inst);

void exec_ADDIW(HART *hart, inst_data& inst);
void exec_SLLIW(HART *hart, inst_data& inst);
void exec_SRLIW(HART *hart, inst_data& inst);
void exec_SRAIW(HART *hart, inst_data& inst);

void exec_LWU(HART *hart, inst_data& inst);
void exec_LD(HART *hart, inst_data& inst);

// S-Type
void exec_SB(HART *hart, inst_data& inst);
void exec_SH(HART *hart, inst_data& inst);
void exec_SW(HART *hart, inst_data& inst);
void exec_SD(HART *hart, inst_data& inst);

// B-Type
void exec_BEQ(HART *hart, inst_data& inst);
void exec_BNE(HART *hart, inst_data& inst);
void exec_BLT(HART *hart, inst_data& inst);
void exec_BGE(HART *hart, inst_data& inst);
void exec_BLTU(HART *hart, inst_data& inst);
void exec_BGEU(HART *hart, inst_data& inst);

// JUMP
void exec_JAL(HART *hart, inst_data& inst);
void exec_JALR(HART *hart, inst_data& inst);

// what
void exec_LUI(HART *hart, inst_data& inst);
void exec_AUIPC(HART *hart, inst_data& inst);

void exec_FENCE(HART *hart, inst_data& inst);

void exec_ECALL(HART *hart, inst_data& inst);
void exec_EBREAK(HART *hart, inst_data& inst);

//// ZiCSR

void exec_CSRRW(HART *hart, inst_data& inst);
void exec_CSRRS(HART *hart, inst_data& inst);
void exec_CSRRC(HART *hart, inst_data& inst);
void exec_CSRRWI(HART *hart, inst_data& inst);
void exec_CSRRSI(HART *hart, inst_data& inst);
void exec_CSRRCI(HART *hart, inst_data& inst);

//// SYSTEM

void exec_MRET(HART *hart, inst_data& inst);
void exec_SRET(HART *hart, inst_data& inst);