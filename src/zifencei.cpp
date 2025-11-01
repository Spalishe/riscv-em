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
#include <iostream>

void exec_FENCE_I(struct HART *hart, uint32_t inst, CACHE_DecodedOperands* opers) {
    hart->instr_block_cache.clear();
    hart->instr_block_cache_count_executed.clear();
    memset(hart->instr_cache, 0, sizeof(hart->instr_cache));
    if(hart->dbg) hart->print_d("{0x%.8X} [FENCE.I] uhh i cant figure out how fence memory btw so i just clear cache",hart->pc);
}
