#include "../include/mmio.h"
#include "../include/cpu.h"
#include "../include/csr.h"
#include "../include/dram.h"
#include <optional>
#include <iostream>

std::optional<uint64_t> MMIO::load(HART* hart, uint64_t addr, uint64_t size) {
    if((addr % (size/8)) != 0) {
        cpu_trap(hart,EXC_LOAD_ADDR_MISALIGNED,addr,false);
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
    cpu_trap(hart,EXC_LOAD_ACCESS_FAULT,addr,false);
    return std::nullopt;
}

void MMIO::store(HART* hart, uint64_t addr, uint64_t size, uint64_t value) {
    if(addr % (size/8) != 0) {
        cpu_trap(hart,EXC_STORE_ADDR_MISALIGNED,addr,false);
        return;
    }

    // DRAM
    if (addr >= DRAM_BASE && addr < DRAM_BASE + DRAM_SIZE) {
        dram_store(&ram,addr,size,value);
        return;
    }

    // Devices
    for (auto* dev : devices) {
        if (addr >= dev->base && addr < dev->base + dev->size) {
            dev->write(hart, addr, size, value);
            return;
        }
    }
    
    // Fault
    cpu_trap(hart,EXC_STORE_ACCESS_FAULT,addr,false);
}
