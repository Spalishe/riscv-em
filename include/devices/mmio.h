#pragma once

#include <stdint.h>
#include <vector>
#include <dram.h>
#include <csr.h>
#include <optional>
#include <cstdio>
#include <atomic>
#include <thread>
#include <iostream>
#include "libfdt.hpp"

struct HART;
void cpu_trap(struct HART *hart, uint64_t cause, uint64_t tval, bool is_interrupt);

uint8_t h_cpu_id(struct HART *hart);
uint64_t h_cpu_csr_read(struct HART *hart, uint64_t addr);
void h_cpu_csr_write(struct HART *hart, uint64_t addr, uint64_t value);


struct Device {
    uint64_t base;
    uint64_t size;
    DRAM& ram;

    Device(uint64_t base, uint64_t size, DRAM& ram)
        : base(base), size(size), ram(ram) {}

    virtual uint64_t read(HART* hart,uint64_t addr,uint64_t size) = 0;
    virtual void write(HART* hart,uint64_t addr, uint64_t size,uint64_t value) = 0;
};

struct MMIO {
    std::vector<Device*> devices;
    DRAM& ram;

    MMIO(DRAM& ram)
        : ram(ram) {}

    void add(Device* dev) {
        devices.push_back(dev);
    }

    std::optional<uint64_t> load(HART* hart, uint64_t addr, uint64_t size);
    void store(HART* hart, uint64_t addr, uint64_t size, uint64_t value);
};

struct DTB : public Device {
    DTB(uint64_t base, uint64_t size, DRAM& ram)
        : Device(base, size, ram) {}

    uint64_t read(HART* hart,uint64_t addr, uint64_t size) override {
        return dram_load(&ram,addr,size);
    }

    void write(HART* hart, uint64_t addr, uint64_t size, uint64_t val) override {
        //dram_store(&ram,addr,32,val);
        cpu_trap(hart,EXC_STORE_ACCESS_FAULT,addr,false);
    }
}; // Dont use this until point that will force you to place dtb not in 0x80000000
