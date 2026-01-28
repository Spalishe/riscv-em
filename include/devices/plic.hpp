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

#include "mmio.h"
#include <vector>

static constexpr uint64_t PLIC_OFF_PRIORITY    = 0x0000;     
static constexpr uint64_t PLIC_OFF_PENDING     = 0x1000;     
static constexpr uint64_t PLIC_OFF_ENABLE_BASE = 0x2000;     
static constexpr uint64_t PLIC_OFF_CTX_BASE    = 0x200000;   
static constexpr uint64_t PLIC_CTX_THRESHOLD   = 0x0;
static constexpr uint64_t PLIC_CTX_CLAIMCOMP   = 0x4;

static constexpr uint32_t PLIC_NUM_CONTEXTS = 2;   // 0: M-mode, 1: S-mode

struct PLIC : public Device {
    uint64_t base_addr;
    uint64_t size_bytes;
    uint64_t num_harts;
    uint64_t num_contexts;
    uint32_t num_sources;

    std::vector<uint32_t> priority;                 
    std::vector<uint8_t>  pending;                  
    std::vector<uint8_t>  active;                   
    std::vector<std::vector<uint32_t>> enable;      
    std::vector<uint32_t> threshold;                
    std::vector<uint32_t> last_claimed;             

    PLIC(uint64_t base, uint64_t size, Machine& cpu, uint32_t num_sources, fdt_node* fdt);

    void raise(uint32_t src);
    void clear(uint32_t src);
    uint64_t read(HART* hart, uint64_t addr,uint64_t size);
    void write(HART* hart, uint64_t addr, uint64_t size, uint64_t value);
    void plic_service(HART* hart);

private:
    uint32_t pick_claimable(uint32_t ctx);
    bool is_enabled(uint32_t ctx, uint32_t src);
};