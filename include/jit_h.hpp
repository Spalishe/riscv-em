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
#include "cpu.h"

struct HART;

extern int jit_init();
extern void jit_reset();

using BlockFn = void(*)(HART*);
BlockFn jit_create_block(HART* hart, std::vector<CACHE_Instr>& instrs);

struct OptUInt64 {
    bool has_value;
    uint64_t value;
};
struct DRAMJITSTORE_ARGS {
    uint64_t hart;
    uint64_t addr;
    uint64_t size;
    uint64_t value;
};
struct DRAMJITLOAD_ARGS {
    uint64_t hart;
    uint64_t addr;
    uint64_t size;
};

extern "C" bool dram_jit_store(DRAMJITSTORE_ARGS* args);
extern "C" OptUInt64 dram_jit_load(DRAMJITLOAD_ARGS* args);