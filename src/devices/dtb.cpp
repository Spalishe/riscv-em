#include "../include/devices/dtb.hpp"

uint64_t DTB::read(HART* hart,uint64_t addr, uint64_t size) {
    return dram_load(&ram,addr,size);
}

void DTB::write(HART* hart, uint64_t addr, uint64_t size, uint64_t val) {
    //dram_store(&ram,addr,32,val);
    hart->cpu_trap(EXC_STORE_ACCESS_FAULT,addr,false);
}