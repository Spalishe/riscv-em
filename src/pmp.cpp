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

#include "../include/pmp.hpp"
#include "../include/cpu.hpp"

std::vector<uint8_t> pmp_get_num_cfgs(uint64_t val) {
    std::vector<uint8_t> cfgs;
    for(int i = 0; i < 8; i++) {
        uint64_t cfg = (val >> (i*8)) & 0xFF;
        if(cfg != 0) {
            cfgs.push_back(i);
        }
    }
    return cfgs;
}

bool pmp_check_locked_cfg(HART* hart, uint8_t ind) {
    return hart->pmp.entries[ind].L;
}
bool pmp_check_locked_addr(HART* hart, uint8_t ind) {
    // Check current
    if(!hart->pmp.entries[ind].L) {
        // Check for next one in case of TOR
        PMPEntry e = hart->pmp.entries[ind+1];
        if(ind >= 64) return false;
        return e.L && e.A == AddressingMode::TOR;
    } else return true;
}
void pmp_update(HART* hart, uint8_t ind) {
    PMP& pmp = hart->pmp;
    
    if(pmp.entries[ind].L) return;

    uint8_t csr = PMPCFG0 + (ind / 8);
    uint64_t csr_v = hart->csrs[csr];
    uint8_t cfg = (csr_v >> ((ind % 8) * 8)) & 0xFF;
    
    pmp.entries[ind].A = (AddressingMode)((cfg >> 3) & 0x1);
    pmp.entries[ind].L = ((cfg >> 7) & 0x1);
    pmp.entries[ind].R = cfg & 0x1;
    pmp.entries[ind].W = (cfg >> 1) & 0x1;
    pmp.entries[ind].X = (cfg >> 2) & 0x1;
}

bool pmp_region_match(HART* hart, uint64_t PA, uint8_t ind) {
    PMP& pmp = hart->pmp;
    PMPEntry cfg = pmp.entries[ind];
    AddressingMode A = cfg.A;

    if (A == AddressingMode::Off)
        return false;

    if (A == AddressingMode::TOR) {
        uint64_t low  = (ind == 0) ? 0 : (hart->csrs[PMPADDR0 + (ind - 1)] << 2);
        uint64_t high = hart->csrs[PMPADDR0 + ind] << 2;
        return PA >= low && PA < high;
    }

    if (A == AddressingMode::NA4) {
        uint64_t base = hart->csrs[PMPADDR0 + ind] << 2;
        return PA >= base && PA < base + 4;
    }

    uint64_t addr = hart->csrs[PMPADDR0 + ind];
    uint64_t mask = (~addr) & (addr + 1);

    uint64_t base = (addr & ~mask) << 2;
    uint64_t size = (mask + 1) << 2;

    return PA >= base && PA < base + size;
}

bool pmp_validate(HART* hart, uint64_t PA, AccessType access_type) {
    PMP& pmp = hart->pmp;
    for(int i = 0; i < 64; i++) {
        if(!pmp_region_match(hart,PA,i)) continue;

        PMPEntry e = pmp.entries[i];
        if (hart->mode == PrivilegeMode::Machine && !e.L)
            return true;
            
        if (access_type == AccessType::EXECUTE && !e.X) return false;
        if (access_type == AccessType::LOAD && !e.R) return false;
        if (access_type == AccessType::STORE && !e.W) return false;

        return true;
    }
    return hart->mode == PrivilegeMode::Machine;
}