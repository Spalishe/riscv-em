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

#include "../include/devices/i2c_oc.hpp"
#include "../include/libfdt.hpp"
#include <vector>
#include <cstdint>

#define I2C_OC_MMIO_SIZE 0x1000

// OpenCores I2C registers
#define I2C_OC_CLKLO     0x00 // Clock prescale low byte
#define I2C_OC_CLKHI     0x04 // Clock prescale high byte
#define I2C_OC_CTR       0x08 // Control register
#define I2C_OC_TXRXR     0x0C // Transmit & Receive register (W/R)
#define I2C_OC_CRSR      0x10 // Command & Status Register (W/R)

// Register values
#define I2C_OC_CTR_MASK  0xC0 // Mask of legal bits
#define I2C_OC_CTR_EN    0x80 // Core enable bit
#define I2C_OC_CTR_IEN   0x40 // Interrupt enable bit

#define I2C_OC_CR_STA    0x80 // Generate (repeated) start condition
#define I2C_OC_CR_STO    0x40 // Generate stop condition
#define I2C_OC_CR_RD     0x20 // Read from slave
#define I2C_OC_CR_WR     0x10 // Write to slave
#define I2C_OC_CR_ACK    0x08 // Send ACK (0) or NACK (1) to master
#define I2C_OC_CR_IACK   0x01 // Interrupt acknowledge, clear a pending IRQ

#define I2C_OC_SR_ACK    0x80 // Received ACK from slave (0), NACK is 1
#define I2C_OC_SR_BSY    0x40 // I2C bus busy
#define I2C_OC_SR_AL     0x20 // Arbitration lost
#define I2C_OC_SR_TIP    0x02 // Transfer in progress
#define I2C_OC_SR_IF     0x01 // Interrupt flag

void i2c_bus_t::i2c_oc_interrupt()
{
    status |= I2C_OC_SR_IF;
    if (control & I2C_OC_CTR_IEN) {
        plic->raise(irq_num);
    }
}

i2c_dev_t* i2c_bus_t::i2c_oc_get_dev(uint16_t addr)
{
    for (i2c_dev_t &i : devices) {
        if (i.addr == addr) {
            return &i;
        }
    }
    return NULL;
}

uint64_t i2c_bus_t::read(HART* hart,uint64_t addr, uint64_t size)
{
    auto offset = base - addr;
    switch (offset) {
        case I2C_OC_CLKLO:
            return clock & 0xFF;
            break;
        case I2C_OC_CLKHI:
            return clock >> 8;
            break;
        case I2C_OC_CTR:
            return control;
            break;
        case I2C_OC_TXRXR:
            return rx_byte;
            break;
        case I2C_OC_CRSR:
            return status;
            break;
    }
    return -1;
}

void i2c_bus_t::write(HART* hart, uint64_t addr, uint64_t size, uint64_t val)
{
    auto offset = base - addr;
    switch (offset) {
        case I2C_OC_CLKLO:
            clock = (clock & 0xFF00) | dram_load(&ram,addr,8);
            break;
        case I2C_OC_CLKHI:
            clock = (clock & 0xFF) | (dram_load(&ram,addr,8) << 8);
            break;
        case I2C_OC_CTR:
            control = dram_load(&ram,addr,8) & I2C_OC_CTR_MASK;
            break;
        case I2C_OC_TXRXR:
            tx_byte = dram_load(&ram,addr,8);
            break;
        case I2C_OC_CRSR: {
            uint8_t cmd  = dram_load(&ram,addr,8);
            status |= I2C_OC_SR_ACK;
            if (cmd & I2C_OC_CR_IACK) {
                // Clear a pending interrupt
                status &= ~I2C_OC_SR_IF;
            }
            if (cmd & I2C_OC_CR_STA) {
                // Start the transaction
                sel_addr  = 0xFFFF;
                status   |= I2C_OC_SR_BSY;
            }
            if ((cmd & I2C_OC_CR_WR)) {
                if (sel_addr == 0xFFFF) {
                    // Get device address, signal start of transaction
                    sel_addr       = tx_byte >> 1;
                    i2c_dev_t* i2c_dev  = i2c_oc_get_dev(sel_addr);
                    bool       is_write = !(tx_byte & 1);
                    if (i2c_dev && (!i2c_dev->start || i2c_dev->start(i2c_dev->data, is_write))) {
                        status &= ~I2C_OC_SR_ACK;
                    }
                } else {
                    // Write byte
                    i2c_dev_t* i2c_dev = i2c_oc_get_dev(sel_addr);
                    if (i2c_dev && i2c_dev->write(i2c_dev->data, tx_byte)) {
                        status &= ~I2C_OC_SR_ACK;
                    }
                }
                i2c_oc_interrupt();
            }
            if (cmd & I2C_OC_CR_RD) {
                // Read byte
                i2c_dev_t* i2c_dev = i2c_oc_get_dev(sel_addr);
                if (i2c_dev && i2c_dev->read(i2c_dev->data, &rx_byte)) {
                    status &= ~I2C_OC_SR_ACK;
                }
                i2c_oc_interrupt();
            }
            if (cmd & I2C_OC_CR_STO) {
                // End of transaction
                i2c_dev_t* i2c_dev = i2c_oc_get_dev(sel_addr);
                if (i2c_dev && i2c_dev->stop) {
                    i2c_dev->stop(i2c_dev->data);
                }
                sel_addr  = 0xFFFF;
                status   &= ~I2C_OC_SR_BSY;
                i2c_oc_interrupt();
            }
            break;
        }
    }
}

i2c_bus_t::i2c_bus_t(uint64_t base, DRAM& ram, PLIC* plic, uint8_t irq_num, fdt_node* fdt)
    : Device(base, I2C_OC_MMIO_SIZE, ram)
    {    
        if(fdt != NULL) {
            struct fdt_node* i2c_osc = fdt_node_find(fdt, "i2c_osc");
            if (i2c_osc == NULL) {
                i2c_osc = fdt_node_create("i2c_osc");
                fdt_node_add_prop_str(i2c_osc, "compatible", "fixed-clock");
                fdt_node_add_prop_u32(i2c_osc, "#clock-cells", 0);
                fdt_node_add_prop_u32(i2c_osc, "clock-frequency", 20000000);
                fdt_node_add_prop_str(i2c_osc, "clock-output-names", "clk");
                fdt_node_add_child(fdt, i2c_osc);
            }

            struct fdt_node* i2c_fdt = fdt_node_create_reg("i2c", base);
            fdt_node_add_prop_reg(i2c_fdt, "reg", base, I2C_OC_MMIO_SIZE);
            fdt_node_add_prop_str(i2c_fdt, "compatible", "opencores,i2c-ocores");
            fdt_node_add_prop_u32(i2c_fdt, "interrupt-parent", fdt_node_get_phandle(fdt_node_find_reg(fdt_node_find(fdt,"soc"),"plic",0x0C000000)));
            fdt_node_add_prop_u32(i2c_fdt, "interrupts", irq_num);
            fdt_node_add_prop_u32(i2c_fdt, "clocks", fdt_node_get_phandle(i2c_osc));
            fdt_node_add_prop_str(i2c_fdt, "clock-names", "clk");
            fdt_node_add_prop_u32(i2c_fdt, "reg-shift", 2);
            fdt_node_add_prop_u32(i2c_fdt, "reg-io-width", 1);
            fdt_node_add_prop_u32(i2c_fdt, "opencores,ip-clock-frequency", 20000000);
            fdt_node_add_prop_u32(i2c_fdt, "#address-cells", 1);
            fdt_node_add_prop_u32(i2c_fdt, "#size-cells", 0);
            fdt_node_add_prop_str(i2c_fdt, "status", "okay");
            fdt_node_add_child(fdt_node_find(fdt,"soc"), i2c_fdt);
            this->fdt_n = fdt_node_find_reg(fdt_node_find(fdt,"soc"),"i2c",base);
        }
        this->plic = plic;
        this->irq_num = irq_num;
    }

uint16_t i2c_bus_t::i2c_attach_dev(i2c_dev_t dev_desc)
{
    i2c_dev_t tmp = dev_desc;
    if (dev_desc.addr == I2C_AUTO_ADDR) {
        tmp.addr = 0x8;
    }
    while (i2c_oc_get_dev(tmp.addr)) {
        if (dev_desc.addr == I2C_AUTO_ADDR) {
            tmp.addr++;
        } else {
            std::cerr << "Duplicate I2C device address on a single bus" << std::endl;
            return 0;
        }
    }
    devices.push_back(tmp);
    return tmp.addr;
}

struct fdt_node* i2c_bus_t::i2c_bus_fdt_node()
{
    return fdt_n;
}
