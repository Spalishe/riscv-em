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

#include "../include/instset.h"
#include "../include/cpu.h"
#include "../include/csr.h"
#include <iostream>

void exec_WFI(struct HART *hart, uint32_t inst) {
    hart->print_d("{0x%.8X} [WFI] waiting for interrupt...",hart->pc);
    hart->stopexec = true;
}

inline uint64_t bit_set_to(uint64_t number, uint64_t n, bool x) {
    return (number & ~((uint64_t)1 << n)) | ((uint64_t)x << n);
}
inline bool bit_check(uint64_t number, uint64_t n) {
    return (number >> n) & (uint64_t)1;
}

void exec_SRET(struct HART *hart, uint32_t inst) {
    if(hart->mode != 0) {
        hart->cpu_trap(EXC_ILLEGAL_INSTRUCTION,hart->mode,false);
        return;
    }

    hart->csrs[SSTATUS] = bit_set_to(hart->csrs[SSTATUS],1,bit_check(hart->csrs[SSTATUS],5));
    hart->csrs[SSTATUS] = bit_set_to(hart->csrs[SSTATUS],5,true);
    hart->csrs[SSTATUS] = bit_set_to(hart->csrs[SSTATUS],8,false);
    hart->pc = hart->csrs[SEPC];

    hart->trap_active = false;
    hart->print_d("{0x%.8X} [SRET] ahh returned from exc",hart->pc);
}
void exec_MRET(struct HART *hart, uint32_t inst) {
    if(get_bits(hart->csrs[MSTATUS],12,11) != 3) {
        hart->csrs[MSTATUS] = bit_set_to(hart->csrs[MSTATUS],17,false);
    }

    hart->csrs[MSTATUS] = bit_set_to(hart->csrs[MSTATUS],3,bit_check(hart->csrs[MSTATUS],7));
    hart->csrs[SSTATUS] = bit_set_to(hart->csrs[SSTATUS],7,true);

    if(get_bits(hart->csrs[MSTATUS],12,11) == 0) {
        hart->mode = 0;
    } else if(get_bits(hart->csrs[MSTATUS],12,11) == 1) {
        hart->mode = 1;
    } else if(get_bits(hart->csrs[MSTATUS],12,11) == 3) {
        hart->mode = 3;
    }

    hart->csrs[SSTATUS] = bit_set_to(hart->csrs[SSTATUS],11,false);
    hart->csrs[SSTATUS] = bit_set_to(hart->csrs[SSTATUS],12,false);

    hart->pc = hart->csrs[MEPC];
    
    hart->trap_active = false;
    hart->print_d("{0x%.8X} [MRET] ahh returned from exc",hart->pc);
}