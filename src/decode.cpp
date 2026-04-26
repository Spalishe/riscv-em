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

#include "../include/decode.h"
#include "../include/cpu.hpp"
#include <stdint.h>

uint64_t d_rd(uint32_t inst) {
  return (inst >> 7) & 0x1f; // rd in bits 11..7
}
uint64_t d_rs1(uint32_t inst) {
  return (inst >> 15) & 0x1f; // rs1 in bits 19..15
}
uint64_t d_rs2(uint32_t inst) {
  return (inst >> 20) & 0x1f; // rs2 in bits 24..20
}
uint64_t d_rs3(uint32_t inst) {
  return (inst >> 27) & 0x1f; // rs3 in bits 31..27
}
uint64_t imm_Zicsr(uint32_t inst) { return (inst >> 20); }
uint64_t imm_I(uint32_t inst) {
  // imm[11:0] = inst[31:20]
  return sext(inst >> 20, 12);
}
uint64_t imm_S(uint32_t inst) {
  // imm[11:5] = inst[31:25], imm[4:0] = inst[11:7]
  return sext((get_bits(inst, 11, 7) | (get_bits(inst, 31, 25) << 5)), 12);
}
uint64_t imm_B(uint32_t inst) {
  // imm[12|10:5|4:1|11] = inst[31|30:25|11:8|7]
  return sext(((get_bits(inst, 11, 8) << 1) | (get_bits(inst, 30, 25) << 5) |
               (get_bits(inst, 7, 7) << 11) | (get_bits(inst, 31, 31) << 12)),
              13);
}
uint64_t imm_U(uint32_t inst) {
  // imm[31:12] = inst[31:12]
  return sext(inst >> 12, 20);
}
uint64_t imm_J(uint32_t inst) {
  // imm[20|10:1|11|19:12] = inst[31|30:21|20|19:12]
  return sext((get_bits(inst, 30, 21) << 1) | (get_bits(inst, 20, 20) << 11) |
                  (get_bits(inst, 19, 12) << 12) |
                  (get_bits(inst, 31, 31) << 20),
              21);
}
uint64_t C_imm_J(uint16_t inst) {
  return ((get_bits(inst, 12, 12) << 11) | (get_bits(inst, 11, 11) << 4) |
          (get_bits(inst, 10, 9) << 8) | (get_bits(inst, 8, 8) << 10) |
          (get_bits(inst, 7, 7) << 6) | (get_bits(inst, 6, 6) << 7) |
          (get_bits(inst, 5, 3) << 1) | (get_bits(inst, 2, 2) << 5));
}
uint32_t shamt(uint32_t inst) {
  // shamt(shift amount) only required for immediate shift instructions
  // shamt[4:5] = imm[5:0]
  return (uint32_t)(imm_I(inst) & 0x1f);
}
uint32_t shamt64(uint32_t inst) {
  // shamt(shift amount) only required for immediate shift instructions
  // shamt[4:5] = imm[5:0]
  return (uint32_t)(imm_I(inst) & 0x3f);
}

int32_t sext(uint32_t val, int bits) {
  int32_t shift = 32 - bits;
  return (int32_t)(val << shift) >> shift;
}
uint32_t get_bits(uint32_t inst, int hi, int lo) {
  return (inst >> lo) & ((1u << (hi - lo + 1)) - 1);
}

inst_data parse_instruction(struct HART *hart, uint32_t inst, uint64_t pc) {
  int OP = inst & 3;

  uint8_t rd = 0;
  uint8_t rs1 = 0;
  uint8_t rs2 = 0;
  uint8_t rs3 = 0;
  uint64_t d_imm = 0;

  bool valid = false;
  bool canChangePC = false;
  inst_ret (*fn)(HART *, inst_data &);

  inst_data& cache = hart->instr_cache[(pc >> 2) & 0x1FFF];
  if (cache.valid && cache.inst == inst) {
    return cache;
  } else {
    int opcode = inst & 0x7f;
    int funct3 = (inst >> 12) & 0x7;
    int amo_funct5 = (inst >> 27) & 0x1F;
    int funct7 = (inst >> 25) & 0x7f;
    int funct6 = (inst >> 26);
    int imm = (inst >> 20);
    FMT fmt = (FMT)((inst >> 25) & 0x3);
    RoundingMode rm = (RoundingMode)funct3;

    switch (opcode) {
    case FENCE:
      switch (funct3) {
      case 1:
        fn = exec_FENCE_I;
        valid = true;
        canChangePC = true;
        break;
      case 0:
        fn = exec_FENCE;
        valid = true;
        canChangePC = true;
        break;
      };
      break;
    case R_TYPE:
      switch (funct3) {
      case ADDSUB:
        switch (funct7) {
        case ADD:
          fn = exec_ADD;
          valid = true;
          break;
        case 1:
          fn = exec_MUL;
          valid = true;
          break;
        case SUB:
          fn = exec_SUB;
          valid = true;
          break;
        };
        break;
      case XOR:
        switch (funct7) {
        case 0:
          fn = exec_XOR;
          valid = true;
          break;
        case 5:
          fn = exec_MIN;
          valid = true;
          break;
        case 1:
          fn = exec_DIV;
          valid = true;
          break;
        case 16:
          fn = exec_SH2ADD;
          valid = true;
          break;
        case 32:
          fn = exec_XNOR;
          valid = true;
          break;
        };
        break;
      case OR:
        switch (funct7) {
        case 0:
          fn = exec_OR;
          valid = true;
          break;
        case 5:
          fn = exec_MAX;
          valid = true;
          break;
        case 1:
          fn = exec_REM;
          valid = true;
          break;
        case 16:
          fn = exec_SH3ADD;
          valid = true;
          break;
        case 32:
          fn = exec_ORN;
          valid = true;
          break;
        };
        valid = true;
        break;
      case AND:
        switch (funct7) {
        case 0:
          fn = exec_AND;
          valid = true;
          break;
        case 5:
          fn = exec_MAXU;
          valid = true;
          break;
        case 1:
          fn = exec_REMU;
          valid = true;
          break;
        case 32:
          fn = exec_ANDN;
          valid = true;
          break;
        };
        break;
      case SLL:
        switch (funct7) {
        case 0:
          fn = exec_SLL;
          valid = true;
          break;
        case 1:
          fn = exec_MULH;
          valid = true;
          break;
        case 5:
          fn = exec_CLMUL;
          valid = true;
          break;
        case 20:
          fn = exec_BSET;
          valid = true;
          break;
        case 36:
          fn = exec_BCLR;
          valid = true;
          break;
        case 48:
          fn = exec_ROL;
          valid = true;
          break;
        };
        break;
      case SR:
        switch (funct7) {
        case SRL:
          fn = exec_SRL;
          valid = true;
          break;
        case 5:
          fn = exec_MINU;
          valid = true;
          break;
        case SRA:
          fn = exec_SRA;
          valid = true;
          break;
        case 36:
          fn = exec_BEXT;
          valid = true;
          break;
        case 48:
          fn = exec_ROR;
          valid = true;
          break;
        case 1:
          fn = exec_DIVU;
          valid = true;
          break;
        };
        break;
      case SLT:
        switch (funct7) {
        case 0:
          fn = exec_SLT;
          valid = true;
          break;
        case 1:
          fn = exec_MULHSU;
          valid = true;
          break;
        case 5:
          fn = exec_CLMULR;
          valid = true;
          break;
        case 16:
          fn = exec_SH1ADD;
          valid = true;
          break;
        case 52:
          fn = exec_BINV;
          valid = true;
          break;
        };
        break;
      case SLTU:
        switch (funct7) {
        case 0:
          fn = exec_SLTU;
          valid = true;
          break;
        case 1:
          fn = exec_MULHU;
          valid = true;
          break;
        case 5:
          fn = exec_CLMULH;
          valid = true;
          break;
        };
        break;
      };
      rs1 = d_rs1(inst);
      rs2 = d_rs2(inst);
      rd = d_rd(inst);
      break;
    case R_TYPE64:
      switch (funct3) {
      case ADDSUB:
        switch (funct7) {
        case ADD:
          fn = exec_ADDW;
          valid = true;
          break;
        case SUB:
          fn = exec_SUBW;
          valid = true;
          break;
        case 1:
          fn = exec_MULW;
          valid = true;
          break;
        case 4:
          fn = exec_ADD_UW;
          valid = true;
          break;
        };
        break;
      case SLL:
        switch (funct7) {
        case 0:
          fn = exec_SLLW;
          valid = true;
          break;
        case 48:
          fn = exec_ROLW;
          valid = true;
          break;
        };
        break;
      case SR:
        switch (funct7) {
        case SRL:
          fn = exec_SRLW;
          valid = true;
          break;
        case SRA:
          fn = exec_SRAW;
          valid = true;
          break;
        case 48:
          fn = exec_RORW;
          valid = true;
          break;
        case 1:
          fn = exec_DIVUW;
          valid = true;
          break;
        };
        break;
      case XOR:
        switch (funct7) {
        case 1:
          fn = exec_DIVW;
          valid = true;
          break;
        case 16:
          fn = exec_SH2ADD_UW;
          valid = true;
          break;
        };
        break;
      case OR:
        switch (funct7) {
        case 1:
          fn = exec_REMW;
          valid = true;
          break;
        case 16:
          fn = exec_SH3ADD_UW;
          valid = true;
          break;
        };
        break;
      case AND:
        fn = exec_REMUW;
        valid = true;
        break;
      case SLT:
        fn = exec_SH1ADD_UW;
        valid = true;
        break;
      };
      rs1 = d_rs1(inst);
      rs2 = d_rs2(inst);
      rd = d_rd(inst);
      break;
    case AMO:
      switch (funct3) {
      case AMO_W:
        switch (amo_funct5) {
        case AMOADD:
          fn = exec_AMOADD_W;
          valid = true;
          break;
        case AMOSWAP:
          fn = exec_AMOSWAP_W;
          valid = true;
          break;
        case LR:
          fn = exec_LR_W;
          valid = true;
          break;
        case SC:
          fn = exec_SC_W;
          valid = true;
          break;
        case AMOXOR:
          fn = exec_AMOXOR_W;
          valid = true;
          break;
        case AMOOR:
          fn = exec_AMOOR_W;
          valid = true;
          break;
        case AMOAND:
          fn = exec_AMOAND_W;
          valid = true;
          break;
        case AMOMIN:
          fn = exec_AMOMIN_W;
          valid = true;
          break;
        case AMOMAX:
          fn = exec_AMOMAX_W;
          valid = true;
          break;
        case AMOMINU:
          fn = exec_AMOMINU_W;
          valid = true;
          break;
        case AMOMAXU:
          fn = exec_AMOMAXU_W;
          valid = true;
          break;
        };
        break;
      case AMO_D:
        switch (amo_funct5) {
        case AMOADD:
          fn = exec_AMOADD_D;
          valid = true;
          break;
        case AMOSWAP:
          fn = exec_AMOSWAP_D;
          valid = true;
          break;
        case LR:
          fn = exec_LR_D;
          valid = true;
          break;
        case SC:
          fn = exec_SC_D;
          valid = true;
          break;
        case AMOXOR:
          fn = exec_AMOXOR_D;
          valid = true;
          break;
        case AMOOR:
          fn = exec_AMOOR_D;
          valid = true;
          break;
        case AMOAND:
          fn = exec_AMOAND_D;
          valid = true;
          break;
        case AMOMIN:
          fn = exec_AMOMIN_D;
          valid = true;
          break;
        case AMOMAX:
          fn = exec_AMOMAX_D;
          valid = true;
          break;
        case AMOMINU:
          fn = exec_AMOMINU_D;
          valid = true;
          break;
        case AMOMAXU:
          fn = exec_AMOMAXU_D;
          valid = true;
          break;
        };
        break;
      };
      rs1 = d_rs1(inst);
      rs2 = d_rs2(inst);
      rd = d_rd(inst);
      break;
    case I_TYPE:
      switch (funct3) {
      case ADDI:
        fn = exec_ADDI;
        d_imm = imm_I(inst);
        valid = true;
        break;
      case XORI:
        fn = exec_XORI;
        d_imm = imm_I(inst);
        valid = true;
        break;
      case ORI:
        fn = exec_ORI;
        d_imm = imm_I(inst);
        valid = true;
        break;
      case ANDI:
        fn = exec_ANDI;
        d_imm = imm_I(inst);
        valid = true;
        break;
      case SLLI:
        switch (funct6) {
        case 0:
          fn = exec_SLLI;
          d_imm = shamt64(inst);
          valid = true;
          break;
        case 10:
          fn = exec_BSETI;
          d_imm = shamt64(inst);
          valid = true;
          break;
        case 26:
          fn = exec_BINVI;
          d_imm = shamt64(inst);
          valid = true;
          break;
        case 36:
          fn = exec_BCLRI;
          d_imm = shamt64(inst);
          valid = true;
          break;
        case 24:
          switch ((inst >> 20) & 0x1F) {
          case 0:
            fn = exec_CLZ;
            valid = true;
            break;
          case 1:
            fn = exec_CTZ;
            valid = true;
            break;
          case 2:
            fn = exec_CPOP;
            valid = true;
            break;
          case 4:
            fn = exec_SEXT_B;
            valid = true;
            break;
          case 5:
            fn = exec_SEXT_H;
            valid = true;
            break;
          };
          break;
        };
        break;
      case SRI:
        /*switch(funct7) {
            case SRLI: fn = exec_SRLI; valid = true; break;
            case SRAI: fn = exec_SRAI; valid = true; break;
        }; break;*/ // 32-bit
        switch (funct6) {
        case SRLI:
          fn = exec_SRLI;
          d_imm = shamt64(inst);
          valid = true;
          break;
        case SRAI:
          fn = exec_SRAI;
          d_imm = shamt64(inst);
          valid = true;
          break;
        case 18:
          fn = exec_BEXTI;
          d_imm = shamt64(inst);
          valid = true;
          break;
        case 24:
          fn = exec_RORI;
          d_imm = shamt64(inst);
          valid = true;
          break;
        };
        switch (inst >> 20) {
        case 647:
          fn = exec_ORC_B;
          valid = true;
          break;
        case 1688:
        case 1720:
          fn = exec_REV8;
          valid = true;
          break;
        };
        break;
      case SLTI:
        fn = exec_SLTI;
        d_imm = imm_I(inst);
        valid = true;
        break;
      case SLTIU:
        fn = exec_SLTIU;
        d_imm = imm_I(inst);
        valid = true;
        break;
      };
      rs1 = d_rs1(inst);
      rd = d_rd(inst);
      break;
    case I_TYPE64:
      switch (funct3) {
      case XORI:
        switch (funct7) {
        case 4:
          fn = exec_ZEXT_H;
          valid = true;
          break;
        };
        break;
      case ADDI:
        fn = exec_ADDIW;
        d_imm = imm_I(inst);
        valid = true;
        break;
      case SLLI:
        switch (funct6) {
        case 2:
          fn = exec_SLLI_UW;
          d_imm = shamt64(inst);
          valid = true;
          break;
        }
        switch (funct7) {
        case 0:
          fn = exec_SLLIW;
          d_imm = shamt(inst);
          valid = true;
          break;
        case 48:
          switch ((inst >> 20) & 0x1F) {
          case 0:
            fn = exec_CLZW;
            valid = true;
            break;
          case 1:
            fn = exec_CTZW;
            valid = true;
            break;
          case 2:
            fn = exec_CPOPW;
            valid = true;
            break;
          };
          break;
        };
        break;
      case SRI:
        switch (funct7) {
        case SRLI:
          fn = exec_SRLIW;
          d_imm = shamt(inst);
          valid = true;
          break;
        case SRAIW:
          fn = exec_SRAIW;
          d_imm = shamt(inst);
          valid = true;
          break;
        case 24:
          fn = exec_RORIW;
          d_imm = shamt(inst);
          valid = true;
          break;
        };
        break;
      };
      rs1 = d_rs1(inst);
      rd = d_rd(inst);
      break;
    case LOAD_TYPE:
      switch (funct3) {
      case LB:
        fn = exec_LB;
        valid = true;
        break;
      case LH:
        fn = exec_LH;
        valid = true;
        break;
      case LW:
        fn = exec_LW;
        valid = true;
        break;
      case LD:
        fn = exec_LD;
        valid = true;
        break;
      case LBU:
        fn = exec_LBU;
        valid = true;
        break;
      case LHU:
        fn = exec_LHU;
        valid = true;
        break;
      case LWU:
        fn = exec_LWU;
        valid = true;
        break;
      };
      rs1 = d_rs1(inst);
      d_imm = imm_I(inst);
      rd = d_rd(inst);
      break;
    case S_TYPE:
      switch (funct3) {
      case SB:
        fn = exec_SB;
        valid = true;
        break;
      case SH:
        fn = exec_SH;
        valid = true;
        break;
      case SW:
        fn = exec_SW;
        valid = true;
        break;
      case inst_SD:
        fn = exec_SD;
        valid = true;
        break;
      };
      rs1 = d_rs1(inst);
      rs2 = d_rs2(inst);
      d_imm = imm_S(inst);
      break;
    case B_TYPE:
      switch (funct3) {
      case BEQ:
        fn = exec_BEQ;
        valid = true;
        break;
      case BNE:
        fn = exec_BNE;
        valid = true;
        break;
      case BLT:
        fn = exec_BLT;
        valid = true;
        break;
      case BGE:
        fn = exec_BGE;
        valid = true;
        break;
      case BLTU:
        fn = exec_BLTU;
        valid = true;
        break;
      case BGEU:
        fn = exec_BGEU;
        valid = true;
        break;
      };
      rs1 = d_rs1(inst);
      rs2 = d_rs2(inst);
      d_imm = imm_B(inst);
      canChangePC = true;
      break;
    case JAL:
      fn = exec_JAL;
      valid = true;
      canChangePC = true;
      rd = d_rd(inst);
      d_imm = imm_J(inst);
      break;
    case JALR:
      fn = exec_JALR;
      valid = true;
      canChangePC = true;
      rd = d_rd(inst);
      rs1 = d_rs1(inst);
      d_imm = imm_I(inst);
      break;
    case LUI:
      fn = exec_LUI;
      valid = true;
      rd = d_rd(inst);
      d_imm = imm_U(inst);
      break;
    case AUIPC:
      fn = exec_AUIPC;
      valid = true;
      rd = d_rd(inst);
      d_imm = imm_U(inst);
      break;
    case ECALL:
      switch (funct3) {
      case CSRRW:
        fn = exec_CSRRW;
        rd = d_rd(inst);
        rs1 = d_rs1(inst);
        d_imm = imm_Zicsr(inst);
        valid = true;
        break;
      case CSRRS:
        fn = exec_CSRRS;
        rd = d_rd(inst);
        rs1 = d_rs1(inst);
        d_imm = imm_Zicsr(inst);
        valid = true;
        break;
      case CSRRC:
        fn = exec_CSRRC;
        rd = d_rd(inst);
        rs1 = d_rs1(inst);
        d_imm = imm_Zicsr(inst);
        valid = true;
        break;
      case CSRRWI:
        fn = exec_CSRRWI;
        rd = d_rd(inst);
        rs1 = d_rs1(inst);
        d_imm = imm_Zicsr(inst);
        valid = true;
        break;
      case CSRRSI:
        fn = exec_CSRRSI;
        rd = d_rd(inst);
        rs1 = d_rs1(inst);
        d_imm = imm_Zicsr(inst);
        valid = true;
        break;
      case CSRRCI:
        fn = exec_CSRRCI;
        rd = d_rd(inst);
        rs1 = d_rs1(inst);
        d_imm = imm_Zicsr(inst);
        valid = true;
        break;
      case 0:
        switch (imm) {
        case 0:
          fn = exec_ECALL;
          valid = true;
          break;
        case 1:
          fn = exec_EBREAK;
          valid = true;
          break;
        case 261:
          fn = exec_WFI;
          valid = true;
          break;
        case 258:
          fn = exec_SRET;
          valid = true;
          break;
        case 288:
          fn = exec_SFENCE_VMA;
          valid = true;
          break;
        case 770:
          fn = exec_MRET;
          valid = true;
          break;
        };
        canChangePC = true;
        break;
      };
      break;
#ifdef USE_FPU
    case FLOAD:
      switch (funct3) {
      case 0x2:
        fn = exec_FLW;
        rd = d_rd(inst);
        rs1 = d_rs1(inst);
        d_imm = imm_I(inst);
        valid = true;
        break;
      case 0x3:
        fn = exec_FLD;
        rd = d_rd(inst);
        rs1 = d_rs1(inst);
        d_imm = imm_I(inst);
        valid = true;
        break;
      };
      break;
    case FSTORE:
      switch (funct3) {
      case 0x2:
        fn = exec_FSW;
        rs1 = d_rs1(inst);
        rs2 = d_rs2(inst);
        d_imm = imm_S(inst);
        valid = true;
        break;
      case 0x3:
        fn = exec_FSD;
        rs1 = d_rs1(inst);
        rs2 = d_rs2(inst);
        d_imm = imm_S(inst);
        valid = true;
        break;
      };
      break;
    case R_F:
      rs2 = d_rs2(inst);
      rd = d_rd(inst);
      rs1 = d_rs1(inst);
      switch (amo_funct5) {
      case FADD:
        switch (fmt) {
        case FMT::S:
          fn = exec_FADD_S;
          valid = true;
          break;
        case FMT::D:
          fn = exec_FADD_D;
          valid = true;
          break;
        };
        break;
      case FSUB:
        switch (fmt) {
        case FMT::S:
          fn = exec_FSUB_S;
          valid = true;
          break;
        case FMT::D:
          fn = exec_FSUB_D;
          valid = true;
          break;
        };
        break;
      case FMUL:
        switch (fmt) {
        case FMT::S:
          fn = exec_FMUL_S;
          valid = true;
          break;
        case FMT::D:
          fn = exec_FMUL_D;
          valid = true;
          break;
        };
        break;
      case FDIV:
        switch (fmt) {
        case FMT::S:
          fn = exec_FDIV_S;
          valid = true;
          break;
        case FMT::D:
          fn = exec_FDIV_D;
          valid = true;
          break;
        };
        break;
      case FSQRT:
        switch (fmt) {
        case FMT::S:
          fn = exec_FSQRT_S;
          valid = true;
          break;
        case FMT::D:
          fn = exec_FSQRT_D;
          valid = true;
          break;
        };
        break;
      case FMIN_MAX:
        switch (funct3) {
        case FMIN:
          switch (fmt) {
          case FMT::S:
            fn = exec_FMIN_S;
            valid = true;
            break;
          case FMT::D:
            fn = exec_FMIN_D;
            valid = true;
            break;
          };
          break;
        case FMAX:
          switch (fmt) {
          case FMT::S:
            fn = exec_FMAX_S;
            valid = true;
            break;
          case FMT::D:
            fn = exec_FMAX_D;
            valid = true;
            break;
          };
          break;
        };
        break;
      case FCVT_X_T:
        switch (rs2) {
        case 0:
          switch (fmt) {
          case FMT::S:
            fn = exec_FCVT_W_S;
            valid = true;
            break;
          case FMT::D:
            fn = exec_FCVT_W_D;
            valid = true;
            break;
          };
          break;
        case 1:
          switch (fmt) {
          case FMT::S:
            fn = exec_FCVT_WU_S;
            valid = true;
            break;
          case FMT::D:
            fn = exec_FCVT_WU_D;
            valid = true;
            break;
          };
          break;
        case 2:
          switch (fmt) {
          case FMT::S:
            fn = exec_FCVT_L_S;
            valid = true;
            break;
          case FMT::D:
            fn = exec_FCVT_L_D;
            valid = true;
            break;
          };
          break;
        case 3:
          switch (fmt) {
          case FMT::S:
            fn = exec_FCVT_LU_S;
            valid = true;
            break;
          case FMT::D:
            fn = exec_FCVT_LU_D;
            valid = true;
            break;
          };
          break;
        };
        break;
      case FCVT_T_X:
        switch (rs2) {
        case 0:
          switch (fmt) {
          case FMT::S:
            fn = exec_FCVT_S_W;
            valid = true;
            break;
          case FMT::D:
            fn = exec_FCVT_D_W;
            valid = true;
            break;
          };
          break;
        case 1:
          switch (fmt) {
          case FMT::S:
            fn = exec_FCVT_S_WU;
            valid = true;
            break;
          case FMT::D:
            fn = exec_FCVT_D_WU;
            valid = true;
            break;
          };
          break;
        case 2:
          switch (fmt) {
          case FMT::S:
            fn = exec_FCVT_S_L;
            valid = true;
            break;
          case FMT::D:
            fn = exec_FCVT_D_L;
            valid = true;
            break;
          };
          break;
        case 3:
          switch (fmt) {
          case FMT::S:
            fn = exec_FCVT_S_LU;
            valid = true;
            break;
          case FMT::D:
            fn = exec_FCVT_D_LU;
            valid = true;
            break;
          };
          break;
        };
        break;
      case FSGN:
        switch (funct3) {
        case FSGNJ:
          switch (fmt) {
          case FMT::S:
            fn = exec_FSGNJ_S;
            valid = true;
            break;
          case FMT::D:
            fn = exec_FSGNJ_D;
            valid = true;
            break;
          };
          break;
        case FSGNJN:
          switch (fmt) {
          case FMT::S:
            fn = exec_FSGNJN_S;
            valid = true;
            break;
          case FMT::D:
            fn = exec_FSGNJN_D;
            valid = true;
            break;
          };
          break;
        case FSGNJX:
          switch (fmt) {
          case FMT::S:
            fn = exec_FSGNJX_S;
            valid = true;
            break;
          case FMT::D:
            fn = exec_FSGNJX_D;
            valid = true;
            break;
          };
          break;
        };
        break;
      case FMV_X_T:
        switch (funct3) {
        case 0:
          switch (fmt) {
          case FMT::S:
            fn = exec_FMV_X_W;
            valid = true;
            break;
          case FMT::D:
            fn = exec_FMV_X_D;
            valid = true;
            break;
          };
          break;
        case 1:
          switch (fmt) {
          case FMT::S:
            fn = exec_FCLASS_S;
            valid = true;
            break;
          case FMT::D:
            fn = exec_FCLASS_D;
            valid = true;
            break;
          };
          break;
        };
        break;
      case FMV_T_X:
        switch (fmt) {
        case FMT::S:
          fn = exec_FMV_W_X;
          valid = true;
          break;
        case FMT::D:
          fn = exec_FMV_D_X;
          valid = true;
          break;
        };
        break;
      case FBR:
        switch (funct3) {
        case FLE:
          switch (fmt) {
          case FMT::S:
            fn = exec_FLE_S;
            valid = true;
            break;
          case FMT::D:
            fn = exec_FLE_D;
            valid = true;
            break;
          };
          break;
        case FLT:
          switch (fmt) {
          case FMT::S:
            fn = exec_FLT_S;
            valid = true;
            break;
          case FMT::D:
            fn = exec_FLT_D;
            valid = true;
            break;
          };
          break;
        case FEQ:
          switch (fmt) {
          case FMT::S:
            fn = exec_FEQ_S;
            valid = true;
            break;
          case FMT::D:
            fn = exec_FEQ_D;
            valid = true;
            break;
          };
          break;
        };
        break;
      };
      break;
    case FMADD:
      switch (fmt) {
      case FMT::S:
        fn = exec_FMADD_S;
        valid = true;
        break;
      case FMT::D:
        fn = exec_FMADD_D;
        valid = true;
        break;
      };
      rd = d_rd(inst);
      rs1 = d_rs1(inst);
      rs2 = d_rs2(inst);
      rs3 = d_rs3(inst);
      break;
    case FMSUB:
      switch (fmt) {
      case FMT::S:
        fn = exec_FMSUB_S;
        valid = true;
        break;
      case FMT::D:
        fn = exec_FMSUB_D;
        valid = true;
        break;
      };
      rd = d_rd(inst);
      rs1 = d_rs1(inst);
      rs2 = d_rs2(inst);
      rs3 = d_rs3(inst);
      break;
    case FNMADD:
      switch (fmt) {
      case FMT::S:
        fn = exec_FNMADD_S;
        valid = true;
        break;
      case FMT::D:
        fn = exec_FNMADD_D;
        valid = true;
        break;
      };
      rd = d_rd(inst);
      rs1 = d_rs1(inst);
      rs2 = d_rs2(inst);
      rs3 = d_rs3(inst);
      break;
    case FNMSUB:
      switch (fmt) {
      case FMT::S:
        fn = exec_FNMSUB_S;
        valid = true;
        break;
      case FMT::D:
        fn = exec_FNMSUB_D;
        valid = true;
        break;
      };
      rd = d_rd(inst);
      rs1 = d_rs1(inst);
      rs2 = d_rs2(inst);
      rs3 = d_rs3(inst);
      break;
#endif

      // default: hart->cpu_trap(EXC_ILLEGAL_INSTRUCTION,inst,false); std::cout
      // << "[WARNING] Unknown instruction: " << inst << std::endl; valid =
      // true; break;
    //}
    // if(increase) pc += 4;

    /*rd = d_rd(inst);
    rs1 = d_rs1(inst);
    rs2 = d_rs2(inst);
    rs3 = d_rs3(inst);

    InstSubKeyInfo sk = pick_subkey(inst);
    uint32_t key = make_key(opcode, sk.funct3, sk.kind, sk.sub);

    if (sk.has_imm) {
        d_imm = sk.d_imm;
    }
    if (opcode == FENCE || opcode == B_TYPE || opcode == JAL || opcode == JALR || (opcode == ECALL && funct3 == 0))
      canChangePC = true;

    fn = inst_table[key];
    valid = fn != NULL;*/

    //cout << (void*)fn << " with key " <<key << " and inst " << inst << " and name " << inst_names[fn] << endl; 

    }
    
    #ifdef USE_FPU
    inst_data dec{valid, canChangePC, inst, rd, rs1, rs2, d_imm, fn, rm, fmt, rs3};
    #else
    inst_data dec{valid, canChangePC, inst, rd, rs1, rs2, d_imm, fn};
    #endif

    return dec;
  }
}

void instr_initialize() {
  add_inst_func(0x0000100f, exec_FENCE_I,"FENCE.I");
  add_inst_func(0x0000000f, exec_FENCE,"FENCE");

  add_inst_func(0x00000033, exec_ADD,"ADD");
  add_inst_func(0x02000033, exec_MUL,"MUL");
  add_inst_func(0x40000033, exec_SUB,"SUB");
  add_inst_func(0x00004033, exec_XOR,"XOR");
  add_inst_func(0x0A004033, exec_MIN,"MIN");
  add_inst_func(0x02004033, exec_DIV,"DIV");
  add_inst_func(0x20004033, exec_SH2ADD,"SH2ADD");
  add_inst_func(0x40004033, exec_XNOR,"XNOR");
  add_inst_func(0x00006033, exec_OR,"OR");
  add_inst_func(0x0A006033, exec_MAX,"MAX");
  add_inst_func(0x02006033, exec_REM,"REM");
  add_inst_func(0x20006033, exec_SH3ADD,"SH3ADD");
  add_inst_func(0x40006033, exec_ORN,"ORN");
  add_inst_func(0x00007033, exec_AND,"AND");
  add_inst_func(0x0A007033, exec_MAXU,"MAXU");
  add_inst_func(0x02007033, exec_REMU,"REMU");
  add_inst_func(0x40007033, exec_ANDN,"ANDN");
  add_inst_func(0x00001033, exec_SLL,"SLL");
  add_inst_func(0x02001033, exec_MULH,"MULH");
  add_inst_func(0x0A001033, exec_CLMUL,"CLMUL");
  add_inst_func(0x28001033, exec_BSET,"BSET");
  add_inst_func(0x48001033, exec_BCLR,"BCLR");
  add_inst_func(0x60001033, exec_ROL,"ROL");
  add_inst_func(0x00005033, exec_SRL,"SRL");
  add_inst_func(0x0A005033, exec_MINU,"MINU");
  add_inst_func(0x40005033, exec_SRA,"SRA");
  add_inst_func(0x48005033, exec_BEXT,"BEXT");
  add_inst_func(0x60005033, exec_ROR,"ROR");
  add_inst_func(0x02005033, exec_DIVU,"DIVU");
  add_inst_func(0x00002033, exec_SLT,"SLT");
  add_inst_func(0x02002033, exec_MULHSU,"MULHSU");
  add_inst_func(0x0A002033, exec_CLMULR,"CLMULR");
  add_inst_func(0x20002033, exec_SH1ADD,"SH1ADD");
  add_inst_func(0x68001033, exec_BINV,"BINV");
  add_inst_func(0x00003033, exec_SLTU,"SLTU");
  add_inst_func(0x02003033, exec_MULHU,"MULHU");
  add_inst_func(0x0A003033, exec_CLMULH,"CLMULH");

  add_inst_func(0x0000003b, exec_ADDW,"ADDW");
  add_inst_func(0x4000003b, exec_SUBW,"SUBW");
  add_inst_func(0x0200003b, exec_MULW,"MULW");
  add_inst_func(0x0800003B, exec_ADD_UW,"ADD.UW");
  add_inst_func(0x0000103b, exec_SLLW,"SLLW");
  add_inst_func(0x6000103B, exec_ROLW,"ROLW");
  add_inst_func(0x0000503b, exec_SRLW,"SRLW");
  add_inst_func(0x4000503b, exec_SRAW,"SRAW");
  add_inst_func(0x6000103B, exec_RORW,"RORW");
  add_inst_func(0x0200503b, exec_DIVUW,"DIVUW");
  add_inst_func(0x0200403b, exec_DIVW,"DIVW");
  add_inst_func(0x2000403B, exec_SH2ADD_UW,"SH2ADD.UW");
  add_inst_func(0x0200603b, exec_REMW,"REMW");
  add_inst_func(0x2000603B, exec_SH3ADD_UW,"SH3ADD.UW");
  add_inst_func(0x0200703b, exec_REMUW,"REMUW");
  add_inst_func(0x2000203B, exec_SH1ADD_UW,"SH1ADD.UW");

  add_inst_func(0x0000202f, exec_AMOADD_W,"AMOADD.W");
  add_inst_func(0x0800202f, exec_AMOSWAP_W,"AMOSWAP.W");
  add_inst_func(0x1000202f, exec_LR_W,"LR.W");
  add_inst_func(0x1800202f, exec_SC_W,"SC.W");
  add_inst_func(0x2000202f, exec_AMOXOR_W,"AMOXOR.W");
  add_inst_func(0x4000202f, exec_AMOOR_W,"AMOOR.W");
  add_inst_func(0x6000202f, exec_AMOAND_W,"AMOAND.W");
  add_inst_func(0x8000202f, exec_AMOMIN_W,"AMOMIN.W");
  add_inst_func(0xa000202f, exec_AMOMAX_W,"AMOMAX.W");
  add_inst_func(0xc000202f, exec_AMOMINU_W,"AMOMINU.W");
  add_inst_func(0xe000202f, exec_AMOMAXU_W,"AMOMAXU.W");

  add_inst_func(0x0000302f, exec_AMOADD_D,"AMOADD.D");
  add_inst_func(0x0800302f, exec_AMOSWAP_D,"AMOSWAP.D");
  add_inst_func(0x1000302f, exec_LR_D,"LR.D");
  add_inst_func(0x1800302f, exec_SC_D,"SC.D");
  add_inst_func(0x2000302f, exec_AMOXOR_D,"AMOXOR.D");
  add_inst_func(0x4000302f, exec_AMOOR_D,"AMOOR.D");
  add_inst_func(0x6000302f, exec_AMOAND_D,"AMOAND.D");
  add_inst_func(0x8000302f, exec_AMOMIN_D,"AMOMIN.D");
  add_inst_func(0xa000302f, exec_AMOMAX_D,"AMOMAX.D");
  add_inst_func(0xc000302f, exec_AMOMINU_D,"AMOMINU.D");
  add_inst_func(0xe000302f, exec_AMOMAXU_D,"AMOMAXU.D");

  add_inst_func(0x00000013, exec_ADDI,"ADDI");
  add_inst_func(0x00004013, exec_XORI,"XORI");
  add_inst_func(0x00006013, exec_ORI,"ORI");
  add_inst_func(0x00007013, exec_ANDI,"ANDI");
  add_inst_func(0x00001013, exec_SLLI,"SLLI");
  add_inst_func(0x28001013, exec_BSETI,"BSETI");
  add_inst_func(0x68001013, exec_BINVI,"BINVI");
  add_inst_func(0x48001013, exec_BCLRI,"BCLRI");
  add_inst_func(0x60001013, exec_CLZ,"CLZ");
  add_inst_func(0x60081013, exec_CTZ,"CTZ");
  add_inst_func(0x60101013, exec_CPOP,"CPOP");
  add_inst_func(0x60201013, exec_SEXT_B,"SEXT.B");
  add_inst_func(0x60281013, exec_SEXT_H,"SEXT.H");
  add_inst_func(0x00005013, exec_SRLI,"SRLI");
  add_inst_func(0x40005013, exec_SRAI,"SRAI");
  add_inst_func(0x48005013, exec_BEXTI,"BEXTI");
  add_inst_func(0x60005013, exec_RORI,"RORI");
  add_inst_func(0x28705013, exec_ORC_B,"ORC.B");
  add_inst_func(0x69805013, exec_REV8,"REV8"); //broken
  add_inst_func(0x00002013, exec_SLTI,"SLTI");
  add_inst_func(0x00003013, exec_SLTIU,"SLTIU");

  add_inst_func(0x0800403B, exec_ZEXT_H,"ZEXT.H");
  add_inst_func(0x0000001b, exec_ADDIW,"ADDIW");
  add_inst_func(0x0800101B, exec_SLLI_UW,"SLLI.UW");
  add_inst_func(0x0000101b, exec_SLLIW,"SLLIW");
  add_inst_func(0x6000101B, exec_CLZW,"CLZW");
  add_inst_func(0x6010101B, exec_CTZW,"CTZW");
  add_inst_func(0x6020101B, exec_CPOPW,"CPOPW");
  add_inst_func(0x0000501b, exec_SRLIW,"CRLIW");
  add_inst_func(0x4000501b, exec_SRAIW,"SRAIW");
  add_inst_func(0x6000501B, exec_RORIW,"RORIW");

  add_inst_func(0x00000003, exec_LB,"LB");
  add_inst_func(0x00001003, exec_LH,"LH");
  add_inst_func(0x00002003, exec_LW,"LW");
  add_inst_func(0x00003003, exec_LD,"LD");
  add_inst_func(0x00004003, exec_LBU,"LBU");
  add_inst_func(0x00005003, exec_LHU,"LHU");
  add_inst_func(0x00006003, exec_LWU,"LWU");

  add_inst_func(0x00000023, exec_SB,"SB");
  add_inst_func(0x00001023, exec_SH,"SH");
  add_inst_func(0x00002023, exec_SW,"SW");
  add_inst_func(0x00003023, exec_SD,"SD");

  add_inst_func(0x00000063, exec_BEQ,"BEQ");
  add_inst_func(0x00001063, exec_BNE,"BNE");
  add_inst_func(0x00004063, exec_BLT,"BLT");
  add_inst_func(0x00005063, exec_BGE,"BGE");
  add_inst_func(0x00006063, exec_BLTU,"BLTU");
  add_inst_func(0x00007063, exec_BGEU,"BGEU");

  add_inst_func(0x0000006f, exec_JAL,"JAL");
  add_inst_func(0x00000067, exec_JALR,"JALR");
  add_inst_func(0x00000037, exec_LUI,"LUI");
  add_inst_func(0x00000017, exec_AUIPC,"AUIPC");

  add_inst_func(0x00001073, exec_CSRRW,"CSRRW");
  add_inst_func(0x00002073, exec_CSRRS,"CSRRS");
  add_inst_func(0x00003073, exec_CSRRC,"CSRRC");
  add_inst_func(0x00005073, exec_CSRRWI,"CSRRWI");
  add_inst_func(0x00006073, exec_CSRRSI,"CSRRSI");
  add_inst_func(0x00007073, exec_CSRRCI,"CSRRCI");
  add_inst_func(0x00000073, exec_ECALL,"ECALL");
  add_inst_func(0x00100073, exec_EBREAK,"EBREAK");
  add_inst_func(0x10500073, exec_WFI,"WFI");
  add_inst_func(0x10200073, exec_SRET,"SRET");
  add_inst_func(0x12000073, exec_SFENCE_VMA,"SFENCE.VMA");
  add_inst_func(0x30200073, exec_MRET,"MRET");

  #ifdef USE_FPU
    add_inst_func(0x00002007, exec_FLW,"FLW");
    add_inst_func(0x00003007, exec_FLD,"FLD");
    add_inst_func(0x00002027, exec_FSW,"FSW");
    add_inst_func(0x00003027, exec_FSD,"FSD");

    add_inst_func(0x00000053, exec_FADD_S,"FADD.S");
    add_inst_func(0x02000053, exec_FADD_D,"FADD.D");
    add_inst_func(0x08000053, exec_FSUB_S,"FSUB.S");
    add_inst_func(0x0a000053, exec_FSUB_D,"FSUB.D");
    add_inst_func(0x10000053, exec_FMUL_S,"FMUL.S");
    add_inst_func(0x12000053, exec_FMUL_D,"FMUL.D");
    add_inst_func(0x18000053, exec_FDIV_S,"FDIV.S");
    add_inst_func(0x1a000053, exec_FDIV_D,"FDIV.D");
    add_inst_func(0x58000053, exec_FSQRT_S,"FSQRT.S");
    add_inst_func(0x5a000053, exec_FSQRT_D,"FSQRT.D");

    add_inst_func(0x28000053, exec_FMIN_S,"FMIN.S");
    add_inst_func(0x2a000053, exec_FMIN_D,"FMIN.D");
    add_inst_func(0x28001053, exec_FMAX_S,"FMAX.S");
    add_inst_func(0x2a001053, exec_FMAX_D,"FMAX.D");

    add_inst_func(0xc0000053, exec_FCVT_W_S,"FCVT.W.S");
    add_inst_func(0xc2000053, exec_FCVT_W_D,"FCVT.W.D");
    add_inst_func(0xc0100053, exec_FCVT_WU_S,"FCVT.WU.S");
    add_inst_func(0xc2100053, exec_FCVT_WU_D,"FCVT.WU.D");
    add_inst_func(0xc0200053, exec_FCVT_L_S,"FCVT.L.S");
    add_inst_func(0xc2200053, exec_FCVT_L_D,"FCVT.L.D");
    add_inst_func(0xc0300053, exec_FCVT_LU_S,"FCVT.LU.S");
    add_inst_func(0xc2300053, exec_FCVT_LU_D,"FCVT.LU.D");

    add_inst_func(0xd0000053, exec_FCVT_S_W,"FCVT.S.W");
    add_inst_func(0xd2000053, exec_FCVT_D_W,"FCVT.D.W");
    add_inst_func(0xd0100053, exec_FCVT_S_WU,"FCVT.S.WU");
    add_inst_func(0xd2100053, exec_FCVT_D_WU,"FCVT.D.WU");
    add_inst_func(0xd0200053, exec_FCVT_S_L,"FCVT.S.L");
    add_inst_func(0xd2200053, exec_FCVT_D_L,"FCVT.D.L");
    add_inst_func(0xd0300053, exec_FCVT_S_LU,"FCVT.S.LU");
    add_inst_func(0xd2300053, exec_FCVT_D_LU,"FCVT.D.LU");

    add_inst_func(0x20000053, exec_FSGNJ_S,"FSGNJ.S");
    add_inst_func(0x22000053, exec_FSGNJ_D,"FSGNJ.D");
    add_inst_func(0x20001053, exec_FSGNJN_S,"FSGNJN.S");
    add_inst_func(0x22001053, exec_FSGNJN_D,"FSGNJN.D");
    add_inst_func(0x20002053, exec_FSGNJX_S,"FSGNJX.S");
    add_inst_func(0x22002053, exec_FSGNJX_D,"FSGNJX.D");

    add_inst_func(0xe0000053, exec_FMV_X_W,"FMV.X.W");
    add_inst_func(0xe2000053, exec_FMV_X_D,"FMV.X.D");
    add_inst_func(0xe0001053, exec_FCLASS_S,"FCLASS.S");
    add_inst_func(0xe2001053, exec_FCLASS_D,"FCLASS.D");
    add_inst_func(0xf0000053, exec_FMV_W_X,"FMV.W.X");
    add_inst_func(0xf2000053, exec_FMV_D_X,"FMV.D.X");

    add_inst_func(0xa0000053, exec_FLE_S,"FLE.S");
    add_inst_func(0xa2000053, exec_FLE_D,"FLE.D");
    add_inst_func(0xa0001053, exec_FLT_S,"FLT.S");
    add_inst_func(0xa2001053, exec_FLT_D,"FLT.D");
    add_inst_func(0xa0002053, exec_FEQ_S,"FEQ.S");
    add_inst_func(0xa2002053, exec_FEQ_D,"FEQ.D");

    add_inst_func(0x00000043, exec_FMADD_S,"FMADD.S");
    add_inst_func(0x02000043, exec_FMADD_D,"FMADD.D");
    add_inst_func(0x00000047, exec_FMSUB_S,"FMSUB.S");
    add_inst_func(0x02000047, exec_FMSUB_D,"FMSUB.D");
    add_inst_func(0x0000004f, exec_FNMADD_S,"FNMADD.S");
    add_inst_func(0x0200004f, exec_FNMADD_D,"FNMADD.D");
    add_inst_func(0x0000004b, exec_FNMSUB_S,"FNMSUB.S");
    add_inst_func(0x0200004b, exec_FNMSUB_D,"FNMSUB.D");
  #endif
}