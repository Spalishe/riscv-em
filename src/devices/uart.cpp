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

#include "../../include/devices/uart.hpp"
#include "../../include/devices/plic.hpp"
#include "../../include/libfdt.hpp"

UART::UART(uint64_t base, Machine& cpu, PLIC* plic, int irq_num, fdt_node* fdt)
        : Device(base, 0x100, cpu), plic(plic), irq_num(irq_num)
    {
        struct fdt_node* uart_fdt = fdt_node_create_reg("serial", base);
        fdt_node_add_prop_reg(uart_fdt, "reg", base, 0x100);
        fdt_node_add_prop_str(uart_fdt, "compatible", "ns16550a");
        fdt_node_add_prop_u32(uart_fdt, "clock-frequency", 20000000);
        fdt_node_add_prop_u32(uart_fdt, "fifo-size", 16);
        fdt_node_add_prop_str(uart_fdt, "status", "okay");
        fdt_node_add_prop_u32(uart_fdt, "interrupt-parent", fdt_node_get_phandle(fdt_node_find_reg(fdt_node_find(fdt,"soc"),"plic",0x0C000000)));
        fdt_node_add_prop_u32(uart_fdt, "interrupts", irq_num);
        fdt_node_add_child(fdt_node_find(fdt,"soc"), uart_fdt);

        struct fdt_node* chosen = fdt_node_find(fdt, "chosen");
        fdt_node_add_prop_str(chosen, "stdout-path", "/soc/serial@10000000");
    }

void UART::trigger_irq() {
    plic->raise(irq_num);
}

void UART::clear_irq() {
    plic->clear(irq_num);
}

uint8_t UART::calc_iir_locked() {
    // choose RX priority over TX
    if ((ier & 0x01) && (lsr & LSR_DATA_READY)) {
        // received data available
        return IIR_RX_AVAILABLE;
    }
    if ((ier & 0x02) && (lsr & LSR_THR_EMPTY)) {
        // THR empty
        return IIR_THR_EMPTY;
    }
    return IIR_NO_INT;
}

void UART::update_iir() {
    iir = calc_iir_locked();
    if (iir != IIR_NO_INT) {
        trigger_irq();
    } else {
        clear_irq();
    }
}

uint64_t UART::read(HART* hart, uint64_t addr, uint64_t size) {
    uint64_t offset = addr - base;
    uint8_t reg = offset;
    uint64_t value = 0;

    switch (reg) {
        case 0: // RHR (read) or DLL (if DLAB=1)
            if (dlab) {
                value = dll;
            } else {
                if(fifo_enabled) {
                    value = fifo_buffer.front();
                    fifo_buffer.pop();
                    if(fifo_buffer.size() == 0) lsr &= ~0x01;
                } else {
                    value = rhr;
                    lsr &= ~0x01; // Clear data ready flag after read
                }
                update_iir();
            }
            break;
            
        case 1: // IER (read) or DLM (if DLAB=1)
            if (dlab) {
                value = dlm;
            } else {
                value = ier;
            }
            break;
            
        case 2: // IIR (read)
            value = iir;
            // Reading IIR clears highest priority interrupt
            if (iir == 0x02) { // TX interrupt
                lsr &= ~0x20; // Clear THR empty flag temporarily
            } else if (iir == 0x04) { // RX interrupt
                lsr &= ~0x01; // Clear data ready flag
            }
            update_iir();
            break;
            
        case 3: // LCR (read)
            value = lcr;
            break;
            
        case 4: // MCR (read)
            value = mcr;
            break;
            
        case 5: // LSR (read)
            value = lsr;
            break;
            
        case 6: // MSR (read)
            value = msr;
            break;
            
        case 7: // SCR (read)
            value = scr;
            break;
            
        default:
            break;
    }

    return value;
}

void UART::write(HART* hart, uint64_t addr, uint64_t size, uint64_t value) {
    uint64_t offset = addr - base;
    uint8_t reg = offset;
    uint8_t byte_value = value & 0xFF;

    switch (reg) {
        case 0: // THR write
            if (dlab) {
                dll = byte_value;
            } else {
                thr = byte_value;

                std::putchar(thr);
                std::fflush(stdout);

                lsr |= LSR_THR_EMPTY; // THR empty
                lsr |= LSR_TEMT;      // Transmitter empty

                if (ier & 0x02) { 
                    trigger_irq();
                }

                update_iir();
            }
            break;
            
        case 1: // IER (write) or DLM (if DLAB=1)
            if (dlab) {
                dlm = byte_value;
            } else {
                ier = byte_value & 0x0F; // Only lower 4 bits are valid
                update_iir();
            }
            break;
            
        case 2: // FCR (write) - FIFO Control Register
            fcr = byte_value;
            fifo_enabled = (fcr & 0x01);
            if (fcr & 0x02) { // clear RX FIFO
                std::queue<uint8_t> empty; 
                fifo_buffer.swap(empty);
            }
            if (fcr & 0x04) { // clear TX FIFO 
                //TX Buffer Clear
                //Empty rn cuz i have no buffer
            }

            break;
            
        case 3: // LCR (write)
            lcr = byte_value;
            dlab = (lcr & 0x80) != 0; // Update DLAB from bit 7
            break;
            
        case 4: // MCR (write)
            mcr = byte_value;
            break;
            
        case 5: // LSR (read-only)
            // Ignore writes to read-only register
            break;
            
        case 6: // MSR (read-only)
            // Ignore writes to read-only register
            break;
            
        case 7: // SCR (write)
            scr = byte_value;
            break;
            
        default:
            // Ignore writes to unknown registers
            break;
    }
}

void UART::receive_byte(uint8_t byte) {
    if(fifo_enabled) {
        if (fifo_buffer.size() < 16) {
            fifo_buffer.push(byte);
        }
    } else {
        rhr = byte;
    }
    lsr |= LSR_DATA_READY; // Data Ready
    
    if (ier & 0x01) { 
        trigger_irq();
    }

    update_iir();
}

// idk why but why not
void UART::reset() {
    rhr = 0;
    thr = 0;
    ier = 0;
    iir = 0x01;
    fcr = 0;
    lcr = 0;
    mcr = 0;
    lsr = 0x60 | 0x40; // THR empty (0x20) + TEMT (0x40) + line idle
    msr = 0;
    scr = 0;
    dll = 0;
    dlm = 0;
    dlab = false;
}
