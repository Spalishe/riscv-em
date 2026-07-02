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
// TODO: Finish X11
#pragma once
#ifdef __PKG_X11
#include "../devices/hid/hid_keyboard.hpp"
#include "window.hpp"
#include <X11/Xlib.h>
#include <vulkan/vulkan_xlib.h>

struct AppWindow
{
	Display* x11Display = nullptr;
	Window x11Window	= 0;
	InputState input;
	HID_Keyboard* kb;
	int width  = 800;
	int height = 600;
};
inline bool InitializeNativeWindow(AppWindow& appWindow, const std::string& title)
{
	appWindow.x11Display = XOpenDisplay(nullptr);
	if(!appWindow.x11Display) return false;

	int screen			= DefaultScreen(appWindow.x11Display);
	appWindow.x11Window = XCreateSimpleWindow(
		appWindow.x11Display, RootWindow(appWindow.x11Display, screen),
		0, 0, appWindow.width, appWindow.height, 1,
		BlackPixel(appWindow.x11Display, screen), WhitePixel(appWindow.x11Display, screen));
	XStoreName(appWindow.x11Display, appWindow.x11Window, title.c_str());
	XMapWindow(appWindow.x11Display, appWindow.x11Window);
	XFlush(appWindow.x11Display);

	return true;
}

inline bool CreateVulkanSurface(VkInstance instance, const AppWindow& appWindow, VkSurfaceKHR& outSurface)
{
	VkXlibSurfaceCreateInfoKHR createInfo{};
	createInfo.sType  = VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR;
	createInfo.dpy	  = appWindow.x11Display;
	createInfo.window = appWindow.x11Window;

	VkResult result = vkCreateXlibSurfaceKHR(instance, &createInfo, nullptr, &outSurface);
	return result == VK_SUCCESS;
}

inline void TerminateNativeWindow(AppWindow& appWindow)
{
	if(appWindow.x11Display)
	{
		XDestroyWindow(appWindow.x11Display, appWindow.x11Window);
		XCloseDisplay(appWindow.x11Display);
		appWindow.x11Display = nullptr;
	}
}
#endif
