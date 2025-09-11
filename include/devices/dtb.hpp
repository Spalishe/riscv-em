#include "mmio.h"

struct DTB : public Device {
    DTB(uint64_t base, uint64_t size, DRAM& ram)
        : Device(base, size, ram) {}

    uint64_t read(HART* hart,uint64_t addr, uint64_t size);
    void write(HART* hart, uint64_t addr, uint64_t size, uint64_t val);
}; // Dont use this until point that will force you to place dtb not in 0x80000000