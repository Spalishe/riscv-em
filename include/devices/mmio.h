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
    bool store(HART* hart, uint64_t addr, uint64_t size, uint64_t value);
};

