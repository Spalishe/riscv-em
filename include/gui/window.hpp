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
#include <iostream>
#include <string>
#include <vector>
#include <vulkan/vulkan.h>
#ifdef __PKG_WAYLAND
#include <vulkan/vulkan_wayland.h>
#endif

enum : uint8_t
{
	KEY_LEFTSHIFT  = 42,
	KEY_RIGHTSHIFT = 54,

	KEY_LEFTCTRL  = 29,
	KEY_RIGHTCTRL = 97,

	KEY_LEFTALT	 = 56,
	KEY_RIGHTALT = 100
};
struct InputState
{
	uint8_t active[6]{};
	uint8_t active_count = 0;

	bool lctrl = false;
	bool rctrl = false;

	bool lshift = false;
	bool rshift = false;

	bool lalt = false;
	bool ralt = false;

	bool pressed[256]{};
};

static inline void key_press(InputState& s, uint32_t key)
{
	if(key < 256)
	{
		s.pressed[key] = true;
	}

	switch(key)
	{
		case KEY_LEFTSHIFT:
			s.lshift = true;
			return;
		case KEY_RIGHTSHIFT:
			s.rshift = true;
			return;

		case KEY_LEFTCTRL:
			s.lctrl = true;
			return;
		case KEY_RIGHTCTRL:
			s.rctrl = true;
			return;

		case KEY_LEFTALT:
			s.lalt = true;
			return;
		case KEY_RIGHTALT:
			s.ralt = true;
			return;
	}

	if(s.active_count < 6)
		s.active[s.active_count++] = key;
}
static inline void key_release(InputState& s, uint32_t key)
{
	if(key < 256)
	{
		s.pressed[key] = false;
	}

	switch(key)
	{
		case KEY_LEFTSHIFT:
			s.lshift = false;
			return;
		case KEY_RIGHTSHIFT:
			s.rshift = false;
			return;

		case KEY_LEFTCTRL:
			s.lctrl = false;
			return;
		case KEY_RIGHTCTRL:
			s.rctrl = false;
			return;

		case KEY_LEFTALT:
			s.lalt = false;
			return;
		case KEY_RIGHTALT:
			s.ralt = false;
			return;
	}

	// remove from rollover list (compact shift)
	for(uint8_t i = 0; i < s.active_count; i++)
	{
		if(s.active[i] == key)
		{
			for(uint8_t j = i; j + 1 < s.active_count; j++)
				s.active[j] = s.active[j + 1];

			s.active_count--;
			break;
		}
	}
}

struct AppWindow;

bool InitializeNativeWindow(AppWindow& appWindow, const std::string& title);
bool CreateVulkanSurface(VkInstance instance, const AppWindow& appWindow, VkSurfaceKHR& outSurface);
void TerminateNativeWindow(AppWindow& appWindow);
void StartEventLoop(AppWindow& appWindow);
void UpdateFrame(AppWindow& appWindow, const uint8_t* raw_pixels);
inline VkInstance CreateVulkanInstance()
{
	VkApplicationInfo appInfo{};
	appInfo.sType			 = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = "Unified Vulkan Engine";
	appInfo.apiVersion		 = VK_API_VERSION_1_2;

	// You must load global surface extensions and system-specific modules
	std::vector<const char*> extensions = { VK_KHR_SURFACE_EXTENSION_NAME };

#ifdef __PKG_WAYLAND
	extensions.push_back(VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME);
#endif
#ifdef __PKG_X11
	extensions.push_back(VK_KHR_XLIB_SURFACE_EXTENSION_NAME);
#endif

	VkInstanceCreateInfo createInfo{};
	createInfo.sType				   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo		   = &appInfo;
	createInfo.enabledExtensionCount   = static_cast<uint32_t>(extensions.size());
	createInfo.ppEnabledExtensionNames = extensions.data();

	VkInstance instance;
	if(vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create unified Vulkan Instance context.");
	}
	return instance;
}
