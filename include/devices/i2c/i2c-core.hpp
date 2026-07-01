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

#pragma once
#include "../../device.hpp"
#include "i2c-slave.hpp"
#include <cstdint>
#include <memory>
#include <vector>

#define I2C_REG_PRER_LO 0x0
#define I2C_REG_PRER_HI 0x1
#define I2C_REG_CTR		0x2
#define I2C_REG_TXR_RXR 0x3
#define I2C_REG_CR_SR	0x4

#define I2C_CTR_EN_BIT	0x7
#define I2C_CTR_IEN_BIT 0x6

union i2c_cr
{
	struct
	{
		unsigned int IACK : 1;
		unsigned int : 2;
		unsigned int ACK : 1;
		unsigned int WR : 1;
		unsigned int RD : 1;
		unsigned int STO : 1;
		unsigned int STA : 1;
	} fields;

	uint8_t raw;
};
union i2c_sr
{
	struct
	{
		unsigned int IF : 1;
		unsigned int TIP : 1;
		unsigned int : 3;
		unsigned int Al : 1;
		unsigned int BUSY : 1;
		unsigned int RxACK : 1;
	} fields;

	uint8_t raw;
};

struct PLIC;
struct I2CSlave;
struct I2C : public Device
{
	I2C(uint64_t base, Machine& cpu, fdt_node* fdt);

	Machine& cpu;
	PLIC* plic;
	int irq_num;

	uint8_t prer_lo = 0xFF;
	uint8_t prer_hi = 0xFF;
	uint8_t ctr		= 0x00; // control register
	uint8_t txr		= 0x00; // transmit register
	uint8_t rxr		= 0x00; // receive register
	i2c_cr cr;				// command register
	i2c_sr sr;				// status register

	bool device_selected				  = false;
	uint8_t current_address				  = 0;
	std::shared_ptr<I2CSlave> current_dev = 0;
	bool is_read_operation				  = false;
	bool int_line						  = false;

	std::vector<std::shared_ptr<I2CSlave>> slaves;

	void raise_interrupt();
	void lower_interrupt();
	void handle_device_write(uint8_t data);
	uint8_t handle_device_read();
	void execute_command();

	template <typename T, typename... Args>
	std::shared_ptr<I2CSlave> create_device(Args&&... args);

	uint64_t read(uint64_t addr, MemorySize size);
	void write(uint64_t addr, MemorySize size, uint64_t val);
	static std::shared_ptr<I2C> init_auto(Machine& cpu);
};
