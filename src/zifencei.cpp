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

#include "../include/instset.h"
#include "../include/cpu.h"
#include "../include/csr.h"
#include "../include/jit_h.hpp"
#include <iostream>

void exec_FENCE_I(struct HART *hart, uint32_t inst, CACHE_DecodedOperands* opers, std::tuple<llvm::IRBuilder<>*, llvm::Function*, llvm::Value*>* jitd) {
    hart->instr_block_cache.clear();
    jit_reset();
    if(hart->dbg) hart->print_d("{0x%.8X} [FENCE.I] uhh i cant figure out how fence memory btw so i just clear cache",hart->pc);
}
void exec_FENCE(struct HART *hart, uint32_t inst, CACHE_DecodedOperands* opers, std::tuple<llvm::IRBuilder<>*, llvm::Function*, llvm::Value*>* jitd) {
    if(hart->dbg) hart->print_d("{0x%.8X} [FENCE] there is only 1 hart so its nop rn",hart->pc);
}

void exec_SFENCE_VMA(struct HART *hart, uint32_t inst, CACHE_DecodedOperands* opers, std::tuple<llvm::IRBuilder<>*, llvm::Function*, llvm::Value*>* jitd) {
    dram_cache(&hart->dram);
    if(hart->dbg) hart->print_d("{0x%.8X} [SFENCE.VMA] cleaned up memory cache",hart->pc);
}