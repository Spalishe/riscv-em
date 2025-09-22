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

#include "mmio.h"

struct CLINT : public Device {
    // Memory-mapped registers
    std::vector<uint32_t> msip;      // one per HART, 32-bit
    std::vector<uint64_t> mtimecmp;  // one per HART, 64-bit
    std::atomic<uint64_t> mtime = 0; // global timer
    std::atomic<bool> stop_timer = false;
    std::thread timer_thread;

    CLINT(uint64_t base, DRAM& ram, uint32_t num_harts, fdt_node* fdt);
    void start_timer(uint64_t freq_hz, HART* hart);
    void stop_timer_thread();
    uint64_t read(HART* hart, uint64_t addr, uint64_t size);
    void write(HART* hart, uint64_t addr, uint64_t size, uint64_t value);

private:
    void update_mip(HART* hart);
};