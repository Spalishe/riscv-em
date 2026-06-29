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
