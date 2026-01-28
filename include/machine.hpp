/*
Copyright 2026 Spalishe

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
#include <vector>
#include "cpu.h"
#include "libfdt.hpp"
#include "gdbstub.hpp"
#include "devices/aclint.hpp"
#include "devices/plic.hpp"
#include "devices/syscon.hpp"
#include "devices/uart.hpp"
#include "devices/virtio_blk.hpp"
#include "devices/rom.hpp"

using namespace std;

struct HART;

enum class MachineState {
    Running = 0,
    Halted = 1,
    PoweringOff = 2,
    PoweredOff = 3,
};

struct Machine {
    Machine();
    ~Machine() {
        for(Device* dev : mmio->devices) {
            delete dev;
        }
        delete mmio;
    };
    uint8_t core_count = 1;
    uint64_t memsize = DRAM_SIZE;

    DRAM dram;
    MMIO* mmio;
    ACLINT* clint;
    PLIC* plic;
    VirtIO_BLK* image;
    UART* uart;
    SYSCON* syscon;

    fdt_node* fdt;
    MemoryMap memmap;

    std::vector<HART*> harts;
    std::unordered_map<uint8_t,thread> harts_threads; // Hart ID -> Thread

    MachineState state = MachineState::PoweredOff;
    bool gdb = false;
    bool dtb_is_file = false;
};

void machine_run(Machine&);
void machine_create_memory(Machine&, uint64_t size);
void machine_create_fdt(Machine&, string cmdline_append);
void machine_create_devices(Machine&);
void machine_create_harts(Machine&);
void machine_poweroff(Machine&);
void machine_reset(Machine&);
void machine_destroy_harts(Machine&);