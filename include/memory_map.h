#pragma once
#include <cstdint>
#include <vector>
#include <stdexcept>
#include <unordered_map>

struct MemoryRegion {
    uint64_t base_addr;
    size_t size;
    uint8_t* data;

    MemoryRegion(uint64_t base, size_t sz)
        : base_addr(base), size(sz) {
        data = new uint8_t[size]{0};
    }

    ~MemoryRegion() {
        delete[] data;
    }

    uint8_t* ptr(uint64_t addr) {
        if(addr < base_addr || addr >= base_addr + size)
            throw std::out_of_range("Memory access out of region");
        return data + (addr - base_addr);
    }
};

struct MemoryMap {
    std::vector<MemoryRegion*> regions;
    std::unordered_map<uint64_t,MemoryRegion*> cache;

    ~MemoryMap() {
        for(auto* r : regions) delete r;
    }

    void add_region(uint64_t base, size_t size) {
        regions.push_back(new MemoryRegion(base, size));
    }

    uint64_t load(uint64_t addr, uint64_t size) {
        MemoryRegion* r = find_region(addr);
        uint8_t* p = r->ptr(addr);
        switch(size) {
            case 8:  return p[0];
            case 16: return p[0] | ((uint64_t)p[1] << 8);
            case 32: return p[0] | ((uint64_t)p[1] << 8)
                       | ((uint64_t)p[2] << 16) | ((uint64_t)p[3] << 24);
            case 64: {
                uint64_t val = 0;
                for(int i=0;i<8;i++) val |= ((uint64_t)p[i] << (i*8));
                return val;
            }
            default: throw std::invalid_argument("Invalid load size");
        }
    }

    void store(uint64_t addr, uint64_t size, uint64_t value) {
        MemoryRegion* r = find_region(addr);
        uint8_t* p = r->ptr(addr);
        switch(size) {
            case 8:  p[0] = (uint8_t)value; break;
            case 16: p[0] = (uint8_t)value; p[1] = (uint8_t)(value >> 8); break;
            case 32: 
                p[0]=(uint8_t)value; p[1]=(uint8_t)(value>>8);
                p[2]=(uint8_t)(value>>16); p[3]=(uint8_t)(value>>24);
                break;
            case 64:
                for(int i=0;i<8;i++) p[i] = (uint8_t)(value>>(i*8));
                break;
            default: throw std::invalid_argument("Invalid store size");
        }
    }

private:
    MemoryRegion* find_region(uint64_t addr) {
        if(cache.find(addr) != cache.end()) {return cache[addr];}
        for(auto* r : regions) {
            if(addr >= r->base_addr && addr < r->base_addr + r->size) {
                cache[addr] = r;
                return r;
            }
        }
        throw std::out_of_range("Address not mapped in MemoryMap");
    }
};
