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

#include "../include/mmu.hpp"
#include <format>

uint64_t PTE_PPN(uint64_t pte, uint64_t VA, int level) {
    uint64_t PPN_2 = number_read_bits(pte,28,53);
    uint64_t PPN_1 = number_read_bits(pte,19,27);
    uint64_t PPN_0 = number_read_bits(pte,10,18);
    switch(level) {
        case 0:
            return (PPN_2 << 30) | (PPN_1 << 21) | (PPN_0 << 12) | (VA & 0xFFF);
        case 1:
            return (PPN_2 << 30) | (PPN_1 << 21) | (VA & 0x1FFFFF);
        case 2:
            return (PPN_2 << 30) | (VA & 0x3FFFFFFF);
    }
    return 0;
}

std::optional<uint64_t> mmu_translate(MMU& mmu, HART *hart, uint64_t VA, AccessType access_type) {
    uint64_t satp = hart->csrs[SATP];
    PagingMode pmode = (PagingMode)number_read_bits(satp, SATP_MODE_LOW, SATP_MODE_HIGH);
    uint64_t root_table = number_read_bits(satp, SATP_PPN_LOW, SATP_PPN_HIGH) << 12; // page table base
    PrivilegeMode priv_mode = hart->mode;

    if (auto pa = tlb_lookup(mmu.tlb, VA, access_type, priv_mode))
        return *pa;

    uint64_t exc_cause = EXC_INST_PAGE_FAULT;
    if(access_type == AccessType::LOAD) exc_cause = EXC_LOAD_PAGE_FAULT;
    if(access_type == AccessType::STORE) exc_cause = EXC_STORE_PAGE_FAULT;

    if(pmode == PagingMode::Bare || priv_mode == PrivilegeMode::Machine) {
        return VA;
    }

    uint64_t va_sign = (VA >> 38) & 1;
    uint64_t va_upper = VA >> 39;
    uint64_t sign_ext = va_sign ? ((1ULL << 25) - 1) : 0;
    if(va_upper != sign_ext) {
        hart_trap(*hart, exc_cause, 0, false);
        return std::nullopt;
    }

    uint64_t VPN_2 = number_read_bits(VA, SV39_VPN2_LOW, SV39_VPN2_HIGH);
    uint64_t VPN_1 = number_read_bits(VA, SV39_VPN1_LOW, SV39_VPN1_HIGH);
    uint64_t VPN_0 = number_read_bits(VA, SV39_VPN0_LOW, SV39_VPN0_HIGH);

    uint64_t a = root_table;
    for(int level = 2; level >= 0; level--) {
        uint64_t vpn = (level == 2) ? VPN_2 : (level == 1) ? VPN_1 : VPN_0;
        uint64_t pte_addr = a + vpn * 8;
        uint64_t pte = dram_load(mmu.dram, pte_addr, 64);

        bool V = number_read_bits(pte,0,0);
        bool R = number_read_bits(pte,1,1);
        bool W = number_read_bits(pte,2,2);
        bool X = number_read_bits(pte,3,3);
        bool U = number_read_bits(pte,4,4);
        bool G = number_read_bits(pte,5,5);
        bool A = number_read_bits(pte,6,6);
        bool D = number_read_bits(pte,7,7);

        if(!V || (R==0 && W==1)) { // invalid
            hart_trap(*hart, exc_cause, 0, false);
            return std::nullopt;
        }

        if(R || X) { // leaf
            if(priv_mode == PrivilegeMode::User && U==0) {
                hart_trap(*hart, exc_cause, 0, false);
                return std::nullopt;
            } else if(priv_mode == PrivilegeMode::Supervisor && U==1) {
                if( csr_read_mstatus(hart,18,18) != 1 ) { // SUM bit
                    hart_trap(*hart, exc_cause, 0, false);
                    return std::nullopt;
                }
            }

            if(access_type == AccessType::EXECUTE && X==0) {
                hart_trap(*hart, exc_cause, 0, false);
                return std::nullopt;
            } else if(access_type == AccessType::LOAD && R==0) {
                hart_trap(*hart, exc_cause, 0, false);
                return std::nullopt;
            } else if(access_type == AccessType::STORE && W==0) {
                hart_trap(*hart, exc_cause, 0, false);
                return std::nullopt;
            }

            if(!A || (access_type == AccessType::STORE && !D)) { 
                hart_trap(*hart, exc_cause, 0, false);
                return std::nullopt;
            }
            uint64_t PPN_2 = number_read_bits(pte,28,53);
            uint64_t PPN_1 = number_read_bits(pte,19,27);
            uint64_t PPN_0 = number_read_bits(pte,10,18);
            uint64_t pa;
            if(level==0) { // 4KiB page
                pa = (PPN_2<<30)|(PPN_1<<21)|(PPN_0<<12)|(VA & 0xFFF);
            } else if(level==1) { // 2MiB page
                pa = (PPN_2<<30)|(PPN_1<<21)|(VA & 0x1FFFFF);
                if(PPN_0 != 0) { hart_trap(*hart, exc_cause, 0, false); return std::nullopt; }
            } else { // level==2, 1GiB page
                pa = (PPN_2<<30)|(VA & 0x3FFFFFFF);
                if(PPN_1 != 0 || PPN_0 != 0) { hart_trap(*hart, exc_cause, 0, false); return std::nullopt; }
            }
            tlb_insert(
                mmu.tlb,
                VA,
                pa,
                level,
                R, W, X, U
            );
            return pa;
        } else { // non-leaf
            uint64_t next_ppn = number_read_bits(pte, 10, 53);
            a = next_ppn << 12;
            continue;
        }
    }

    hart_trap(*hart, exc_cause, 0, false);
    return std::nullopt;
}

void tlb_flush(TLB& tlb) {
    for(auto& entry : tlb.entries) {
        entry.valid = false;
    }
}
uint64_t make_tlb_vpn(uint64_t va, int level) {
    if (level == 0)
        return va >> 12;        // VPN[2:0]
    if (level == 1)
        return va >> 21;        // VPN[2:1]
    return va >> 30;            // VPN[2]
}
std::optional<uint64_t> tlb_lookup(TLB& tlb, uint64_t va, AccessType type, PrivilegeMode priv) {
    for (auto& e : tlb.entries) {
        if (!e.valid) continue;

        uint64_t vpn = make_tlb_vpn(va, e.level);
        if (vpn != e.vpn) continue;

        if (type == AccessType::EXECUTE && !e.X) return std::nullopt;
        if (type == AccessType::LOAD    && !e.R) return std::nullopt;
        if (type == AccessType::STORE   && !e.W) return std::nullopt;

        if (priv == PrivilegeMode::User && !e.U)
            return std::nullopt;

        uint64_t offset_mask =
            (e.level == 0) ? 0xFFF :
            (e.level == 1) ? 0x1FFFFF :
                             0x3FFFFFFF;

        return (e.ppn << 12) | (va & offset_mask);
    }

    return std::nullopt;
}
void tlb_insert(TLB& tlb, uint64_t va, uint64_t pa, int level,bool R, bool W, bool X, bool U) {
    static int victim = 0;

    TLBEntry& e = tlb.entries[victim];
    victim = (victim + 1) % TLB_SIZE;

    e.vpn   = make_tlb_vpn(va, level);
    e.ppn   = pa >> 12;
    e.level = level;
    e.R = R; e.W = W; e.X = X; e.U = U;
    e.valid = true;
}