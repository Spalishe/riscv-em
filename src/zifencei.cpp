#include "../include/instset.h"
#include "../include/cpu.h"
#include "../include/csr.h"
#include <iostream>

void exec_FENCE_I(struct HART *hart, uint32_t inst) {
    hart->instr_cache.clear();
    hart->instr_block_cache.clear();
    hart->print_d("{0x%.8X} [FENCE.I] uhh i cant figure out how fence memory btw so i just clear cache",hart->pc);
}

void exec_SFENCE_VMA(struct HART *hart, uint32_t inst) {
    hart->print_d("{0x%.8X} [SFENCE.VMA] there is no TLB cache rn; nop",hart->pc);
}