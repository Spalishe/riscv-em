#include "../include/instset.h"
#include "../include/cpu.h"
#include "../include/csr.h"
#include <iostream>

void exec_FENCE_I(struct HART *hart, uint32_t inst) {
    print_d(hart,"{0x%.8X} [FENCE.I] uhh i cant figure out how fence memory btw",hart->pc);
}