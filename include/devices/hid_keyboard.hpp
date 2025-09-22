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

#include "i2c_hid.hpp"
#include "i2c_oc.hpp"
#include <cstdint>

#define MAX_PRESSED_KEYS 6

struct hid_keyboard {
    DRAM* ram;
    hid_dev_t  hid_dev;

    uint8_t input_report[10];
    uint8_t output_report[3];

    // State
    uint32_t keys_pressed_now[8];
    uint32_t keys_pressed[8];
    uint32_t leds;
};

inline static uint64_t swap32(DRAM* dram, uint64_t addr, uint64_t val) {
    auto temp = dram_load(dram,addr,32);
    dram_store(dram,addr,32,val);
    return temp;   
}
hid_keyboard* hid_keyboard_init(uint16_t addr, DRAM& ram, PLIC* plic, uint8_t irq_num, fdt_node* fdt, i2c_bus_t* bus);
void hid_keyboard_press(hid_keyboard* kb, uint8_t key);
void hid_keyboard_release(hid_keyboard* kb, uint8_t key);