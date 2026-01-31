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

#include <cstdint>
#include <vector>

struct HART;

enum class AddressingMode {
    Off = 0,
    TOR = 1,
    NA4 = 2,
    NAPOT = 3,
};

struct PMPEntry {
    AddressingMode A = AddressingMode::Off;
    bool L = false;
    bool X = false;
    bool R = false;
    bool W = false;
};

struct PMP {
    PMPEntry entries[64]{};
};

enum class AccessType;

void pmp_update(HART*, uint8_t ind);
bool pmp_check_locked_cfg(HART*, uint8_t ind);
bool pmp_check_locked_addr(HART*, uint8_t ind);
std::vector<uint8_t> pmp_get_num_cfgs(uint64_t val);
bool pmp_validate(HART*, uint64_t PA, AccessType access_type);
bool pmp_region_match(HART*, uint64_t PA, uint8_t ind);