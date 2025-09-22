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
#include <queue>

struct PLIC;

struct UART : public Device {
    // UART registers
    uint8_t rhr = 0;      // Receiver Holding Register (read)
    uint8_t thr = 0;      // Transmitter Holding Register (write)
    uint8_t ier = 0;      // Interrupt Enable Register
    uint8_t iir = 0x01;   // Interrupt Identification Register (no interrupt pending)
    uint8_t fcr = 0;      // FIFO Control Register
    uint8_t lcr = 0;      // Line Control Register
    uint8_t mcr = 0;      // Modem Control Register
    uint8_t lsr = 0x60;   // Line Status Register (THR empty + line idle)
    uint8_t msr = 0;      // Modem Status Register
    uint8_t scr = 0;      // Scratch Register
    
    // Divisor latch registers (when LCR[7] = 1)
    uint8_t dll = 0;      // Divisor Latch Low
    uint8_t dlm = 0;      // Divisor Latch High
    
    PLIC* plic;
    int irq_num;
    bool dlab = false;    // Divisor Latch Access Bit (from LCR[7])

    std::queue<uint8_t> rx_buffer;

    UART(uint64_t base, DRAM& ram, PLIC* plic, int irq_num, fdt_node* fdt, uint8_t hartcount);

    void trigger_irq();
    void clear_irq();
    void update_iir();
    uint64_t read(HART* hart, uint64_t addr, uint64_t size);
    void write(HART* hart, uint64_t addr, uint64_t size, uint64_t value);
    void receive_byte(uint8_t byte);
    void reset();
};