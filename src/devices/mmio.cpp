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

#include "../../include/devices/mmio.h"
#include "../../include/cpu.h"
#include "../../include/csr.h"
#include "../../include/dram.h"
#include <optional>
#include <iostream>

std::optional<uint64_t> MMIO::load(HART* hart, uint64_t addr, uint64_t size) {
    if((addr % (size/8)) != 0) {
        hart->cpu_trap(EXC_LOAD_ADDR_MISALIGNED,addr,false);
        return std::nullopt;
    }

    // DRAM
    if (addr >= DRAM_BASE && addr < DRAM_BASE + DRAM_SIZE) {
        return dram_load(&ram,addr,size);
    }

    // Devices
    for (auto* dev : devices) {
        if (addr >= dev->base && addr < dev->base + dev->size) {
            return dev->read(hart, addr, size);
        }
    }

    // Fault
    hart->cpu_trap(EXC_LOAD_ACCESS_FAULT,addr,false);
    return std::nullopt;
}

bool MMIO::store(HART* hart, uint64_t addr, uint64_t size, uint64_t value) {
    if(addr % (size/8) != 0) {
        hart->cpu_trap(EXC_STORE_ADDR_MISALIGNED,addr,false);
        return false;
    }

    // DRAM
    if (addr >= DRAM_BASE && addr < DRAM_BASE + DRAM_SIZE) {
        dram_store(&ram,addr,size,value);
        return true;
    }

    // Devices
    for (auto* dev : devices) {
        if (addr >= dev->base && addr < dev->base + dev->size) {
            dev->write(hart, addr, size, value);
            return true;
        }
    }
    
    // Fault
    hart->cpu_trap(EXC_STORE_ACCESS_FAULT,addr,false);
    return false;
}