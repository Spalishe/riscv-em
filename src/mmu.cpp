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

uint16_t SV39_VA_VPN(uint64_t va, int i) {
  switch(i) {
        case 0: return (va >> 12) & 0x1FF;
        case 1: return (va >> 21) & 0x1FF;
        case 2: return (va >> 30) & 0x1FF;
        default: return 0;
    }  
}
uint64_t SV39_SET_PA_PPN(uint64_t pa, int i, uint32_t val) {
    switch(i) {
        case 0:
            pa &= ~(((1ULL << 9)-1) << 12);
            pa |= (val & 0x1FF) << 12;
            break;
        case 1:
            pa &= ~(((1ULL << 9)-1) << 21);
            pa |= (val & 0x1FF) << 21;
            break;
        case 2:
            pa &= ~(((1ULL << 26)-1) << 30);
            pa |= (val & 0x3FFFFFF) << 30;
            break;
    }
    return pa;
}
uint64_t SV39_PTE_PPN(SV39_PTE pte, int i) {
  switch(i) {
        case 0: return pte.PPN_0;
        case 1: return pte.PPN_1;
        case 2: return pte.PPN_2;
        default: return 0;
    }  
}

// Look at https://riscv.github.io/riscv-isa-manual/snapshot/privileged/; 12.3.1. Addressing and Memory Protection
std::optional<uint64_t> mmu_translate(MMU& mmu, HART *hart, uint64_t VA, AccessType access_type) {
    try {
        uint64_t satp = hart->csrs[SATP];
        PagingMode MODE = (PagingMode)number_read_bits(satp,SATP_MODE_LOW,SATP_MODE_HIGH);
        uint16_t ASID = number_read_bits(satp,SATP_ASID_LOW,SATP_ASID_HIGH);
        uint64_t PPN = number_read_bits(satp,SATP_PPN_LOW,SATP_PPN_HIGH);

        PrivilegeMode mode = ((csr_read_mstatus(hart,MSTATUS_MPRV,MSTATUS_MPRV) && access_type != AccessType::EXECUTE) ? (PrivilegeMode)csr_read_mstatus(hart,MSTATUS_MPP_LOW,MSTATUS_MPP_HIGH) : hart->mode);
        //mode = (hart->mode != PrivilegeMode::Machine) ? hart->mode : mode;

        if(MODE == PagingMode::Bare || mode == PrivilegeMode::Machine) {
            // When MODE=Bare, supervisor virtual addresses are equal to supervisor physical addresses,
            // and there is no additional memory protection beyond the physical memory protection scheme described in Section 3.7.
            return VA;
        }

        if (auto pa = tlb_lookup(mmu.tlb, VA, ASID, access_type, mode, csr_read_mstatus(hart,18,18)))
            return *pa;

        // Sv39 implementations support a 39-bit virtual address space, divided into pages.
        // An Sv39 address is partitioned as shown in Figure 68. Instruction fetch addresses and load and store effective addresses,
        // which are 64 bits, must have bits 63–39 all equal to bit 38, or else a page-fault exception will occur.
        uint64_t sign = (VA >> 38) & 1;
        uint64_t upper = VA >> 39;
        if (sign) {
            if(upper != ((1ULL << 25) - 1)) throw 1;
        } else {
            if(upper != 0) throw 1;
        }
        
        // Let a be satp.ppn×PAGESIZE, and let i=LEVELS-1.
        // The satp register must be active, i.e., the effective privilege mode must be S-mode or U-mode.
        uint64_t a = PPN * SV39_PAGE_SIZE;
        int i = SV39_LEVELS - 1;
        while(i >= 0) {
            // Let pte be the value of the PTE at address a+va.vpn[i]×PTESIZE.
            // If accessing pte violates a PMA or PMP check, raise an access-fault exception corresponding to the original access type.
            uint32_t vpn = SV39_VA_VPN(VA,i);
            uint64_t pte_addr = a + vpn*SV39_PTESIZE;
            if(!pmp_validate(hart,pte_addr,access_type)) throw 2;
            SV39_PTE pte = (SV39_PTE)dram_load(mmu.dram,pte_addr,SV39_PTESIZE*8); // Convert bytes count to bits count

            // If pte.v=0, or if pte.r=0 and pte.w=1, or if any bits or encodings that are reserved 
            // for future standard use are set within pte, stop and raise a page-fault exception corresponding to the original access type.
            if(pte.V == 0 || (pte.R == 0 && pte.W == 1)) throw 1;

            
            if(pte.R == 1 | pte.X == 1) {
                // A leaf PTE has been reached. If i>0 and pte.ppn[i-1:0] ≠ 0, this is a misaligned superpage;
                // stop and raise a page-fault exception corresponding to the original access type.
                for(int j = 0; j < i; j++)
                    if(SV39_PTE_PPN(pte,j) != 0) throw 1;

                // Determine if the requested memory access is allowed by the pte.u bit,
                // given the current privilege mode and the value of the SUM and MXR fields of the mstatus register.
                // If not, stop and raise a page-fault exception corresponding to the original access type.
                uint8_t MXR = csr_read_mstatus(hart,MSTATUS_MXR,MSTATUS_MXR);
                uint8_t SUM = csr_read_mstatus(hart,MSTATUS_SUM,MSTATUS_SUM);

                if(mode == PrivilegeMode::User && !pte.U) throw 1;
                if(mode == PrivilegeMode::Supervisor && pte.U && !SUM) throw 1;
                if(access_type == AccessType::LOAD) {
                    bool readable = MXR ? (pte.R || pte.X) : pte.R;
                    if(!readable) throw 1;
                }

                // Determine if the requested memory access is allowed by the pte.r, pte.w, and pte.x bits.
                // If not, stop and raise a page-fault exception corresponding to the original access type.
                if(access_type == AccessType::EXECUTE && !pte.X) throw 1;
                if(access_type == AccessType::LOAD && !pte.R) throw 1;
                if(access_type == AccessType::STORE && !pte.W) throw 1;

                // If pte.a=0, or if the original memory access is a store and pte.d=0:
                if(pte.A == 0 || (access_type == AccessType::STORE && pte.D == 0)) {
                    uint64_t new_addr = a+SV39_VA_VPN(VA,i)*SV39_PTESIZE;
                    // If a store to the PTE at address a+va.vpn[i]×PTESIZE would violate a PMA or PMP check,
                    // raise an access-fault exception corresponding to the original access type.
                    if(!pmp_validate(hart,pte_addr,access_type)) throw 2;
                    uint64_t pte_new = dram_load(mmu.dram,new_addr,SV39_PTESIZE*8);
                    // Compare pte to the value of the PTE at address a+va.vpn[i]×PTESIZE.
                    if(pte_new == (uint64_t)pte) {
                        // If the values match, set pte.a to 1 and, if the original memory access is a store,
                        // also set pte.d to 1. Then store pte to the PTE at address a+va.vpn[i]×PTESIZE.
                        pte.A = 1;
                        if(access_type == AccessType::STORE) pte.D = 1;
                        dram_store(mmu.dram,new_addr,SV39_PTESIZE*8,(uint64_t)pte);
                    } else {
                        // If the comparison fails, return to step 2.
                        continue;
                    }
                }

                // The translation is successful. The translated physical address is given as follows:
                // pa.pgoff = va.pgoff.
                // If i>0, then this is a superpage translation and pa.ppn[i-1:0] = va.vpn[i-1:0].
                // pa.ppn[LEVELS-1:i] = pte.ppn[LEVELS-1:i].
                uint64_t pa = 0;
                pa |= VA & 0xFFF; // page offset
                for (int j = 0; j < i; j++) {
                    if (SV39_PTE_PPN(pte, j) != 0) {
                        throw 1;
                    }
                }

                for (int j = 0; j < SV39_LEVELS; j++) {
                    uint32_t val;
                    if (j < i) {
                        // superpage
                        val = SV39_VA_VPN(VA, j);
                    } else {
                        val = SV39_PTE_PPN(pte, j);
                    }

                    pa = SV39_SET_PA_PPN(pa, j, val);
                }

                tlb_insert(
                    mmu.tlb,
                    VA,
                    pa,
                    ASID,
                    i,
                    pte.R,pte.W,pte.X,pte.U
                );

                return pa;

            } else {
                // the PTE is valid. If pte.r=1 or pte.x=1, go to step 5. Otherwise, this PTE is a pointer to the next level of the page table.
                // Let i=i-1. If i<0, stop and raise a page-fault exception corresponding to the original access type. 
                // Otherwise, let a=pte.ppn×PAGESIZE and go to step 2.
                i = i - 1;
                a = pte.PPN * SV39_PAGE_SIZE;
            }
        }
        throw 1;
    } catch(int errorcode) {
        if(errorcode == 1) {
            // page fault
            uint64_t cause;
            if(access_type == AccessType::EXECUTE) cause = EXC_INST_PAGE_FAULT;
            if(access_type == AccessType::LOAD) cause = EXC_LOAD_PAGE_FAULT;
            if(access_type == AccessType::STORE) cause = EXC_STORE_PAGE_FAULT;
            hart_trap(*hart,cause,VA,false);
            return std::nullopt;
        } else if(errorcode == 2) {
            uint64_t ac_cause;
            if(access_type == AccessType::EXECUTE) ac_cause = EXC_INST_ACCESS_FAULT;
            if(access_type == AccessType::LOAD) ac_cause = EXC_LOAD_ACCESS_FAULT;
            if(access_type == AccessType::STORE) ac_cause = EXC_STORE_ACCESS_FAULT;
            hart_trap(*hart,ac_cause,VA,false);
            return std::nullopt;
        }
    }
    return std::nullopt;
}

void tlb_flush(TLB& tlb) {
    for(auto& entry : tlb.entries) {
        entry.valid = false;
        entry.asid = 0;
        entry.vpn = 0;
        entry.ppn = 0;
    }
}
uint64_t make_tlb_vpn(uint64_t va, int level) {
    switch(level) {
        case 0: return va >> 12;                    // VPN[2:0]
        case 1: return (va >> 12) & 0x1FF;          // VPN[2:1] mask lower VPN0
        case 2: return (va >> 30) & 0x1FF;          // VPN[2] mask VPN1/0
    }
    return 0;
}
std::optional<uint64_t> tlb_lookup(TLB& tlb, uint64_t va, uint16_t asid, AccessType type, PrivilegeMode priv, uint64_t sum_bit = 0) {
    for (auto& e : tlb.entries) {
        if (!e.valid) continue;

        uint64_t vpn = make_tlb_vpn(va, e.level);
        if (priv != PrivilegeMode::Machine && e.asid != 0 && (e.asid != asid)) continue;
        if (vpn != e.vpn) continue;

        if (type == AccessType::EXECUTE && !e.X) return std::nullopt;
        if (type == AccessType::LOAD    && !e.R) return std::nullopt;
        if (type == AccessType::STORE   && !e.W) return std::nullopt;

        if (priv == PrivilegeMode::User && !e.U)
            return std::nullopt;
        if (priv == PrivilegeMode::Supervisor && e.U) {
            if(!sum_bit) return std::nullopt;
        }

        uint64_t offset_mask =
            (e.level == 0) ? 0xFFF :
            (e.level == 1) ? 0x1FFFFF :
                             0x3FFFFFFF;

        return e.ppn << 12 | (va & ((1 << (12 + 9*e.level)) - 1));
    }

    return std::nullopt;
}
void tlb_insert(TLB& tlb, uint64_t va, uint64_t pa, uint16_t asid, int level,bool R, bool W, bool X, bool U) {
    static int victim = 0;

    TLBEntry& e = tlb.entries[victim];
    victim = (victim + 1) % TLB_SIZE;

    e.vpn   = make_tlb_vpn(va, level);
    e.ppn   = pa >> 12;
    e.level = level;
    e.asid = asid;
    e.R = R; e.W = W; e.X = X; e.U = U;
    e.valid = true;
}