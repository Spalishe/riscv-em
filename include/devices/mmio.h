#pragma once

#include <stdint.h>
#include <vector>
#include <dram.h>
#include <csr.h>
#include <cpu.h>
#include <optional>
#include <cstdio>
#include <atomic>
#include <thread>
#include <iostream>
#include "libfdt.hpp"

struct HART;

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

