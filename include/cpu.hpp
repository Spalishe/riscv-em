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

#include <stdint.h>
#include "dram.h"
#include "devices/mmio.h"
#include "decode.h"
#include "csr.h"
#include <string>
#include <functional>
#include <unordered_map>
#include <tuple>
#include "pmp.hpp"

struct MMIO;

struct HART;

struct inst_data;

enum class PrivilegeMode {
    User = 0,
    Supervisor = 1,
    Hypervisor = 2,
    Machine = 3
};

struct Reservation {
    bool valid = false;
    uint64_t vaddr = 0;
    uint8_t size = 0;
};

struct HART {
    uint64_t GPR[32];
    uint64_t pc = DRAM_BASE;
    uint64_t csrs[4069];
    PrivilegeMode mode = PrivilegeMode::Machine;

    inst_data instr_cache[8192];

	uint8_t id = 0;

    MMIO* mmio;

    bool WFI = false;

    uint32_t fetch_buffer[8];
    uint64_t fetch_pc; 
    uint8_t fetch_buffer_i;
    
    PMP pmp;
    Reservation reservation;
};

void hart_reset(HART&, uint64_t dtb_path);
uint32_t hart_fetch(HART&, uint64_t _pc);
void hart_step(HART&);
void hart_execute(HART&, inst_data inst);
void hart_check_interrupts(HART&);
void hart_trap(HART&, uint64_t cause, uint64_t tval, bool is_interrupt);

static uint64_t riscv_mkmisa(const char* str)
{
    uint64_t ret = 0;
    ret |= 0x8000000000000000ULL;
    while (*str && *str != '_') {
        if (*str >= 'a' && *str <= 'z') {
            ret |= (1 << (*str - 'a'));
        }
        str++;
    }
    return ret;
}

inline uint64_t csr_read_mstatus(HART *hart, uint8_t bit_low, uint8_t bit_high) {
    uint64_t mstatus = hart->csrs[MSTATUS];
    uint8_t width = bit_high - bit_low + 1;

    uint64_t mask;
    if (width == 64) {
        mask = ~0ULL;
    } else {
        mask = (1ULL << width) - 1;
    }

    return (mstatus >> bit_low) & mask;
}

inline void csr_write_mstatus(HART *hart, uint8_t bit_low, uint8_t bit_high, uint64_t new_val) {
    uint64_t mstatus = hart->csrs[MSTATUS];
    uint8_t width = bit_high - bit_low + 1;

    uint64_t mask;
    if (width == 64) {
        mask = ~0ULL;
    } else {
        mask = (1ULL << width) - 1;
    }

    mstatus &= ~(mask << bit_low);
    mstatus |= (new_val & mask) << bit_low;
    hart->csrs[MSTATUS] = mstatus;
}
void csr_write(HART *hart, uint16_t csr, uint64_t val);
uint64_t csr_read(HART *hart, uint16_t csr);