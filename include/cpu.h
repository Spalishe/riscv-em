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

#pragma once

#include <stdint.h>
#include "dram.h"
#include "devices/mmio.h"
#include <string>
#include <functional>
#include <unordered_map>
#include <tuple>

#define PC_EXECUTE_COUNT_TO_BLOCK 10

struct MMIO;

struct HART;

struct CACHE_DecodedOperands {
    CACHE_DecodedOperands() {}
    bool s = false;
    uint8_t rd, rs1, rs2;
    uint32_t imm;

    bool brb = false;
};
struct CACHE_Instr {
    void (*fn)(HART*, uint32_t, CACHE_DecodedOperands*);
    bool incr;
    bool j;
    uint32_t inst;
    bool isBr;
    uint32_t imm_optional;
    CACHE_DecodedOperands oprs;
    bool valid = false;
    uint64_t pc;
};

struct HART {
    uint64_t regs[32];
    uint64_t pc;
    uint64_t virt_pc;
    uint64_t csrs[4069];
    uint8_t mode;
    
	uint64_t reservation_addr;
	uint64_t reservation_value;
	bool reservation_valid;
	uint8_t reservation_size;

    bool testing;
    bool gdbstub;
    bool dbg;
    bool dbg_showinst = true;
    bool dbg_singlestep;
	uint64_t breakpoint;
	uint8_t id;

    uint64_t csrs_old_mtvec;
    uint64_t csrs_old_stvec;
    
    bool block_enabled = true;

    std::vector<CACHE_Instr> instr_block; // instruction function, instruction itself
    std::unordered_map<uint64_t, std::vector<CACHE_Instr>> instr_block_cache; // as key put PC
    std::unordered_map<uint64_t, uint64_t> instr_block_cache_count_executed; // as key put PC
    CACHE_Instr instr_cache[8192]; // L1 ??

	bool stopexec;

    bool trap_active;
    bool trap_notify;

    bool god_said_to_destroy_this_thread = false; // Why should you name it like this?
    uint32_t fetch_buffer[8];
    uint64_t fetch_pc; 

    DRAM dram;
    MMIO* mmio;

    void cpu_start(bool debug, uint64_t dtb_path, bool gdbstub);
    int cpu_start_testing();
    uint32_t cpu_fetch(uint64_t _pc);
    void cpu_check_interrupts();
    void cpu_loop();
    void cpu_execute();
    void cpu_execute_inst(uint32_t inst);
    uint64_t cpu_readfile(std::string path, uint64_t addr, bool bigendian);
    void print_d(const std::string& fmt, ...);
    void cpu_trap(uint64_t cause, uint64_t tval, bool is_interrupt);

    uint64_t csr_read(uint64_t addr);
    void csr_write(uint64_t addr,uint64_t val);

    uint64_t h_cpu_csr_read(uint64_t addr);
    void h_cpu_csr_write(uint64_t addr, uint64_t value);
    uint8_t h_cpu_id();
};

std::string uint8_to_hex(uint8_t value);

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