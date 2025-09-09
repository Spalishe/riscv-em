#pragma once

#include "mmio.h"

struct ROM : public Device {
    ROM(uint64_t base, uint64_t size, DRAM& ram)
        : Device(base, size, ram) {}

    uint64_t read(HART* hart,uint64_t addr,uint64_t size);

    void write(HART* hart, uint64_t addr, uint64_t size,uint64_t val);
};