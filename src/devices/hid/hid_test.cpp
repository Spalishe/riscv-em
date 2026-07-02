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

#include "../../../include/devices/hid/hid_test.hpp"
#include "../../../include/devices/plic.hpp"
#include <chrono>
#include <thread>
using namespace std::chrono_literals;
HID_TestDevice::HID_TestDevice(Machine& cpu, fdt_node* fdt) : HIDOverI2C(cpu, fdt, generate_report_descriptor(HID_TestDevice::report_descriptor_items))
{
	// We will make test looping timer which will print test key

	uint16_t max_input_len = 10;
	hid_descriptor[0xa]	   = (max_input_len & 0xFF);		// 0x0A
	hid_descriptor[0xb]	   = ((max_input_len >> 8) & 0xFF); // 0x00

	thr = std::thread([this]()
	{
		// Даем гостевой ОС Linux время загрузиться и инициализировать I2C-драйвер
		std::this_thread::sleep_for(5000ms);

		while(true)
		{
			// ПАКЕТ 1: Нажатие клавиши 'A'
			// Общая длина: 10 байт (0x000A)
			hid_input_report_write({
				0x00,						 // Модификаторы (Shift/Ctrl отжаты)
				0x00,						 // Зарезервировано
				0x04,						 // Скан-код 'A' (нажата)
				0x00, 0x00, 0x00, 0x00, 0x00 // Остальные 5 мест пусты
			});

			// Дергаем прерывание, чтобы Linux пришел и прочитал этот пакет
			plic->set_pending(irq_num, true);

			// Даем Linux время обработать нажатие
			std::this_thread::sleep_for(50ms);
			plic->set_pending(irq_num, false);

			std::this_thread::sleep_for(50ms);

			// ПАКЕТ 2: Отжатие всех клавиш
			hid_input_report_write({ 0x00, // Модификаторы
									 0x00, // Зарезервировано
									 0x00, // Все клавиши отпущены
									 0x00, 0x00, 0x00, 0x00, 0x00 });

			// Снова дергаем прерывание, чтобы Linux зафиксировал отжатие
			plic->set_pending(irq_num, true);

			std::this_thread::sleep_for(50ms);
			plic->set_pending(irq_num, false);

			// Ждем секунду перед следующим циклом нажатия
			std::this_thread::sleep_for(1000ms);
		}
	});
}

void HID_TestDevice::hid_event_output_report_write()
{
}
void HID_TestDevice::hid_event_data_register_write()
{
}
void HID_TestDevice::hid_event_command_register_write()
{
	uint8_t arg	   = command_register[0];
	uint8_t opcode = command_register[1] & 0x0F;

	switch(opcode)
	{
		case 0x01: // RESET
		{
			io_offset = 0;

			input_report[0x00] = 0x00;
			input_report[0x01] = 0x00;
			plic->set_pending(irq_num, true);
			break;
		}
		case 0x08: // SET_POWER
			if(arg == 0x01)
			{
				printf("Device power set to ON\n");
				// is_powered_on = true;
			}
			else if(arg == 0x00)
			{
				// is_powered_on = false;
			}

			break;
	}
}
