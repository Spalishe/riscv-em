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

static inline bool bit(uint64_t x, unsigned pos) { return (x >> pos) & 1ULL; }
bool interrupt_pending_and_enabled(struct HART *hart, bool for_wfi = false) {
    uint64_t mip      = hart->csrs[MIP];
    uint64_t mie      = hart->csrs[MIE];
    uint64_t mideleg  = hart->csrs[MIDELEG];
    uint64_t mstatus  = hart->csrs[MSTATUS];
    uint64_t sstatus  = hart->csr_read(SSTATUS);
    uint64_t sie      = hart->csr_read(SIE);

    uint64_t pending_locally_enabled = mip & mie;
    if (for_wfi) {
        return (pending_locally_enabled != 0);
    }

    uint64_t pending_enabled = pending_locally_enabled;

    int mode = (int)hart->mode; // 3=M, 1=S, 0=U

    bool m_glob = bit(mstatus, MSTATUS_MIE_BIT);
    if (mode == 3) { // M-mode
        uint64_t m_pending = pending_enabled & ~mideleg;
        return (m_glob && (m_pending != 0));
    }

    bool s_glob = bit(sstatus, SSTATUS_SIE_BIT);
    if (mode == 1) { // S-mode
        uint64_t s_pending = pending_enabled & mideleg;
        return (s_glob && ((s_pending & sie) != 0));
    }

    if (mode == 0) { // U-mode
        return (pending_enabled != 0);
    }

    return false;
}


void exec_WFI(struct HART *hart, uint32_t inst, CACHE_DecodedOperands* opers, std::tuple<llvm::IRBuilder<>*, llvm::Function*, llvm::Value*>* jitd) {
    uint64_t mstatus = hart->csrs[MSTATUS];
    bool tw = (mstatus >> 21) & 1;
    
    if (hart->mode == 0) { // U
        goto trapcpu;
    }
    if (tw) {
        if(hart->mode != 3) {
            goto trapcpu;
        }
    }

    if(!interrupt_pending_and_enabled(hart,true)) {
        if(hart->dbg) hart->print_d("{0x%.8X} [WFI] waiting for interrupt...",hart->pc);
        hart->stopexec = true;
    } else {
        if(hart->dbg) hart->print_d("{0x%.8X} [WFI] nop",hart->pc);
    }
    return;

    trapcpu:
        if(hart->trap_active) {
            if(hart->dbg) hart->print_d("{0x%.8X} [WFI] nop",hart->pc);
            return;
        } else {
            hart->cpu_trap(EXC_ILLEGAL_INSTRUCTION,0,false);
            return;
        }
}

inline uint64_t bit_set_to(uint64_t number, uint64_t n, bool x) {
    return (number & ~((uint64_t)1 << n)) | ((uint64_t)x << n);
}
inline bool bit_check(uint64_t number, uint64_t n) {
    return (number >> n) & (uint64_t)1;
}

void exec_SRET(struct HART *hart, uint32_t inst, CACHE_DecodedOperands* opers, std::tuple<llvm::IRBuilder<>*, llvm::Function*, llvm::Value*>* jitd) {
    if(hart->mode == 0) {
        hart->cpu_trap(EXC_ILLEGAL_INSTRUCTION,hart->mode,false);
        return;
    }

    if(get_bits(hart->csr_read(SSTATUS),8,8) == 1) {
        hart->mode = 1;
    } else {
        hart->mode = 0;
    }

    hart->csr_write(SSTATUS, bit_set_to(hart->csr_read(SSTATUS),1,bit_check(hart->csr_read(SSTATUS),5)));
    hart->csr_write(SSTATUS, bit_set_to(hart->csr_read(SSTATUS),5,true));
    hart->csr_write(SSTATUS, bit_set_to(hart->csr_read(SSTATUS),8,false));
    hart->pc = hart->csr_read(SEPC);

    hart->trap_active = false;
    if(hart->dbg) hart->print_d("{0x%.8X} [SRET] ahh returned from exc",hart->pc);
}
void exec_MRET(struct HART *hart, uint32_t inst, CACHE_DecodedOperands* opers, std::tuple<llvm::IRBuilder<>*, llvm::Function*, llvm::Value*>* jitd) {
    if(get_bits(hart->csrs[MSTATUS],12,11) != 3) {
        hart->csrs[MSTATUS] = bit_set_to(hart->csrs[MSTATUS],17,false);
    }

    hart->csrs[MSTATUS] = bit_set_to(hart->csrs[MSTATUS],3,bit_check(hart->csrs[MSTATUS],7));
    hart->csrs[MSTATUS] = bit_set_to(hart->csrs[MSTATUS],7,true);

    if(get_bits(hart->csrs[MSTATUS],12,11) == 0) {
        hart->mode = 0;
    } else if(get_bits(hart->csrs[MSTATUS],12,11) == 1) {
        hart->mode = 1;
    } else if(get_bits(hart->csrs[MSTATUS],12,11) == 3) {
        hart->mode = 3;
    }

    hart->csrs[MSTATUS] = bit_set_to(hart->csrs[MSTATUS],11,false);
    hart->csrs[MSTATUS] = bit_set_to(hart->csrs[MSTATUS],12,false);

    hart->pc = hart->csrs[MEPC];
    
    hart->trap_active = false;
    if(hart->dbg) hart->print_d("{0x%.8X} [MRET] ahh returned from exc",hart->pc);
}
void exec_SFENCE_VMA(struct HART *hart, uint32_t inst, CACHE_DecodedOperands* opers, std::tuple<llvm::IRBuilder<>*, llvm::Function*, llvm::Value*>* jitd) {
    uint64_t rs1v = rs1(inst);
    uint64_t rs2v = rs2(inst);
    uint64_t mstatus = hart->csrs[MSTATUS];

    bool tvm = (mstatus >> 20) & 1; // TVM bit

    // Only legal in S or M mode
    if (hart->mode == 0) { // U-mode
        hart->cpu_trap(EXC_ILLEGAL_INSTRUCTION, inst, false);
        return;
    }
    if (hart->mode == 1 && tvm) { // S-mode and TVM=1
        hart->cpu_trap(EXC_ILLEGAL_INSTRUCTION, inst, false);
        return;
    }

    // If you implement TLB:
    // hart->tlb.invalidate(rs1 == 0 ? ALL_PAGES : specific_page);
    dram_cache(&hart->dram);

    if (hart->dbg)
        hart->print_d("{0x%.8X} [SFENCE.VMA] rs1=%lu rs2=%lu", hart->pc, rs1v, rs2v);
}