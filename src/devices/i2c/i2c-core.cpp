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
#include "../../../include/devices/i2c/i2c-core.hpp"
#include "../../../include/devices/i2c/i2c-slave.hpp"
#include "../../../include/devices/plic.hpp"
#include "../../../include/libfdt.hpp"
#include "../../../include/machine.hpp"

I2C::I2C(uint64_t start, Machine& cpu, fdt_node* fdt)
	: Device(start, 0x20, fdt, cpu.mmap),
	  plic(cpu.mmio->get<PLIC>().get()), irq_num(plic->acquire_irq()),
	  cpu(cpu)
{
	sr.raw = 0x0;
	cr.raw = 0x0;

	cpu.mmap->add_region(start, size);
	if(fdt != NULL)
	{
		struct fdt_node* i2c_fdt = fdt_node_create_reg("i2c", start);
		fdt_node_add_prop_reg(i2c_fdt, "reg", start, size);
		fdt_node_add_prop_str(i2c_fdt, "compatible", "opencores,i2c-ocores");
		fdt_node_add_prop_u32(i2c_fdt, "interrupt-parent", fdt_node_get_phandle(fdt_node_find_reg(fdt_node_find(fdt, "soc"), "plic", 0x0C000000)));
		fdt_node_add_prop_u32(i2c_fdt, "interrupts", irq_num);
		fdt_node_add_prop_u32(i2c_fdt, "#address-cells", 1);
		fdt_node_add_prop_u32(i2c_fdt, "#size-cells", 0);
		fdt_node_add_prop_u32(i2c_fdt, "reg-shift", 0);
		fdt_node_add_prop_u32(i2c_fdt, "reg-io-width", 1);
		fdt_node_add_prop_u32(i2c_fdt, "opencores,ip-clock-frequency", 0x20000000);

		fdt_node_add_prop_str(i2c_fdt, "status", "okay");

		fdt_node_add_child(fdt_node_find(fdt, "soc"), i2c_fdt);
	}
}

std::shared_ptr<I2C> I2C::init_auto(Machine& cpu)
{
	return std::make_shared<I2C>(0x10030000, cpu, cpu.fdt);
}

template <typename T, typename... Args>
std::shared_ptr<I2CSlave> I2C::create_device(Args&&... args)
{
	auto new_device = std::make_shared<T>(std::forward<Args>(args)...);
	slaves.push_back(new_device);
	return new_device;
}

uint64_t I2C::read(uint64_t addr, MemorySize size)
{
	uint64_t offs = addr - start;
	switch(offs)
	{
		case I2C_REG_PRER_LO:
			return prer_lo;
		case I2C_REG_PRER_HI:
			return prer_hi;
		case I2C_REG_CTR:
			return ctr;
		case I2C_REG_TXR_RXR:
			return rxr;
		case I2C_REG_CR_SR:
			return sr.raw;
		default:
			return 0;
	}
}

void I2C::execute_command()
{
	// reset int
	if(cr.fields.IACK)
	{
		sr.fields.IF = 0;
		lower_interrupt();
	}

	sr.fields.TIP  = 1;
	sr.fields.BUSY = 0;

	// set dev addr
	if(cr.fields.STA && cr.fields.WR)
	{
		current_address	  = txr >> 1;
		is_read_operation = txr & 1;

		sr.fields.RxACK = 1;
		device_selected = false;
		for(auto dev : slaves)
		{
			if(dev->address == current_address)
			{
				current_dev = dev;
				current_dev->start_transmit();
				device_selected = true;
				sr.fields.RxACK = 0;
				break;
			}
		}
	}
	// write
	else if(cr.fields.WR && device_selected)
	{
		handle_device_write(txr);
		sr.fields.RxACK = 0;
	}
	// read
	else if(cr.fields.RD && device_selected)
	{
		rxr = handle_device_read();

		if(cr.fields.ACK)
		{
			// if guest sets ack to 1, it means that he ended his read.
			// for safety we will reset device internal buffer pointer, even if the system will call STO
			current_dev->buffer_ind = 0;
		}
		sr.fields.RxACK = 0;
	}

	// stop
	if(cr.fields.STO)
	{
		current_dev->stop_transmit();
		device_selected = false;
		sr.fields.BUSY	= 0; // release bus
	}

	sr.fields.TIP = 0; // transfer complete
	sr.fields.IF  = 1; // irq flag of transaction complete
	raise_interrupt();
}

void I2C::raise_interrupt()
{
	if(ctr & (1 << 6) && !int_line)
	{
		// IEN set
		int_line = true;
		plic->set_pending(irq_num, true);
	}
}

void I2C::lower_interrupt()
{
	if(int_line)
	{
		int_line = false;
		plic->set_pending(irq_num, false);
	}
}

void I2C::handle_device_write(uint8_t data)
{
	if(device_selected)
	{
		current_dev->dev_write(data);
	}
}
uint8_t I2C::handle_device_read()
{
	if(device_selected)
	{
		return current_dev->dev_read(cr.fields.ACK);
	}
	return 0;
}

void I2C::write(uint64_t addr, MemorySize size, uint64_t val)
{
	uint64_t offs = addr - start;
	switch(offs)
	{
		case I2C_REG_PRER_LO:
			prer_lo = val;
		case I2C_REG_PRER_HI:
			prer_hi = val;
		case I2C_REG_CTR:
			ctr = val;
			if(!(ctr & (1 << 7)))
			{
				sr.raw			= 0;
				device_selected = false;
				lower_interrupt();
			}
		case I2C_REG_TXR_RXR:
			txr = val;
		case I2C_REG_CR_SR:
			if(!(ctr & (1 << 7))) return;
			cr.raw = val;
			execute_command();
		default:
			return;
	}
	return;
}
