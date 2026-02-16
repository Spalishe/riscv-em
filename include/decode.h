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
struct Machine;

struct inst_ret {
    bool increasePC;
    bool isCompressed = false; // RVC
    inst_ret(bool increase) {
        increasePC = increase;
    }
};

#define F_QNAN 0x7fc00000
#define F_SNAN 0x7fa00000
#define F_QNAN64 0x7ff8000000000000
#define F_SNAN64 0x7ffc000000000000

enum class RoundingMode {
    RNE = 0b000, // Round to Nearest, ties to Even
    RTZ = 0b001, // Round towards Zero
    RDN = 0b010, // Round Down (towards -inf)
    RUP = 0b011, // Round Up (towards +inf)
    RMM = 0b100, // Round to Nearest, ties to Max Magnitude
    DYN = 0b111, // In instruction's rm field, selects dynamic rounding mode; In Rounding Mode register;
};

enum class FMT {
    S = 0b00, // 32-bit single-precision
    D = 0b01, // 64-bit double-precision
    H = 0b10, // 16-bit half-precision
    Q = 0b11, // 128-bit quad-precision
};

struct inst_data {
    bool valid;
    bool canChangePC = false;
    uint64_t pc;
    uint32_t inst;

    uint8_t rd;
    uint8_t rs1;
    uint8_t rs2;
    uint64_t imm;
    inst_ret (*fn)(HART*, inst_data&);

    // RVF
    RoundingMode rm;
    FMT fmt;
    uint8_t rs3;
};

uint64_t d_rd(uint32_t inst);
uint64_t d_rs1(uint32_t inst);
uint64_t d_rs2(uint32_t inst);
uint64_t d_rs3(uint32_t inst);
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

#ifdef USE_FPU
#define R_F       0x53
    #define FADD 0x0
    #define FSUB 0x1
    #define FMUL 0x2
    #define FDIV 0x3
    #define FMIN_MAX 0x5
        #define FMIN 0
        #define FMAX 1
    #define FSQRT 0xB
    #define FCVT_X_T 0x18
    #define FCVT_T_X 0x1A
    #define FSGN 0x4
        #define FSGNJ 0x0
        #define FSGNJN 0x1
        #define FSGNJX 0x2
    #define FMV_X_T 0x1C
        #define FCLASS 0x1
    #define FMV_T_X 0x1E
    #define FBR     0x14
        #define FLE 0x0
        #define FLT 0x1
        #define FEQ 0x2
    
#define FMADD     0x43
#define FMSUB     0x47
#define FNMADD    0x4F
#define FNMSUB    0x4B
#define FLOAD     0x7
#define FSTORE    0x27
#endif

//// RV64I
// R-Type
inst_ret exec_ADD(HART *hart, inst_data& inst);
inst_ret exec_SUB(HART *hart, inst_data& inst);
inst_ret exec_XOR(HART *hart, inst_data& inst);
inst_ret exec_OR(HART *hart, inst_data& inst);
inst_ret exec_AND(HART *hart, inst_data& inst);
inst_ret exec_SLL(HART *hart, inst_data& inst);
inst_ret exec_SRL(HART *hart, inst_data& inst);
inst_ret exec_SRA(HART *hart, inst_data& inst);
inst_ret exec_SLT(HART *hart, inst_data& inst);
inst_ret exec_SLTU(HART *hart, inst_data& inst);

inst_ret exec_ADDW(HART *hart, inst_data& inst);
inst_ret exec_SUBW(HART *hart, inst_data& inst);
inst_ret exec_SLLW(HART *hart, inst_data& inst);
inst_ret exec_SRLW(HART *hart, inst_data& inst);
inst_ret exec_SRAW(HART *hart, inst_data& inst);

// I-Type
inst_ret exec_ADDI(HART *hart, inst_data& inst);
inst_ret exec_XORI(HART *hart, inst_data& inst);
inst_ret exec_ORI(HART *hart, inst_data& inst);
inst_ret exec_ANDI(HART *hart, inst_data& inst);
inst_ret exec_SLLI(HART *hart, inst_data& inst);
inst_ret exec_SRLI(HART *hart, inst_data& inst);
inst_ret exec_SRAI(HART *hart, inst_data& inst);
inst_ret exec_SLTI(HART *hart, inst_data& inst);
inst_ret exec_SLTIU(HART *hart, inst_data& inst);
inst_ret exec_LB(HART *hart, inst_data& inst);
inst_ret exec_LH(HART *hart, inst_data& inst);
inst_ret exec_LW(HART *hart, inst_data& inst);
inst_ret exec_LBU(HART *hart, inst_data& inst);
inst_ret exec_LHU(HART *hart, inst_data& inst);

inst_ret exec_ADDIW(HART *hart, inst_data& inst);
inst_ret exec_SLLIW(HART *hart, inst_data& inst);
inst_ret exec_SRLIW(HART *hart, inst_data& inst);
inst_ret exec_SRAIW(HART *hart, inst_data& inst);

inst_ret exec_LWU(HART *hart, inst_data& inst);
inst_ret exec_LD(HART *hart, inst_data& inst);

// S-Type
inst_ret exec_SB(HART *hart, inst_data& inst);
inst_ret exec_SH(HART *hart, inst_data& inst);
inst_ret exec_SW(HART *hart, inst_data& inst);
inst_ret exec_SD(HART *hart, inst_data& inst);

// B-Type
inst_ret exec_BEQ(HART *hart, inst_data& inst);
inst_ret exec_BNE(HART *hart, inst_data& inst);
inst_ret exec_BLT(HART *hart, inst_data& inst);
inst_ret exec_BGE(HART *hart, inst_data& inst);
inst_ret exec_BLTU(HART *hart, inst_data& inst);
inst_ret exec_BGEU(HART *hart, inst_data& inst);

// JUMP
inst_ret exec_JAL(HART *hart, inst_data& inst);
inst_ret exec_JALR(HART *hart, inst_data& inst);

// what
inst_ret exec_LUI(HART *hart, inst_data& inst);
inst_ret exec_AUIPC(HART *hart, inst_data& inst);

inst_ret exec_FENCE(HART *hart, inst_data& inst);

inst_ret exec_ECALL(HART *hart, inst_data& inst);
inst_ret exec_EBREAK(HART *hart, inst_data& inst);

//// RV64M
// R-Type
inst_ret exec_MUL(HART *hart, inst_data& inst);
inst_ret exec_MULH(HART *hart, inst_data& inst);
inst_ret exec_MULHSU(HART *hart, inst_data& inst);
inst_ret exec_MULHU(HART *hart, inst_data& inst);
inst_ret exec_DIV(HART *hart, inst_data& inst);
inst_ret exec_DIVU(HART *hart, inst_data& inst);
inst_ret exec_REM(HART *hart, inst_data& inst);
inst_ret exec_REMU(HART *hart, inst_data& inst);

inst_ret exec_MULW(HART *hart, inst_data& inst);
inst_ret exec_DIVW(HART *hart, inst_data& inst);
inst_ret exec_DIVUW(HART *hart, inst_data& inst);
inst_ret exec_REMW(HART *hart, inst_data& inst);
inst_ret exec_REMUW(HART *hart, inst_data& inst);

//// ZiCSR

inst_ret exec_CSRRW(HART *hart, inst_data& inst);
inst_ret exec_CSRRS(HART *hart, inst_data& inst);
inst_ret exec_CSRRC(HART *hart, inst_data& inst);
inst_ret exec_CSRRWI(HART *hart, inst_data& inst);
inst_ret exec_CSRRSI(HART *hart, inst_data& inst);
inst_ret exec_CSRRCI(HART *hart, inst_data& inst);

// RV64A

void amo_check_reservation(Machine& cpu, uint64_t va);
inst_ret exec_LR_D(HART *hart, inst_data& inst);
inst_ret exec_SC_D(HART *hart, inst_data& inst);
inst_ret exec_AMOSWAP_D(HART *hart, inst_data& inst);
inst_ret exec_AMOADD_D(HART *hart, inst_data& inst);
inst_ret exec_AMOXOR_D(HART *hart, inst_data& inst);
inst_ret exec_AMOAND_D(HART *hart, inst_data& inst);
inst_ret exec_AMOOR_D(HART *hart, inst_data& inst);
inst_ret exec_AMOMIN_D(HART *hart, inst_data& inst);
inst_ret exec_AMOMAX_D(HART *hart, inst_data& inst);
inst_ret exec_AMOMINU_D(HART *hart, inst_data& inst);
inst_ret exec_AMOMAXU_D(HART *hart, inst_data& inst);

inst_ret exec_LR_W(HART *hart, inst_data& inst);
inst_ret exec_SC_W(HART *hart, inst_data& inst);
inst_ret exec_AMOSWAP_W(HART *hart, inst_data& inst);
inst_ret exec_AMOADD_W(HART *hart, inst_data& inst);
inst_ret exec_AMOXOR_W(HART *hart, inst_data& inst);
inst_ret exec_AMOAND_W(HART *hart, inst_data& inst);
inst_ret exec_AMOOR_W(HART *hart, inst_data& inst);
inst_ret exec_AMOMIN_W(HART *hart, inst_data& inst);
inst_ret exec_AMOMAX_W(HART *hart, inst_data& inst);
inst_ret exec_AMOMINU_W(HART *hart, inst_data& inst);
inst_ret exec_AMOMAXU_W(HART *hart, inst_data& inst);

#ifdef USE_FPU
// RV64F
inst_ret exec_FLW(HART *hart, inst_data& inst);
inst_ret exec_FSW(HART *hart, inst_data& inst);
inst_ret exec_FADD_S(HART *hart, inst_data& inst);
inst_ret exec_FSUB_S(HART *hart, inst_data& inst);
inst_ret exec_FMUL_S(HART *hart, inst_data& inst);
inst_ret exec_FDIV_S(HART *hart, inst_data& inst);
inst_ret exec_FSQRT_S(HART *hart, inst_data& inst);
inst_ret exec_FMIN_S(HART *hart, inst_data& inst);
inst_ret exec_FMAX_S(HART *hart, inst_data& inst);
inst_ret exec_FMADD_S(HART *hart, inst_data& inst);
inst_ret exec_FMSUB_S(HART *hart, inst_data& inst);
inst_ret exec_FNMADD_S(HART *hart, inst_data& inst);
inst_ret exec_FNMSUB_S(HART *hart, inst_data& inst);
inst_ret exec_FCVT_W_S(HART *hart, inst_data& inst);
inst_ret exec_FCVT_WU_S(HART *hart, inst_data& inst);
inst_ret exec_FCVT_L_S(HART *hart, inst_data& inst);
inst_ret exec_FCVT_LU_S(HART *hart, inst_data& inst);
inst_ret exec_FCVT_S_W(HART *hart, inst_data& inst);
inst_ret exec_FCVT_S_WU(HART *hart, inst_data& inst);
inst_ret exec_FCVT_S_L(HART *hart, inst_data& inst);
inst_ret exec_FCVT_S_LU(HART *hart, inst_data& inst);
inst_ret exec_FSGNJ_S(HART *hart, inst_data& inst);
inst_ret exec_FSGNJN_S(HART *hart, inst_data& inst);
inst_ret exec_FSGNJX_S(HART *hart, inst_data& inst);
inst_ret exec_FMV_X_W(HART *hart, inst_data& inst);
inst_ret exec_FMV_W_X(HART *hart, inst_data& inst);
inst_ret exec_FLE_S(HART *hart, inst_data& inst);
inst_ret exec_FLT_S(HART *hart, inst_data& inst);
inst_ret exec_FEQ_S(HART *hart, inst_data& inst);
inst_ret exec_FCLASS_S(HART *hart, inst_data& inst);

// RV64D
inst_ret exec_FLD(HART *hart, inst_data& inst);
inst_ret exec_FSD(HART *hart, inst_data& inst);
inst_ret exec_FADD_D(HART *hart, inst_data& inst);
inst_ret exec_FSUB_D(HART *hart, inst_data& inst);
inst_ret exec_FMUL_D(HART *hart, inst_data& inst);
inst_ret exec_FDIV_D(HART *hart, inst_data& inst);
inst_ret exec_FSQRT_D(HART *hart, inst_data& inst);
inst_ret exec_FMIN_D(HART *hart, inst_data& inst);
inst_ret exec_FMAX_D(HART *hart, inst_data& inst);
inst_ret exec_FMADD_D(HART *hart, inst_data& inst);
inst_ret exec_FMSUB_D(HART *hart, inst_data& inst);
inst_ret exec_FNMADD_D(HART *hart, inst_data& inst);
inst_ret exec_FNMSUB_D(HART *hart, inst_data& inst);
inst_ret exec_FCVT_W_D(HART *hart, inst_data& inst);
inst_ret exec_FCVT_WU_D(HART *hart, inst_data& inst);
inst_ret exec_FCVT_L_D(HART *hart, inst_data& inst);
inst_ret exec_FCVT_LU_D(HART *hart, inst_data& inst);
inst_ret exec_FCVT_D_W(HART *hart, inst_data& inst);
inst_ret exec_FCVT_D_WU(HART *hart, inst_data& inst);
inst_ret exec_FCVT_D_L(HART *hart, inst_data& inst);
inst_ret exec_FCVT_D_LU(HART *hart, inst_data& inst);
inst_ret exec_FSGNJ_D(HART *hart, inst_data& inst);
inst_ret exec_FSGNJN_D(HART *hart, inst_data& inst);
inst_ret exec_FSGNJX_D(HART *hart, inst_data& inst);
inst_ret exec_FMV_X_D(HART *hart, inst_data& inst);
inst_ret exec_FMV_D_X(HART *hart, inst_data& inst);
inst_ret exec_FLE_D(HART *hart, inst_data& inst);
inst_ret exec_FLT_D(HART *hart, inst_data& inst);
inst_ret exec_FEQ_D(HART *hart, inst_data& inst);
inst_ret exec_FCLASS_D(HART *hart, inst_data& inst);

#endif

//// SYSTEM

inst_ret exec_MRET(HART *hart, inst_data& inst);
inst_ret exec_SRET(HART *hart, inst_data& inst);
inst_ret exec_SFENCE_VMA(HART *hart, inst_data& inst);
inst_ret exec_WFI(HART *hart, inst_data& inst);

// Zbb

inst_ret exec_ANDN(HART *hart, inst_data& inst);
inst_ret exec_ORN(HART *hart, inst_data& inst);
inst_ret exec_XNOR(HART *hart, inst_data& inst);
inst_ret exec_CLZ(HART *hart, inst_data& inst);
inst_ret exec_CLZW(HART *hart, inst_data& inst);
inst_ret exec_CTZ(HART *hart, inst_data& inst);
inst_ret exec_CTZW(HART *hart, inst_data& inst);
inst_ret exec_CPOP(HART *hart, inst_data& inst);
inst_ret exec_CPOPW(HART *hart, inst_data& inst);
inst_ret exec_MAX(HART *hart, inst_data& inst);
inst_ret exec_MAXU(HART *hart, inst_data& inst);
inst_ret exec_MIN(HART *hart, inst_data& inst);
inst_ret exec_MINU(HART *hart, inst_data& inst);
inst_ret exec_SEXT_B(HART *hart, inst_data& inst);
inst_ret exec_SEXT_H(HART *hart, inst_data& inst);
inst_ret exec_ZEXT_H(HART *hart, inst_data& inst);
inst_ret exec_ROL(HART *hart, inst_data& inst);
inst_ret exec_ROLW(HART *hart, inst_data& inst);
inst_ret exec_ROR(HART *hart, inst_data& inst);
inst_ret exec_RORI(HART *hart, inst_data& inst);
inst_ret exec_RORIW(HART *hart, inst_data& inst);
inst_ret exec_RORW(HART *hart, inst_data& inst);
inst_ret exec_ORC_B(HART *hart, inst_data& inst);
inst_ret exec_REV8(HART *hart, inst_data& inst);

// Zba

inst_ret exec_ADD_UW(HART *hart, inst_data& inst);
inst_ret exec_SH1ADD(HART *hart, inst_data& inst);
inst_ret exec_SH1ADD_UW(HART *hart, inst_data& inst);
inst_ret exec_SH2ADD(HART *hart, inst_data& inst);
inst_ret exec_SH2ADD_UW(HART *hart, inst_data& inst);
inst_ret exec_SH3ADD(HART *hart, inst_data& inst);
inst_ret exec_SH3ADD_UW(HART *hart, inst_data& inst);
inst_ret exec_SLLI_UW(HART *hart, inst_data& inst);

// Zbc

inst_ret exec_CLMUL(HART *hart, inst_data& inst);
inst_ret exec_CLMULH(HART *hart, inst_data& inst);
inst_ret exec_CLMULR(HART *hart, inst_data& inst);

// Zbs

inst_ret exec_BCLR(HART *hart, inst_data& inst);
inst_ret exec_BCLRI(HART *hart, inst_data& inst);
inst_ret exec_BEXT(HART *hart, inst_data& inst);
inst_ret exec_BEXTI(HART *hart, inst_data& inst);
inst_ret exec_BINV(HART *hart, inst_data& inst);
inst_ret exec_BINVI(HART *hart, inst_data& inst);
inst_ret exec_BSET(HART *hart, inst_data& inst);
inst_ret exec_BSETI(HART *hart, inst_data& inst);

// ZifenceI

inst_ret exec_FENCE_I(HART *hart, inst_data& inst);