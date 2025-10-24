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

#include "memory_map.h"
#include "devices/mmio.h"
#include "cpu.h"
#include <vector>
#include <thread>
#include <atomic>
#include <unordered_map>

void reset(bool isNotMain = false);
void poweroff(bool ctrlc, bool isNotMain = false);
void fastexit();
extern MemoryMap memmap;
extern MMIO* mmio;

extern std::vector<HART*> hart_list;
extern std::unordered_map<HART*,std::thread> hart_list_threads;