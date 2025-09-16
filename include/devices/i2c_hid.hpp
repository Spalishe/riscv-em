#pragma once

#include "mmio.h"
#include "hid_dev.hpp"
#include "i2c_oc.hpp"
#include "plic.hpp"


#define I2C_HID_DESC_REG             1
#define I2C_HID_REPORT_REG           2
#define I2C_HID_INPUT_REG            3
#define I2C_HID_OUTPUT_REG           4
#define I2C_HID_COMMAND_REG          5
#define I2C_HID_DATA_REG             6

#define I2C_HID_COMMAND_RESET        1
#define I2C_HID_COMMAND_GET_REPORT   2
#define I2C_HID_COMMAND_SET_REPORT   3
#define I2C_HID_COMMAND_GET_IDLE     4
#define I2C_HID_COMMAND_SET_IDLE     5
#define I2C_HID_COMMAND_GET_PROTOCOL 6
#define I2C_HID_COMMAND_SET_PROTOCOL 7
#define I2C_HID_COMMAND_SET_POWER    8

enum {
    wHIDDescLength,
    bcdVersion,
    wReportDescLength,
    wReportDescRegister,
    wInputRegister,
    wMaxInputLength,
    wOutputRegister,
    wMaxOutputLength,
    wCommandRegister,
    wDataRegister,
    wVendorID,
    wProductID,
    wVersionID,
};

struct report_id_queue {
    int16_t first;
    int16_t last;
    int16_t list[256];

    void report_id_queue_init();
    void report_id_queue_insert(uint8_t report_id);
    int16_t report_id_queue_get();
    void report_id_queue_remove_at(uint8_t report_id);
};

struct i2c_hid_t : public Device{
    hid_dev_t* hid_dev;

    // IRQ data
    PLIC* plic;
    uint8_t   irq_num;

    i2c_hid_t(uint16_t base, DRAM& ram, PLIC* plic, uint8_t irq_num, fdt_node* fdt, i2c_bus_t* bus, hid_dev_t* hid_dev);

    struct report_id_queue report_id_queue;

    // i2c IO state
    bool     is_write;
    int32_t  io_offset;
    uint16_t reg;
    uint8_t  command;
    uint8_t  report_type;
    uint8_t  report_id;
    uint16_t data_size;
    uint16_t data_val;
    bool     is_reset;

    uint64_t read(HART* hart,uint64_t addr, uint64_t size);
    void write(HART* hart, uint64_t addr, uint64_t size, uint64_t val);

    bool i2c_hid_read_data_size(uint32_t offset, uint8_t val);
    void i2c_hid_read_report(uint8_t report_type, uint8_t report_id, uint32_t offset,uint8_t* val);
    bool i2c_hid_write_report(uint8_t report_type, uint8_t report_id, uint32_t offset,uint8_t val);
    uint8_t i2c_hid_read_reg(uint16_t reg, uint32_t offset);
    bool i2c_hid_write_reg(uint16_t reg, uint32_t offset, uint8_t val);
};

void i2c_hid_input_available(void* host, uint8_t report_id);