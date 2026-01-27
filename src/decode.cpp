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

#include "../include/cpu.h"
#include "../include/opcodes.h"
#include <stdint.h>

uint64_t rd(uint32_t inst) {
    return (inst >> 7) & 0x1f;    // rd in bits 11..7
}
uint64_t rs1(uint32_t inst) {
    return (inst >> 15) & 0x1f;   // rs1 in bits 19..15
}
uint64_t rs2(uint32_t inst) {
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