#pragma once

#include "mmio.h"
#include "plic.hpp"
#include "../libfdt.hpp"
#include <cstdint>
#include <vector>

#define I2C_OC_ADDR_DEFAULT 0x10030000

#define I2C_AUTO_ADDR 0x0 // Auto-pick I2C device address

struct i2c_dev_t {
    // I2C bus address
    uint16_t addr;
    // Device-specific data
    void*    data;

    // Start transaction, return device availability
    bool (*start)(void* dev, bool is_write);
    // Return false on NACK or no data to read
    bool (*write)(void* dev, uint8_t byte);
    bool (*read)(void* dev, uint8_t* byte);
    // Stop the current transaction
    void (*stop)(void* dev);
    // Device cleanup
    void (*remove)(void* dev);
};

struct i2c_bus_t : public Device {

    std::vector<i2c_dev_t> devices;
    fdt_node*    fdt_n;
    PLIC*        plic;
    uint8_t          irq_num;

    i2c_bus_t(uint64_t base, DRAM& ram, PLIC* plic, uint8_t irq_num, fdt_node* fdt);
    
    uint16_t            sel_addr;
    uint16_t            clock;
    uint8_t             control;
    uint8_t             status;
    uint8_t             tx_byte;
    uint8_t             rx_byte;

    // Returns assigned device address or zero on error
    uint16_t i2c_attach_dev(i2c_dev_t dev_desc);

    // Get I2C controller FDT node for nested device nodes
    struct fdt_node* i2c_bus_fdt_node();

    i2c_dev_t* i2c_oc_get_dev(uint16_t addr);
    void i2c_oc_interrupt();

    uint64_t read(HART* hart,uint64_t addr, uint64_t size);
    void write(HART* hart, uint64_t addr, uint64_t size, uint64_t val);
};