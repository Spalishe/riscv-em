#pragma once

#include <stdint.h>
#include "dram.h"
#include "devices/mmio.h"
#include <string>
#include <functional>
#include <unordered_map>
#include <tuple>

struct MMIO;

struct HART {
    DRAM dram;
    MMIO* mmio;
    uint64_t regs[32];
    uint64_t pc;
    uint64_t csrs[4069];
    uint8_t mode;

    bool dbg;
    bool dbg_singlestep;
	uint64_t breakpoint;
	uint8_t id;

    std::unordered_map<uint32_t, std::tuple<void(*)(HART*, uint32_t), bool>> instr_cache;
    
	bool stopexec;

	uint64_t reservation_addr;
	uint64_t reservation_value;
	bool reservation_valid;
	uint8_t reservation_size;

    void cpu_start(bool debug, uint64_t dtb_path);
    uint32_t cpu_fetch();
    void cpu_loop();
    void cpu_execute(uint32_t inst);
    uint64_t cpu_readfile(std::string path, uint64_t addr, bool bigendian);
    void print_d(const std::string& fmt, ...);
    void cpu_trap(uint64_t cause, uint64_t tval, bool is_interrupt);

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