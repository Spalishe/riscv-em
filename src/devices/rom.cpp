#include "../include/devices/rom.hpp"

uint64_t ROM::read(HART* hart,uint64_t addr,uint64_t size) {
    return dram_load(&ram,addr,size);
}

void ROM::write(HART* hart, uint64_t addr, uint64_t size,uint64_t val) {
    dram_store(&ram,addr,size,val);
    //cpu_trap(hart,EXC_STORE_ACCESS_FAULT,addr,false);
}