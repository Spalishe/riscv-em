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
#ifdef USE_FRAMEBUFFER
#pragma once
#include "../device.hpp"

#include "../gui/wayland/wayland.hpp"
#include "../gui/x11.hpp"

#include <chrono>

struct Framebuffer : public Device
{
	Framebuffer(uint64_t base, Machine& cpu, fdt_node* fdt, size_t width, size_t height, AppWindow& window);
	uint8_t* buffer;

	Machine& cpu;
	AppWindow& window;
	uint64_t read(uint64_t addr, MemorySize size);
	void write(uint64_t addr, MemorySize size, uint64_t val);
	void tick();
	static std::shared_ptr<Framebuffer> init_auto(Machine& cpu);

  private:
	std::chrono::high_resolution_clock::time_point last_frame_time;
};
#endif
