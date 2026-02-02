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

#pragma once
#include <cstdint>
#include <optional>
#include "csr.h"
#include "cpu.hpp"

#define SATP_MODE_HIGH 63
#define SATP_MODE_LOW 60
#define SATP_ASID_HIGH 59
#define SATP_ASID_LOW 44
#define SATP_PPN_HIGH 43
#define SATP_PPN_LOW 0

#define SV39_VPN2_HIGH 38
#define SV39_VPN2_LOW 30
#define SV39_VPN1_HIGH 29
#define SV39_VPN1_LOW 21
#define SV39_VPN0_HIGH 20
#define SV39_VPN0_LOW 12

enum class PagingMode {
    Bare = 0,
    Sv39 = 8,
    Sv48 = 9,
    Sv57 = 10,
    Sv64 = 11,
};

enum class AccessType {
    LOAD = 0,
    STORE = 1,
    EXECUTE = 2
};

struct DRAM;
struct HART;
struct Machine;
enum class PrivilegeMode;

static constexpr int TLB_SIZE = 32;
struct TLBEntry {
    uint64_t vpn;
    uint64_t ppn;
    uint16_t asid;       // из satp
    uint8_t  level;   // 0=4K, 1=2M, 2=1G
    bool R, W, X, U;
    bool valid;
};
struct TLB {
    TLBEntry entries[TLB_SIZE];
};

struct MMU {
    ~MMU();
    Machine* cpu;
    DRAM* dram;
    TLB tlb;
};

void tlb_flush(TLB& tlb);
uint64_t make_tlb_vpn(uint64_t va, int level);
std::optional<uint64_t> tlb_lookup(TLB&, uint64_t va, uint16_t asid, AccessType type, PrivilegeMode priv, uint64_t sum_bit);
void tlb_insert(TLB&, uint64_t va, uint64_t pa, uint16_t asid, int level,bool R, bool W, bool X, bool U);

std::optional<uint64_t> mmu_translate(MMU&, HART*, uint64_t VA, AccessType access_type);

inline uint64_t number_read_bits(uint64_t val, uint8_t bit_low, uint8_t bit_high) {
    uint8_t width = bit_high - bit_low + 1;

    uint64_t mask;
    if (width == 64) {
        mask = ~0ULL;
    } else {
        mask = (1ULL << width) - 1;
    }

    return (val >> bit_low) & mask;
}

inline uint64_t number_write_bits(uint64_t val, uint8_t bit_low, uint8_t bit_high, uint64_t new_val) {
    uint8_t width = bit_high - bit_low + 1;

    uint64_t mask;
    if (width == 64) {
        mask = ~0ULL;
    } else {
        mask = (1ULL << width) - 1;
    }

    val &= ~(mask << bit_low);
    val |= (new_val & mask) << bit_low;
    return val;
}