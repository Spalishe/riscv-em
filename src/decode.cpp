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

#include "../include/cpu.h"
#include "../include/decode.h"
#include <stdint.h>

uint64_t d_rd(uint32_t inst) {
    return (inst >> 7) & 0x1f;    // rd in bits 11..7
}
uint64_t d_rs1(uint32_t inst) {
    return (inst >> 15) & 0x1f;   // rs1 in bits 19..15
}
uint64_t d_rs2(uint32_t inst) {
    return (inst >> 20) & 0x1f;   // rs2 in bits 24..20
}
uint64_t imm_Zicsr(uint32_t inst) {
    return (inst >> 20);
}
uint64_t imm_I(uint32_t inst) {
    // imm[11:0] = inst[31:20]
    return sext(inst>>20,12);
}
uint64_t imm_S(uint32_t inst) {
    // imm[11:5] = inst[31:25], imm[4:0] = inst[11:7]
    return sext((get_bits(inst,11,7) | (get_bits(inst,31,25) << 5)),12);
}
uint64_t imm_B(uint32_t inst) {
    // imm[12|10:5|4:1|11] = inst[31|30:25|11:8|7]
    return sext(((get_bits(inst,11,8) << 1) | (get_bits(inst,30,25) << 5) | (get_bits(inst,7,7) << 11) | (get_bits(inst,31,31) << 12)),13);
}
uint64_t imm_U(uint32_t inst) {
    // imm[31:12] = inst[31:12]
    return sext(inst>>12,20);
}
uint64_t imm_J(uint32_t inst) {
    // imm[20|10:1|11|19:12] = inst[31|30:21|20|19:12]
    return sext((get_bits(inst, 30,21) << 1)
        | (get_bits(inst, 20,20) << 11) 
        | (get_bits(inst, 19,12) << 12) 
        | (get_bits(inst, 31,31) << 20),21);
}
uint64_t C_imm_J(uint16_t inst) {
    return ((get_bits(inst, 12, 12) << 11) |
        (get_bits(inst, 11, 11) << 4)  |
        (get_bits(inst, 10, 9) << 8)   |
        (get_bits(inst, 8, 8) << 10)   |
        (get_bits(inst, 7, 7) << 6)    |
        (get_bits(inst, 6, 6) << 7)    |
        (get_bits(inst, 5, 3) << 1)    |
        (get_bits(inst, 2, 2) << 5));
}
uint32_t shamt(uint32_t inst) {
    // shamt(shift amount) only required for immediate shift instructions
    // shamt[4:5] = imm[5:0]
    return (uint32_t) (imm_I(inst) & 0x1f);
}
uint32_t shamt64(uint32_t inst) {
    // shamt(shift amount) only required for immediate shift instructions
    // shamt[4:5] = imm[5:0]
    return (uint32_t) (imm_I(inst) & 0x3f);
}

int32_t sext(uint32_t val, int bits) {
    int32_t shift = 32 - bits;
    return (int32_t)(val << shift) >> shift;
}
uint32_t get_bits(uint32_t inst, int hi, int lo) {
    return (inst >> lo) & ((1u << (hi - lo + 1)) - 1);
}

inst_data parse_instruction(struct HART* hart, uint32_t inst, uint64_t pc) {
	int OP = inst & 3;

    uint8_t rd = 0;
    uint8_t rs1 = 0;
    uint8_t rs2 = 0;
    int64_t imm = 0;

    bool valid = false;
	void (*fn)(HART*, inst_data&);

    inst_data cache = hart->instr_cache[(pc >> 2) & 0x1FFF];
    if(cache.valid && cache.pc == pc) {
        return cache;
    } else {
        int opcode = inst & 0x7f;
        int funct3 = (inst >> 12) & 0x7;
        int amo_funct5 = (inst >> 27) & 0x1F;
        int funct7 = (inst >> 25) & 0x7f;
        int funct6 = (inst >> 26);
        int imm = (inst >> 20);
        switch(opcode) {
            /*case FENCE:
                switch(funct3) {
                    case 1: fn = exec_FENCE_I; junction = true; valid = true; break;
                    case 0: fn = exec_FENCE; junction = true; valid = true; break;
                }*/
            case R_TYPE:
                switch(funct3) {
                    case ADDSUB:
                        switch(funct7) {
                            case ADD: fn = exec_ADD; valid = true; break;
                            //case 1: fn = exec_MUL; valid = true; break;
                            case SUB: fn = exec_SUB; valid = true; break;
                        }; break;
                    case XOR: 
                        switch(funct7) {
                            case 0: fn = exec_XOR; valid = true; break;
                            //case 1: fn = exec_DIV; valid = true; break;
                        }; break;
                    case OR: 
                        switch(funct7) {
                            case 0: fn = exec_OR; valid = true; break;
                            //case 1: fn = exec_REM; valid = true; break;
                        };  valid = true; break;
                    case AND: 
                        switch(funct7) {
                            case 0: fn = exec_AND; valid = true; break;
                            //case 1: fn = exec_REMU; valid = true; break;
                        }; break;
                    case SLL: 
                        switch(funct7) {
                            case 0: fn = exec_SLL; valid = true; break;
                            //case 1: fn = exec_MULH; valid = true; break;
                        }; break;
                    case SR:
                        switch(funct7) {
                            case SRL: fn = exec_SRL; valid = true; break;
                            case SRA: fn = exec_SRA; valid = true; break;
                            //case 1: fn = exec_DIVU; valid = true; break;
                        }; break;
                    case SLT: 
                        switch(funct7) {
                            case 0: fn = exec_SLT; valid = true; break;
                            //case 1: fn = exec_MULHSU; valid = true; break;
                        }; break;
                    case SLTU: 
                        switch(funct7) {
                            case 0: fn = exec_SLTU; valid = true; break;
                            //case 1: fn = exec_MULHU; valid = true; break;
                        }; break;
                };
                rs1 = d_rs1(inst);
                rs2 = d_rs2(inst);
                rd = d_rd(inst);
                break;
            case R_TYPE64:
                switch(funct3) {
                    case ADDSUB:
                        switch(funct7) {
                            case ADD: fn = exec_ADDW; valid = true; break;
                            case SUB: fn = exec_SUBW; valid = true; break;
                            //case 1: fn = exec_MULW; valid = true; break;
                        }; break;
                    case SLL: fn = exec_SLLW; valid = true; break;
                    case SR:
                        switch(funct7) {
                            case SRL: fn = exec_SRLW; valid = true; break;
                            case SRA: fn = exec_SRAW; valid = true; break;
                            //case 1: fn = exec_DIVUW; valid = true; break;
                        }; break;
                    //case XOR: fn = exec_DIVW; valid = true; break;
                    //case OR: fn = exec_REMW; valid = true; break;
                    //case AND: fn = exec_REMUW; valid = true; break;
                };
                rs1 = d_rs1(inst);
                rs2 = d_rs2(inst);
                rd = d_rd(inst);
                break;
            /*case AMO:
                switch(funct3) {
                    case AMO_W: 
                        switch(amo_funct5) {
                            case AMOADD: fn = exec_AMOADD_W; valid = true; break;
                            case AMOSWAP: fn = exec_AMOSWAP_W; valid = true; break;
                            case LR: fn = exec_LR_W; valid = true; break;
                            case SC: fn = exec_SC_W; valid = true; break;
                            case AMOXOR: fn = exec_AMOXOR_W; valid = true; break;
                            case AMOOR: fn = exec_AMOOR_W; valid = true; break;
                            case AMOAND: fn = exec_AMOAND_W; valid = true; break;
                            case AMOMIN: fn = exec_AMOMIN_W; valid = true; break;
                            case AMOMAX: fn = exec_AMOMAX_W; valid = true; break;
                            case AMOMINU: fn = exec_AMOMINU_W; valid = true; break;
                            case AMOMAXU: fn = exec_AMOMAXU_W; valid = true; break;
                        }; valid = true; break;
                    case AMO_D: 
                        switch(amo_funct5) {
                            case AMOADD: fn = exec_AMOADD_D; valid = true; break;
                            case AMOSWAP: fn = exec_AMOSWAP_D; valid = true; break;
                            case LR: fn = exec_LR_D; valid = true; break;
                            case SC: fn = exec_SC_D; valid = true; break;
                            case AMOXOR: fn = exec_AMOXOR_D; valid = true; break;
                            case AMOOR: fn = exec_AMOOR_D; valid = true; break;
                            case AMOAND: fn = exec_AMOAND_D; valid = true; break;
                            case AMOMIN: fn = exec_AMOMIN_D; valid = true; break;
                            case AMOMAX: fn = exec_AMOMAX_D; valid = true; break;
                            case AMOMINU: fn = exec_AMOMINU_D; valid = true; break;
                            case AMOMAXU: fn = exec_AMOMAXU_D; valid = true; break;
                        }; valid = true; break;
                }; valid = true; break;*/
            case I_TYPE:
                switch(funct3) {
                    case ADDI: fn = exec_ADDI; imm = imm_I(inst); valid = true; break;
                    case XORI: fn = exec_XORI; imm = imm_I(inst); valid = true; break;
                    case ORI: fn = exec_ORI; imm = imm_I(inst); valid = true; break;
                    case ANDI: fn = exec_ANDI; imm = imm_I(inst); valid = true; break;
                    case SLLI: fn = exec_SLLI; imm = shamt64(inst); valid = true; break;
                    case SRI:
                        /*switch(funct7) {
                            case SRLI: fn = exec_SRLI; valid = true; break;
                            case SRAI: fn = exec_SRAI; valid = true; break;
                        }; break;*/
                        switch(funct6) {
                            case SRLI: fn = exec_SRLI; imm = shamt64(inst); valid = true; break;
                            case SRAI: fn = exec_SRAI; imm = shamt64(inst); valid = true; break;
                        }; break;
                    case SLTI: fn = exec_SLTI; imm = imm_I(inst); valid = true; break;
                    case SLTIU: fn = exec_SLTIU; imm = imm_I(inst); valid = true; break;
                };
                rs1 = d_rs1(inst);
                rd = d_rd(inst);
                break;
            case I_TYPE64:
                switch(funct3) {
                    case ADDI: fn = exec_ADDIW; imm = imm_I(inst); valid = true; break;
                    case SLLI: fn = exec_SLLIW; imm = shamt(inst); valid = true; break;
                    case SRI:
                        switch(funct7) {
                            case SRLI: fn = exec_SRLIW; imm = shamt(inst); valid = true; break;
                            case SRAIW: fn = exec_SRAIW; imm = shamt(inst); valid = true; break;
                        }; break;
                };
                rs1 = d_rs1(inst);
                rd = d_rd(inst);
                break;
            case LOAD_TYPE:
                switch(funct3) {
                    case LB: fn = exec_LB; valid = true; break;
                    case LH: fn = exec_LH; valid = true; break;
                    case LW: fn = exec_LW; valid = true; break;
                    case LD: fn = exec_LD; valid = true; break;
                    case LBU: fn = exec_LBU; valid = true; break;
                    case LHU: fn = exec_LHU; valid = true; break;
                    case LWU: fn = exec_LWU; valid = true; break;
                };
                rs1 = d_rs1(inst);
                imm = imm_I(inst);
                rd = d_rd(inst);
                break;
            case S_TYPE:
                switch(funct3) {
                    case SB: fn = exec_SB; valid = true; break;
                    case SH: fn = exec_SH; valid = true; break;
                    case SW: fn = exec_SW; valid = true; break;
                    case SD: fn = exec_SD; valid = true; break;
                };
                rs1 = d_rs1(inst);
                rs2 = d_rs2(inst);
                imm = imm_S(inst);
                break;
            case B_TYPE:
                switch(funct3) {
                    case BEQ: fn = exec_BEQ; valid = true; break;
                    case BNE: fn = exec_BNE; valid = true; break;
                    case BLT: fn = exec_BLT; valid = true; break;
                    case BGE: fn = exec_BGE; valid = true; break;
                    case BLTU: fn = exec_BLTU; valid = true; break;
                    case BGEU: fn = exec_BGEU; valid = true; break;
                };
                rs1 = d_rs1(inst);
                rs2 = d_rs2(inst);
                imm = imm_B(inst);
                break;
            case JAL: 
                fn = exec_JAL;
                rd = d_rd(inst);
                imm = imm_J(inst);
                break;
            case JALR:
                fn = exec_JALR; 
                rd = d_rd(inst);
                rs1 = d_rs1(inst);
                imm = imm_J(inst);
                break;
            case LUI:
                fn = exec_LUI;
                rd = d_rd(inst);
                imm = imm_U(inst);
                break;
            case AUIPC:
                fn = exec_AUIPC;
                rd = d_rd(inst);
                imm = imm_U(inst);
                break;
            case ECALL: 
                switch(funct3) {
                    case CSRRW: fn = exec_CSRRW; rd = d_rd(inst); rs1 = d_rs1(inst); imm = imm_Zicsr(inst); valid = true; break;
                    case CSRRS: fn = exec_CSRRS; rd = d_rd(inst); rs1 = d_rs1(inst); imm = imm_Zicsr(inst); valid = true; break;
                    case CSRRC: fn = exec_CSRRC; rd = d_rd(inst); rs1 = d_rs1(inst); imm = imm_Zicsr(inst); valid = true; break;
                    case CSRRWI: fn = exec_CSRRWI; rd = d_rd(inst); rs1 = d_rs1(inst); imm = imm_Zicsr(inst); valid = true; break;
                    case CSRRSI: fn = exec_CSRRSI; rd = d_rd(inst); rs1 = d_rs1(inst); imm = imm_Zicsr(inst); valid = true; break;
                    case CSRRCI: fn = exec_CSRRCI; rd = d_rd(inst); rs1 = d_rs1(inst); imm = imm_Zicsr(inst); valid = true; break;
                    case 0:
                        switch(imm) {
                            case 0: fn = exec_ECALL; valid = true; break;
                            case 1: fn = exec_EBREAK; valid = true; break;
                            //case 261: fn = exec_WFI; valid = true; break;
                            //case 258: fn = exec_SRET; valid = true; break;
                            //case 288: fn = exec_SFENCE_VMA; valid = true; break;
                            //case 770: fn = exec_MRET; valid = true; break;
                        }; break;
                }; break;

            //default: hart->cpu_trap(EXC_ILLEGAL_INSTRUCTION,inst,false); std::cout << "[WARNING] Unknown instruction: " << inst << std::endl; valid = true; break;
        }
        //if(increase) pc += 4;
        
        inst_data dec = inst_data{valid, pc, inst,rd,rs1,rs2,imm,fn};
        hart->instr_cache[(pc >> 2) & 0x1FFF] = dec;
        return dec;
    }
}