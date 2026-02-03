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
#define SV39_PAGE_SIZE 4096
#define SV39_LEVELS 3
#define SV39_PTESIZE 8

struct SV39_PTE {
    uint8_t V;
    uint8_t R;
    uint8_t W;
    uint8_t X;
    uint8_t U;
    uint8_t G;
    uint8_t A;
    uint8_t D;
    
    uint8_t RSW;
    uint64_t PPN;
    uint16_t PPN_0;
    uint16_t PPN_1;
    uint32_t PPN_2;
    uint8_t Reserved;
    uint8_t PBMT;
    uint8_t N;

    SV39_PTE(uint64_t val) {
        V = val & 0x1;
        R = (val >> 1) & 0x1;
        W = (val >> 2) & 0x1;
        X = (val >> 3) & 0x1;
        U = (val >> 4) & 0x1;
        G = (val >> 5) & 0x1;
        A = (val >> 6) & 0x1;
        D = (val >> 7) & 0x1;
        RSW = (val >> 8) & 0x3;
        PPN = (val >> 10) & 0x7FFFFFFFFFF;
        PPN_0 = (val >> 10) & 0x1FF;
        PPN_1 = (val >> 19) & 0x1FF;
        PPN_2 = (val >> 28) & 0x3FFFFFF;
        Reserved = (val >> 54) & 0x7F;
        PBMT = (val >> 61) & 0x3;
        N = (val >> 63) & 0x1;
    }

    operator uint64_t() const {
        uint64_t val = 0;
        val |= V;
        val |= R << 1;
        val |= W << 2;
        val |= X << 3;
        val |= U << 4;
        val |= G << 5;
        val |= A << 6;
        val |= D << 7;
        val |= RSW << 8;
        val |= (uint64_t)PPN_0 << 10;
        val |= (uint64_t)PPN_1 << 19;
        val |= (uint64_t)PPN_2 << 28;
        val |= (uint64_t)Reserved << 54;
        val |= (uint64_t)PBMT << 61;
        val |= (uint64_t)N << 63;
        return val;
    }
};

enum class PagingMode : uint8_t {
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