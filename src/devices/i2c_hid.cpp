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

#include "../../include/libfdt.hpp"
#include "../../include/devices/hid_api.hpp"
#include "../../include/devices/hid_dev.hpp"
#include "../../include/devices/i2c_oc.hpp"
#include "../../include/devices/i2c_hid.hpp"

static inline uint64_t bit_mask(uint64_t count)
{
    return (1ULL << count) - 1;
}

static inline uint64_t bit_replace(uint64_t val, uint64_t pos, uint64_t bits, uint64_t rep)
{
    return (val & (~(bit_mask(bits) << pos))) | ((rep & bit_mask(bits)) << pos);
}
static inline uint64_t bit_cut(uint64_t val, uint64_t pos, uint64_t bits)
{
    return (val >> pos) & bit_mask(bits);
}


void report_id_queue::report_id_queue_init()
{
    first = -1;
    last  = -1;
    for (int16_t i = 0; i < 256; i++) {
        list[i] = -1;
    }
}

void report_id_queue::report_id_queue_insert(uint8_t report_id)
{
    if (report_id == last || list[report_id] >= 0) {
        return;
    }
    if (first < 0) {
        first = report_id;
    } else {
        list[last] = report_id;
    }
    last = report_id;
}

int16_t report_id_queue::report_id_queue_get()
{
    return first;
}

void report_id_queue::report_id_queue_remove_at(uint8_t report_id)
{
    if (first < 0) {
        return;
    }
    if (report_id == first) {
        first = list[report_id];
        if (first < 0) {
            last = -1;
        }
    } else {
        int16_t prev = first;
        while (prev >= 0 && list[prev] != report_id) {
            prev = list[prev];
        }
        if (prev < 0) {
            return;
        }
        list[prev] = list[report_id];
    }
    list[report_id] = -1;
}

void i2c_hid_input_available(void* host, uint8_t report_id)
{
    i2c_hid_t* i2c_hid = (i2c_hid_t*)host;
    if (i2c_hid->is_reset) {
        i2c_hid->report_id_queue.report_id_queue_insert(report_id);
        i2c_hid->plic->raise(i2c_hid->irq_num);
    }
}

bool i2c_hid_t::i2c_hid_read_data_size(uint32_t offset, uint8_t val)
{
    if (offset < 2) {
        data_size = bit_replace(data_size, offset * 8, 8, val);
    }
    if (offset >= 1 && offset >= data_size) {
        return false;
    }
    return true;
}

void i2c_hid_t::i2c_hid_read_report(uint8_t report_type, uint8_t report_id, uint32_t offset,uint8_t* val)
{
    hid_dev->read_report(hid_dev->dev, report_type, report_id, offset, val);
    if (offset < 2) {
        data_size = bit_replace(data_size, offset * 8, 8, *val);
    }
    if (report_type == REPORT_TYPE_INPUT && offset >= 1
        && offset == (uint32_t)(data_size > 2 ? data_size - 1 : 1)) {
        report_id_queue.report_id_queue_remove_at(report_id);
        if (report_id_queue.report_id_queue_get() >= 0) {
            plic->raise(irq_num);
        } else {
            plic->clear(irq_num);
        }
    }
}

bool i2c_hid_t::i2c_hid_write_report(uint8_t report_type, uint8_t report_id, uint32_t offset,uint8_t val)
{
    if (!i2c_hid_read_data_size(offset, val)) {
        return false;
    }
    hid_dev->write_report(hid_dev->dev, report_type, report_id, offset, val);
    return true;
}

uint8_t i2c_hid_t::i2c_hid_read_reg(uint16_t reg, uint32_t offset)
{
    switch (reg) {
        case I2C_HID_DESC_REG: {
            uint16_t field_val;
            switch (offset / 2) {
                case wHIDDescLength:
                    field_val = 0x1e;
                    break;
                case bcdVersion:
                    field_val = 0x0100;
                    break;
                case wReportDescLength:
                    field_val = hid_dev->report_desc_size;
                    break;
                case wReportDescRegister:
                    field_val = I2C_HID_REPORT_REG;
                    break;
                case wInputRegister:
                    field_val = I2C_HID_INPUT_REG;
                    break;
                case wMaxInputLength:
                    field_val = hid_dev->max_input_size;
                    break;
                case wOutputRegister:
                    field_val = I2C_HID_OUTPUT_REG;
                    break;
                case wMaxOutputLength:
                    field_val = hid_dev->max_output_size;
                    break;
                case wCommandRegister:
                    field_val = I2C_HID_COMMAND_REG;
                    break;
                case wDataRegister:
                    field_val = I2C_HID_DATA_REG;
                    break;
                case wVendorID:
                    field_val = hid_dev->vendor_id;
                    break;
                case wProductID:
                    field_val = hid_dev->product_id;
                    break;
                case wVersionID:
                    field_val = hid_dev->version_id;
                    break;
                default:
                    field_val = 0;
                    break;
            }
            return bit_cut(field_val, 8 * (offset % 2), 8);
        }
        case I2C_HID_REPORT_REG:
            if (offset < hid_dev->report_desc_size) {
                return hid_dev->report_desc[offset];
            }
            break;
        case I2C_HID_INPUT_REG: {
            int16_t report_id = report_id_queue.report_id_queue_get();
            if (report_id < 0) {
                plic->clear(irq_num);
                return 0;
            }
            uint8_t val = 0;
            i2c_hid_read_report(REPORT_TYPE_INPUT, report_id, offset, &val);
            return val;
        }
        case I2C_HID_DATA_REG:
            switch (command) {
                case I2C_HID_COMMAND_GET_REPORT: {
                    uint8_t val = 0;
                    i2c_hid_read_report(report_type, report_id, offset, &val);
                    return val;
                }
                case I2C_HID_COMMAND_GET_IDLE: {
                    uint16_t field_val = 0;
                    switch (offset / 2) {
                        case 0:
                            field_val = 4;
                            break;
                        case 1:
                            if (hid_dev->get_idle) {
                                hid_dev->get_idle(hid_dev->dev, report_id, &field_val);
                            }
                            break;
                    }
                    return bit_cut(field_val, 8 * (offset % 2), 8);
                }
                case I2C_HID_COMMAND_GET_PROTOCOL: {
                    uint16_t field_val = 0;
                    switch (offset / 2) {
                        case 0:
                            field_val = 4;
                            break;
                        case 1:
                            if (hid_dev->get_protocol) {
                                hid_dev->get_protocol(hid_dev->dev, &field_val);
                            }
                            break;
                    }
                    return bit_cut(field_val, 8 * (offset % 2), 8);
                }
            }
            break;
    }
    return 0;
}

bool i2c_hid_t::i2c_hid_write_reg(uint16_t reg, uint32_t offset, uint8_t val)
{
    switch (reg) {
        case I2C_HID_OUTPUT_REG:
            return i2c_hid_write_report(REPORT_TYPE_OUTPUT, 0, offset, val);
        case I2C_HID_COMMAND_REG:
            switch (offset) {
                case 0:
                    report_id   = bit_cut(val, 0, 4);
                    report_type = bit_cut(val, 4, 2);
                    return true;
                case 1:
                    command = bit_cut(val, 0, 4);
                    if (report_id == 0xF) {
                        return true;
                    }
                    break;
                case 2:
                    report_id = val;
                    break;
            }
            switch (command) {
                case I2C_HID_COMMAND_SET_IDLE:
                    if (data_size == 4 && hid_dev->set_idle) {
                        hid_dev->set_idle(hid_dev->dev, report_id, data_val);
                    }
                    break;
                case I2C_HID_COMMAND_SET_PROTOCOL:
                    if (data_size == 4 && hid_dev->set_protocol) {
                        hid_dev->set_protocol(hid_dev->dev, data_val);
                    }
                    break;
                case I2C_HID_COMMAND_SET_POWER:
                    if (hid_dev->set_power) {
                        hid_dev->set_power(hid_dev->dev, report_id % 4);
                    }
                    break;
            }
            break;
        case I2C_HID_DATA_REG:
            if (command == I2C_HID_COMMAND_SET_REPORT) {
                return i2c_hid_write_report(report_type, report_id, offset, val);
            } else {
                if (!i2c_hid_read_data_size(offset, val)) {
                    return false;
                }
                if (offset / 2 == 1) {
                    data_val = bit_replace(data_val, offset * 8, 8, val);
                }
                return true;
            }
            break;
    }
    return false;
}

void i2c_hid_t::write(HART* hart, uint64_t addr, uint64_t size, uint64_t val)
{
    dram_store(&ram,addr,size,val);
}

uint64_t i2c_hid_t::read(HART* hart,uint64_t addr, uint64_t size)
{
    return dram_load(&ram,addr,size);
}


static void i2c_hid_reset(i2c_hid_t* i2c_hid, bool is_init)
{
    i2c_hid->report_id_queue.report_id_queue_init();
    i2c_hid->reg         = I2C_HID_INPUT_REG;
    i2c_hid->command     = 0;
    i2c_hid->report_type = 0;
    i2c_hid->report_id   = 0;
    i2c_hid->is_reset    = !is_init;

    if (i2c_hid->hid_dev->reset) {
        i2c_hid->hid_dev->reset(i2c_hid->hid_dev->dev);
    }

    if (!is_init) {
        i2c_hid->plic->raise(i2c_hid->irq_num);
    }
}


static bool i2c_hid_start(void* dev, bool is_write)
{
    i2c_hid_t* i2c_hid = (i2c_hid_t*)dev;
    i2c_hid->is_write  = is_write;
    i2c_hid->io_offset = 0;
    return true;
}

static bool i2c_hid_write(void* dev, uint8_t byte)
{
    i2c_hid_t* i2c_hid = (i2c_hid_t*)dev;
    switch (i2c_hid->io_offset) {
        case 0:
        case 1:
            i2c_hid->reg = bit_replace(i2c_hid->reg, i2c_hid->io_offset * 8, 8, byte);
            i2c_hid->io_offset++;
            break;
        default:
            if (i2c_hid->i2c_hid_write_reg(i2c_hid->reg, i2c_hid->io_offset - 2, byte)) {
                i2c_hid->io_offset++;
            } else {
                i2c_hid->io_offset = 0;
            }
            break;
    }
    return true;
}

static bool i2c_hid_read(void* dev, uint8_t* byte)
{
    i2c_hid_t* i2c_hid = (i2c_hid_t*)dev;
    *byte = i2c_hid->i2c_hid_read_reg(i2c_hid->reg, i2c_hid->io_offset++);
    return true;
}

static void i2c_hid_stop(void* dev)
{
    i2c_hid_t* i2c_hid = (i2c_hid_t*)dev;
    // fprintf(stderr, "i2c_hid_stop\n");
    i2c_hid->is_reset = false;
    if (i2c_hid->command == I2C_HID_COMMAND_RESET) {
        i2c_hid_reset(i2c_hid,false);
    }
    i2c_hid->reg       = I2C_HID_INPUT_REG;
    i2c_hid->command   = 0;
    i2c_hid->data_size = 0;
}

static void i2c_hid_remove(void* dev)
{
    i2c_hid_t* i2c_hid = (i2c_hid_t*)dev;
    if (i2c_hid->hid_dev->remove) {
        i2c_hid->hid_dev->remove(i2c_hid->hid_dev->dev);
    }
    free(i2c_hid);
}

i2c_hid_t::i2c_hid_t(uint16_t base, DRAM& ram, PLIC* plic, uint8_t irq_num, fdt_node* fdt, i2c_bus_t* bus, hid_dev_t* hid_dev)
    : Device(base, 1, ram)
    {    
        if(fdt != NULL) {
            struct fdt_node* i2c_vdd = fdt_node_find(fdt, "i2c_vdd");
            if (i2c_vdd == NULL) {
                i2c_vdd = fdt_node_create("i2c_vdd");
                fdt_node_add_prop_str(i2c_vdd, "compatible", "regulator-fixed");
                fdt_node_add_prop_str(i2c_vdd, "regulator-name", "i2c_vdd");
                fdt_node_add_child(fdt, i2c_vdd);
            }

            struct fdt_node* i2c_fdt = fdt_node_create_reg("i2c", base);
            fdt_node_add_prop_u32(i2c_fdt, "reg", base);
            fdt_node_add_prop_str(i2c_fdt, "compatible", "hid-over-i2c");
            fdt_node_add_prop_u32(i2c_fdt, "hid-descr-addr", I2C_HID_DESC_REG);
            fdt_node_add_prop_u32(i2c_fdt, "vdd-supply", fdt_node_get_phandle(i2c_vdd));
            fdt_node_add_prop_u32(i2c_fdt, "vddl-supply", fdt_node_get_phandle(i2c_vdd));
            fdt_node_add_prop(i2c_fdt, "wakeup-source", NULL, 0);
            fdt_node_add_prop_u32(i2c_fdt, "interrupt-parent", fdt_node_get_phandle(fdt_node_find_reg(fdt_node_find(fdt,"soc"),"plic",0x0C000000)));
            fdt_node_add_prop_u32(i2c_fdt, "interrupts", irq_num);
            fdt_node_add_child(bus->i2c_bus_fdt_node(), i2c_fdt);
        }
        this->plic = plic;
        this->irq_num = irq_num;
        i2c_dev_t i2c_dev = i2c_dev_t();
        i2c_dev.addr = base;
        i2c_dev.data = this;
        i2c_dev.read = i2c_hid_read;
        i2c_dev.write = i2c_hid_write;
        i2c_dev.start = i2c_hid_start;
        i2c_dev.stop = i2c_hid_stop;
        i2c_dev.remove = i2c_hid_remove;
        
        i2c_dev.addr = bus->i2c_attach_dev(i2c_dev);
        
        this->hid_dev = hid_dev;
        hid_dev->host = this;
        hid_dev->input_available = i2c_hid_input_available;
        is_write  = is_write;
        io_offset = 0;

        i2c_hid_reset(this, true);
    }
