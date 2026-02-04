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

#include "../../include/devices/mmio.h"
#include "../../include/devices/uart.hpp"
#include "../../include/cpu.hpp"
#include "../../include/csr.h"
#include "../../include/dram.h"
#include "../../include/mmu.hpp"
#include <optional>
#include <iostream>

std::optional<uint64_t> MMIO::load(HART* hart, uint64_t addr, uint64_t size) {
    uint64_t o = addr;
    auto phys = mmu_translate(*mmu, hart, addr, AccessType::LOAD);
    if (!phys.has_value()) return std::nullopt;
    uint64_t pa = phys.value();

    if((pa % (size/8)) != 0) {
        hart_trap(*hart,EXC_LOAD_ADDR_MISALIGNED,addr,false);
        return std::nullopt;
    }

    // DRAM
    if (pa >= DRAM_BASE && pa < DRAM_BASE + DRAM_SIZE) {
        return dram_load(ram,pa,size);
    }

    // Devices
    for (auto* dev : devices) {
        if (pa >= dev->base && pa < dev->base + dev->size) {
            return dev->read(hart, pa, size);
        }
    }

    // Fault
    hart_trap(*hart,EXC_LOAD_ACCESS_FAULT,addr,false);
    return std::nullopt;
}

bool MMIO::store(HART* hart, uint64_t addr, uint64_t size, uint64_t value) {
    auto phys = mmu_translate(*mmu, hart, addr, AccessType::STORE);
    if (!phys.has_value()) return false;
    uint64_t pa = phys.value();

    if(pa % (size/8) != 0) {
        hart_trap(*hart,EXC_STORE_ADDR_MISALIGNED,addr,false);
        return false;
    }

    // DRAM
    if (pa >= DRAM_BASE && pa < DRAM_BASE + DRAM_SIZE) {
        dram_store(ram,pa,size,value);
        return true;
    }

    // Devices
    for (auto* dev : devices) {
        if (pa >= dev->base && pa < dev->base + dev->size) {
            dev->write(hart, pa, size, value);
            return true;
        }
    }
    
    // Fault
    hart_trap(*hart,EXC_STORE_ACCESS_FAULT,addr,false);
    return false;
}

std::optional<uint64_t> MMIO::load_GDB(HART* hart, uint64_t addr, uint64_t size) {
    // DRAM
    if (addr >= DRAM_BASE && addr < DRAM_BASE + DRAM_SIZE) {
        return dram_load(ram,addr,size);
    }

    // Devices
    for (auto* dev : devices) {
        if (addr >= dev->base && addr < dev->base + dev->size) {
            return dev->read(hart, addr, size);
        }
    }
    return std::nullopt;
}

void MMIO::store_GDB(HART* hart, uint64_t addr, uint64_t size, uint64_t value) {
    // DRAM
    if (addr >= DRAM_BASE && addr < DRAM_BASE + DRAM_SIZE) {
        dram_store(ram,addr,size,value);
        return;
    }

    // Devices
    for (auto* dev : devices) {
        if (addr >= dev->base && addr < dev->base + dev->size) {
            dev->write(hart, addr, size, value);
            return;
        }
    }
    
    return;
}