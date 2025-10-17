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

#include "../../include/devices/hid_api.hpp"
#include "../../include/devices/i2c_hid.hpp"
#include "../../include/devices/hid_keyboard.hpp"

static inline bool bit_check(uint64_t val, uint64_t pos)
{
    return (val >> pos) & 0x1;
}
static inline uint64_t bit_mask(uint64_t count)
{
    return (1ULL << count) - 1;
}
static inline uint64_t bit_cut(uint64_t val, uint64_t pos, uint64_t bits)
{
    return (val >> pos) & bit_mask(bits);
}

uint8_t keyboard_hid_report_descriptor[] = {
    0x05, 0x01, /* Usage Page (Generic Desktop) */
    0x09, 0x06, /* Usage (Keyboard) */
    0xa1, 0x01, /* Collection (Application) */
    0x75, 0x01, /*   Report Size (1) */
    0x95, 0x08, /*   Report Count (8) */
    0x05, 0x07, /*   Usage Page (Key Codes) */
    0x19, 0xe0, /*   Usage Minimum (224) */
    0x29, 0xe7, /*   Usage Maximum (231) */
    0x15, 0x00, /*   Logical Minimum (0) */
    0x25, 0x01, /*   Logical Maximum (1) */
    0x81, 0x02, /*   Input (Data, Variable, Absolute) */
    0x95, 0x01, /*   Report Count (1) */
    0x75, 0x08, /*   Report Size (8) */
    0x81, 0x01, /*   Input (Constant) */
    0x95, 0x05, /*   Report Count (5) */
    0x75, 0x01, /*   Report Size (1) */
    0x05, 0x08, /*   Usage Page (LEDs) */
    0x19, 0x01, /*   Usage Minimum (1) */
    0x29, 0x05, /*   Usage Maximum (5) */
    0x91, 0x02, /*   Output (Data, Variable, Absolute) */
    0x95, 0x01, /*   Report Count (1) */
    0x75, 0x03, /*   Report Size (3) */
    0x91, 0x01, /*   Output (Constant) */
    0x95, 0x06, /*   Report Count (6) */
    0x75, 0x08, /*   Report Size (8) */
    0x15, 0x00, /*   Logical Minimum (0) */
    0x25, 0xff, /*   Logical Maximum (255) */
    0x05, 0x07, /*   Usage Page (Key Codes) */
    0x19, 0x00, /*   Usage Minimum (0) */
    0x29, 0xff, /*   Usage Maximum (255) */
    0x81, 0x00, /*   Input (Data, Array) */
    0xc0,       /* End Collection */
};

static void hid_keyboard_reset(void* dev)
{
    hid_keyboard* kb = (hid_keyboard*)dev;
    kb->leds = 0;
}

static void hid_keyboard_fill_pressed_keys(hid_keyboard* kb, uint8_t* pressed)
{
    size_t count = 0;

    for (size_t code_hi = 0; code_hi < 8; ++code_hi) {
        uint32_t keys
            = swap32(kb->ram, kb->keys_pressed[code_hi],0) | dram_load(kb->ram, kb->keys_pressed_now[code_hi],32);
        if (keys) {
            for (uint64_t code_lo = 0; code_lo < 32; ++code_lo) {
                if (bit_check(keys, code_lo)) {
                    // Report a pressed button
                    pressed[count++] = (code_hi << 5) | code_lo;
                    if (count == MAX_PRESSED_KEYS) {
                        return;
                    }
                }
            }
        }
    }
}

static void hid_keyboard_read_report(void* dev, uint8_t report_type, uint8_t report_id, uint32_t offset, uint8_t* val)
{
    hid_keyboard* kb = (hid_keyboard*)dev;

    if (report_type == REPORT_TYPE_INPUT) {
        if (offset == 0) {
            kb->input_report[0] = bit_cut(sizeof(kb->input_report), 0, 8);
            kb->input_report[1] = bit_cut(sizeof(kb->input_report), 8, 8);
            kb->input_report[2] = bit_cut(
                dram_load(kb->ram,kb->keys_pressed[7],32) | dram_load(kb->ram,kb->keys_pressed_now[7],32), 0, 8);
            kb->input_report[3] = 0;
            hid_keyboard_fill_pressed_keys(kb, &kb->input_report[4]);
        }
        if (offset < sizeof(kb->input_report)) {
            *val = kb->input_report[offset];
        }
    } else {
        *val = 0;
    }
}

static void hid_keyboard_write_report(void* dev, uint8_t report_type, uint8_t report_id, uint32_t offset, uint8_t val)
{
    hid_keyboard* kb = (hid_keyboard*)dev;

    if (report_type == REPORT_TYPE_OUTPUT) {
        if (offset < sizeof(kb->output_report)) {
            kb->output_report[offset] = val;
            if (offset == sizeof(kb->output_report) - 1) {
                kb->leds = kb->output_report[2];
            }
        }
    }
}

static void hid_keyboard_remove(void* dev)
{
    hid_keyboard* kb = (hid_keyboard*)dev;
    free(kb);
}

void hid_keyboard_press(hid_keyboard* kb, uint8_t key)
{
    // Key is guaranteed to be 1 byte according to HID spec
    if (key != HID_KEY_NONE) {
        uint32_t off         = key >> 5;
        uint32_t bit         = 1U << (key & 0x1F);
        uint32_t old_pressed = kb->keys_pressed_now[off] | bit;

        // Send input_available if it's an actual key press
        if (bit & ~old_pressed) {
            // Mark that this key was pressed, prevents loosing keypresses
            kb->keys_pressed[off] |= bit;
            kb->hid_dev.input_available(kb->hid_dev.host, 0);
        }
    }
}

void hid_keyboard_release(hid_keyboard* kb, uint8_t key)
{
    if (key != HID_KEY_NONE) {
        uint32_t off         = key >> 5;
        uint32_t bit         = 1U << (key & 0x1F);
        uint32_t old_pressed = kb->keys_pressed_now[off] | ~bit;

        // Send input_available if it's an actual key release
        if (old_pressed & bit) {
            kb->hid_dev.input_available(kb->hid_dev.host, 0);
        }
    }
}

hid_keyboard* hid_keyboard_init(uint16_t addr, DRAM& ram, PLIC* plic, uint8_t irq_num, fdt_node* fdt, i2c_bus_t* bus)
{
    hid_keyboard* kb = new hid_keyboard();
    kb->hid_dev.dev = &kb;

    kb->hid_dev.report_desc      = keyboard_hid_report_descriptor;
    kb->hid_dev.report_desc_size = sizeof(keyboard_hid_report_descriptor);
    kb->hid_dev.max_input_size   = sizeof(kb->input_report);
    kb->hid_dev.max_output_size  = sizeof(kb->output_report);
    kb->hid_dev.vendor_id        = 1;
    kb->hid_dev.product_id       = 1;
    kb->hid_dev.version_id       = 1;

    kb->hid_dev.reset        = hid_keyboard_reset;
    kb->hid_dev.read_report  = hid_keyboard_read_report;
    kb->hid_dev.write_report = hid_keyboard_write_report;
    kb->hid_dev.remove       = hid_keyboard_remove;

    i2c_hid_t(addr,ram,plic,irq_num,fdt,bus,&kb->hid_dev);

    return kb;
}