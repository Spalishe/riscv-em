#pragma once

#include "mmio.h"

struct PCI : public Device {
    PCI(uint64_t base, uint64_t size, DRAM& ram,fdt_node* fdt);

    uint64_t read(HART* hart,uint64_t addr, uint64_t size);
    void write(HART* hart, uint64_t addr, uint64_t size, uint64_t val);
};