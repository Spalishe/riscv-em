#include "../include/instset.h"
#include "../include/cpu.h"
#include "../include/csr.h"
#include <iostream>

void exec_C_LDSP(struct HART *hart, uint32_t inst) {
    uint64_t rd = get_bits(inst,11,7);

    uint64_t imm = (get_bits(inst, 12, 12) << 5) | (( get_bits(inst, 6, 4)) << 2) + (((inst >> 2) & 3) * 64);
    imm = sext(imm,9);

	std::optional<uint64_t> val = hart->mmio->load(hart,(hart->regs[2]+imm),64);
	if(val.has_value()) {
		hart->regs[rd] = *val;
	}
    
	print_d(hart,"{0x%.8X} [C.LDSP] rd: %d; imm: int %d uint %u",hart->pc,rd,(int64_t) imm,imm);
}
void exec_C_SDSP(struct HART *hart, uint32_t inst) {
    uint64_t rs2 = get_bits(inst,7,2);

    uint64_t imm = (( get_bits(inst, 13, 9)) << 2) + (get_bits(inst,8,7) * 64);
    imm = sext(imm,9);

	hart->mmio->store(hart,(hart->regs[2]+imm),64,hart->regs[rs2]);
    
	print_d(hart,"{0x%.8X} [C.SDSP] rd: %d; imm: int %d uint %u",hart->pc,rd,(int64_t) imm, imm);
}
void exec_C_LD(struct HART *hart, uint32_t inst) {
    uint64_t rd = 8+get_bits(inst,5,3);
    uint64_t rs1 = 8+get_bits(inst,9,7);

    uint64_t imm1 = get_bits(inst,5,5);
    uint64_t imm2 = get_bits(inst,6,6);
    uint64_t imm3 = get_bits(inst,12,10);

    uint64_t imm = imm1 * 64 + (imm2 | (imm3 << 1));

	std::optional<uint64_t> val = hart->mmio->load(hart,(hart->regs[rs1]+imm),64);
	if(val.has_value()) {
		hart->regs[rd] = *val;
	}
    
	print_d(hart,"{0x%.8X} [C.LD] rs1 %d; rd: %d; imm: int %d uint %u",hart->pc,rs1,rd,(int64_t) imm, imm);
}
void exec_C_SD(struct HART *hart, uint32_t inst) {
    uint64_t rs2 = 8+get_bits(inst,5,3);
    uint64_t rs1 = 8+get_bits(inst,9,7);

    uint64_t imm1 = get_bits(inst,5,5);
    uint64_t imm2 = get_bits(inst,6,6);
    uint64_t imm3 = get_bits(inst,12,10);

    uint64_t imm = imm1 * 64 + (imm2 | (imm3 << 1));

	hart->mmio->store(hart,(hart->regs[rs1]+imm),64,hart->regs[rs2]);
    
	print_d(hart,"{0x%.8X} [C.SD] rs1 %d; rs2: %d; imm: int %d uint %u",hart->pc,rs1,rs2,(int64_t) imm, imm);
}
void exec_C_ADDIW(struct HART *hart, uint32_t inst) {
    uint64_t rd = get_bits(inst,7,11);

    uint64_t imm = (get_bits(inst, 12, 12) << 5) | get_bits(inst, 6, 2);
    imm = sext(imm,6);

	hart->regs[rd] += (int32_t) imm;
    
	print_d(hart,"{0x%.8X} [C.ADDIW] rd: %d; imm: int %d uint %u",hart->pc,rd,(int64_t) imm, imm);
}
void exec_C_SUBW(struct HART *hart, uint32_t inst) {
    uint64_t rd = 8+get_bits(inst,5,3);
    uint64_t rs2 = 8+get_bits(inst,9,7);

	hart->regs[rd] -= (int32_t) hart->regs[rs2];
    
	print_d(hart,"{0x%.8X} [C.SUBW] rd %d; rs2: %d",hart->pc,rd,rs2);
}
void exec_C_ADDW(struct HART *hart, uint32_t inst) {
    uint64_t rd = get_bits(inst,11,7);
    uint64_t rs2 = get_bits(inst,6,2);

    hart->regs[rd] += (int32_t) hart->regs[rs2];
    
	print_d(hart,"{0x%.8X} [C.ADDW] rd: %d; rs2: %d",hart->pc,rd,rs2);
}