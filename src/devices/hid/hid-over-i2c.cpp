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

#include "../../../include/devices/hid/hid-over-i2c.hpp"
#include "../../../include/devices/plic.hpp"
#include "../../../include/machine.hpp"

HIDOverI2C::HIDOverI2C(Machine& cpu, fdt_node* fdt, std::vector<uint8_t> report_desc) : I2CSlave(cpu.mmio->get<PLIC>().get()->acquire_irq(), HID_I2C_BUFFER_SIZE),
																						plic(cpu.mmio->get<PLIC>().get()), irq_num(plic->last_irq()),
																						report_desc(report_desc),
																						cpu(cpu)
{
	hid_descriptor[0x4] = (report_desc.size() & 0xFF);
	hid_descriptor[0x5] = ((report_desc.size() >> 8) & 0xFF);

	input_report	 = new uint8_t[1024];
	output_report	 = new uint8_t[1024];
	data_register	 = new uint8_t[1024];
	command_register = new uint8_t[1024];

	if(fdt != NULL)
	{
		struct fdt_node* i2c_fdt = fdt_node_create_reg("i2c", irq_num);
		fdt_node_add_prop_u32(i2c_fdt, "reg", irq_num);
		fdt_node_add_prop_str(i2c_fdt, "compatible", "hid-over-i2c");
		fdt_node_add_prop_u32(i2c_fdt, "interrupt-parent", fdt_node_get_phandle(fdt_node_find_reg(fdt_node_find(fdt, "soc"), "plic", 0x0C000000)));
		fdt_node_add_prop_u32(i2c_fdt, "interrupts", irq_num);
		fdt_node_add_prop_u32(i2c_fdt, "hid-descr-addr", 0x00);
		fdt_node_add_prop_u32(i2c_fdt, "vdd-supply", 0x06);
		fdt_node_add_prop_u32(i2c_fdt, "vddl-supply", 0x06);
		fdt_node_add_prop(i2c_fdt, "wakeup-source", NULL, 0);

		fdt_node_add_child(fdt_node_find(fdt_node_find(fdt, "soc"), "i2c"), i2c_fdt);
	}
};

void HIDOverI2C::stop_transmit()
{
	write_cycle = 0;
	read_ind	= 0;
	if(last_write_event != nullptr)
		(*last_write_event)();
};
void HIDOverI2C::start_transmit()
{
	write_cycle		 = 0;
	read_ind		 = 0;
	last_write_event = nullptr;
};
uint8_t HIDOverI2C::dev_read(bool m_ack)
{
	uint8_t val = 0;

	if(cur_reg == 0x01)
	{
		if(read_ind < 30)
		{
			val = hid_descriptor[read_ind];
		}
	}
	else if(cur_reg == 0x02)
	{
		// Report descriptor
		if(read_ind < report_desc.size())
		{
			val = report_desc[read_ind];
		}
	}
	else if(cur_reg == 0x03)
	{
		// Input Report Register
		if(read_ind < 1024) val = input_report[read_ind];
	}
	else if(cur_reg == 0x05)
	{
		// Command Register
		if(read_ind < 1024) val = command_register[read_ind];
	}
	else if(cur_reg == 0x06)
	{
		// Data Register
		if(read_ind < 1024) val = data_register[read_ind];
	}

	if(!m_ack) read_ind++;
	return val;
}
void HIDOverI2C::dev_write(uint8_t val)
{
	if(write_cycle == 0)
	{
		cur_reg = val; // LSB
		write_cycle++;
	}
	else if(write_cycle == 1)
	{
		cur_reg |= ((uint16_t)val << 8); // MSB
		write_cycle++;
	}
	else
	{
		if(cur_reg == 0x5)
		{
			// Command register
			if((write_cycle - 2) < 1024) command_register[write_cycle - 2] = val;
			write_cycle++;
			last_write_event = &hid_event_command_register_write;
			return;
		}
		else if(cur_reg == 0x6)
		{
			// Data register
			if((write_cycle - 2) < 1024) data_register[write_cycle - 2] = val;
			write_cycle++;
			last_write_event = &hid_event_data_register_write;
			return;
		}
		else if(cur_reg == 0x07)
		{
			// Output Report Register
			if((write_cycle - 2) < 1024) output_report[write_cycle - 2] = val;
			write_cycle++;
			last_write_event = &hid_event_output_report_write;
			return;
		}
	}
}

void HIDOverI2C::hid_input_report_write(const std::vector<uint8_t>& bytes)
{
	input_report[0x00] = bytes.size() & 0xFF;
	input_report[0x01] = (bytes.size() >> 8) & 0xFF;
	for(int i = 0x0; i < bytes.size(); i++)
	{
		input_report[i + 2] = bytes[i];
	}
}

void HIDOverI2C::hid_data_register_write(const std::vector<uint8_t>& bytes)
{
	data_register[0x00] = bytes.size() & 0xFF;
	data_register[0x01] = (bytes.size() >> 8) & 0xFF;
	for(int i = 0x0; i < bytes.size(); i++)
	{
		data_register[i + 2] = bytes[i];
	}
}
