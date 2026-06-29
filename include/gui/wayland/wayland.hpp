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
#ifdef __PKG_WAYLAND
#include "../window.hpp"
extern "C"
{
#include "xdg-shell-client-protocol.h"
}
#include <cstring>
#include <iostream>
#include <vulkan/vulkan_wayland.h>
#include <wayland-client-core.h>
#include <wayland-client.h>

#include <algorithm>
#include <fcntl.h>
#include <mutex>
#include <sys/mman.h>
#include <thread>
#include <unistd.h>

struct WaylandWindow
{
	// Core Wayland Handles
	struct wl_display* display		 = nullptr;
	struct wl_registry* registry	 = nullptr;
	struct wl_compositor* compositor = nullptr;
	struct xdg_wm_base* xdg_wm_base	 = nullptr;

	// Window Surface Handles
	struct wl_surface* wl_surface	  = nullptr;
	struct xdg_surface* xdg_surface	  = nullptr;
	struct xdg_toplevel* xdg_toplevel = nullptr;

	struct wl_shm* shm				   = nullptr;
	struct wl_event_queue* event_queue = nullptr;
	bool configured					   = false;
	bool running					   = true;

	std::unique_ptr<std::mutex> window_mutex = std::make_unique<std::mutex>();
	struct wl_buffer* wl_buffer				 = nullptr;
	struct wl_shm_pool* wl_shm_pool			 = nullptr;
	uint32_t* pixel_shared_memory			 = nullptr;
	size_t shm_size							 = 0;
	int shm_fd								 = -1;
	int width								 = 0;
	int height								 = 0;
};
namespace
{
	static void xdg_surface_handle_configure(void* data, struct xdg_surface* xdg_surface, uint32_t serial)
	{
		xdg_surface_ack_configure(xdg_surface, serial);

		auto* window	   = static_cast<WaylandWindow*>(data);
		window->configured = true; // Signal that the window is ready for rendering
	}

	static const struct xdg_surface_listener xdg_surface_listener = {
		.configure = xdg_surface_handle_configure
	};

	static void xdg_wm_base_ping(void* data, struct xdg_wm_base* xdg_wm_base, uint32_t serial)
	{
		// respond to compositor pings to prove your application has not frozen
		xdg_wm_base_pong(xdg_wm_base, serial);
	}

	static const struct xdg_wm_base_listener xdg_wm_base_listener = {
		.ping = xdg_wm_base_ping
	};

	inline void xdg_toplevel_handle_close(void* data, struct xdg_toplevel* xdg_toplevel)
	{
		auto* window	= static_cast<WaylandWindow*>(data);
		window->running = false;
	}

	inline const struct xdg_toplevel_listener xdg_toplevel_listener = {
		.configure = [](void*, struct xdg_toplevel*, int32_t, int32_t, struct wl_array*) {},
		.close	   = xdg_toplevel_handle_close
	};

	static void registry_global_handler(void* data, struct wl_registry* registry, uint32_t id, const char* interface, uint32_t version)
	{
		auto* window = static_cast<WaylandWindow*>(data);

		if(std::strcmp(interface, "wl_compositor") == 0)
		{
			window->compositor = static_cast<struct wl_compositor*>(
				wl_registry_bind(registry, id, &wl_compositor_interface, 4));
		}

		else if(std::strcmp(interface, "xdg_wm_base") == 0)
		{
			window->xdg_wm_base = static_cast<struct xdg_wm_base*>(
				wl_registry_bind(registry, id, &xdg_wm_base_interface, 1));
			xdg_wm_base_add_listener(window->xdg_wm_base, &xdg_wm_base_listener, window);
		}

		else if(std::strcmp(interface, "wl_shm") == 0)
		{
			window->shm = static_cast<struct wl_shm*>(
				wl_registry_bind(registry, id, &wl_shm_interface, 1));
		}
	}

	static void registry_global_remove(void* data, struct wl_registry* registry, uint32_t id)
	{
		// i dont think you gonna physically disconnect your monitor while ts works, soooooooo...
	}

	static const struct wl_registry_listener registry_listener = {
		.global		   = registry_global_handler,
		.global_remove = registry_global_remove
	};

	static bool InitializeWaylandScreen(WaylandWindow& window, int width, int height)
	{
		window.display = wl_display_connect(nullptr);
		if(!window.display)
		{
			std::cerr << "Failed to connect to a Wayland display socket.\n";
			return false;
		}
		window.registry = wl_display_get_registry(window.display);
		wl_registry_add_listener(window.registry, &registry_listener, &window);

		wl_display_roundtrip(window.display);

		if(!window.compositor || !window.xdg_wm_base)
		{
			std::cerr << "Critical Error: Wayland compositor missing components.\n";
			return false;
		}

		window.wl_surface = wl_compositor_create_surface(window.compositor);
		if(!window.wl_surface) return false;

		window.xdg_surface = xdg_wm_base_get_xdg_surface(window.xdg_wm_base, window.wl_surface);
		xdg_surface_add_listener(window.xdg_surface, &xdg_surface_listener, &window);

		window.xdg_toplevel = xdg_surface_get_toplevel(window.xdg_surface);
		xdg_toplevel_add_listener(window.xdg_toplevel, &xdg_toplevel_listener, &window);

		xdg_toplevel_set_title(window.xdg_toplevel, "riscv-em");
		xdg_toplevel_set_min_size(window.xdg_toplevel, width, height);
		xdg_toplevel_set_max_size(window.xdg_toplevel, width, height);
		wl_surface_commit(window.wl_surface);
		while(!window.configured)
		{
			if(wl_display_dispatch(window.display) < 0)
			{
				return false;
			}
		}
		window.width	= width;
		window.height	= height;
		int stride		= width * 4;
		window.shm_size = stride * height;

		window.shm_fd = memfd_create("wayland-shm-pool", MFD_CLOEXEC);
		if(window.shm_fd >= 0)
		{
			if(ftruncate(window.shm_fd, window.shm_size) >= 0)
			{
				window.pixel_shared_memory = (uint32_t*)mmap(nullptr, window.shm_size,
															 PROT_READ | PROT_WRITE, MAP_SHARED, window.shm_fd, 0);

				if(window.pixel_shared_memory != MAP_FAILED)
				{
					std::fill_n(window.pixel_shared_memory, width * height, 0xFF000000);

					window.wl_shm_pool = wl_shm_create_pool(window.shm, window.shm_fd, window.shm_size);
					window.wl_buffer   = wl_shm_pool_create_buffer(window.wl_shm_pool, 0, width, height, stride, WL_SHM_FORMAT_XRGB8888);

					wl_surface_attach(window.wl_surface, window.wl_buffer, 0, 0);
					wl_surface_damage(window.wl_surface, 0, 0, width, height);
					wl_surface_commit(window.wl_surface);

					wl_display_flush(window.display);
					wl_display_dispatch_pending(window.display);
				}
			}
		}
		return true;
	}

	inline void StartWaylandEventThread(WaylandWindow& window)
	{
		window.event_queue = wl_display_create_queue(window.display);

		if(window.registry) wl_proxy_set_queue((struct wl_proxy*)window.registry, window.event_queue);
		if(window.xdg_wm_base) wl_proxy_set_queue((struct wl_proxy*)window.xdg_wm_base, window.event_queue);
		if(window.xdg_surface) wl_proxy_set_queue((struct wl_proxy*)window.xdg_surface, window.event_queue);
		if(window.xdg_toplevel) wl_proxy_set_queue((struct wl_proxy*)window.xdg_toplevel, window.event_queue);

		std::thread([&window]()
		{
			while(window.running)
			{
				{
					std::lock_guard<std::mutex> lock(*window.window_mutex);
					while(wl_display_prepare_read_queue(window.display, window.event_queue) != 0)
					{
						wl_display_dispatch_queue_pending(window.display, window.event_queue);
					}
					wl_display_flush(window.display);
				}

				if(wl_display_read_events(window.display) < 0)
				{
					break;
				}

				{
					std::lock_guard<std::mutex> lock(*window.window_mutex);
					wl_display_dispatch_queue_pending(window.display, window.event_queue);
				}
			}
		}).detach();
	}

	static void CleanupWaylandScreen(WaylandWindow& window)
	{
		if(window.display)
		{
			wl_display_flush(window.display);
		}

		if(window.xdg_toplevel) xdg_toplevel_destroy(window.xdg_toplevel);
		if(window.xdg_surface) xdg_surface_destroy(window.xdg_surface);
		if(window.wl_surface) wl_surface_destroy(window.wl_surface);

		if(window.wl_buffer) wl_buffer_destroy(window.wl_buffer);
		if(window.wl_shm_pool) wl_shm_pool_destroy(window.wl_shm_pool);
		if(window.pixel_shared_memory && window.pixel_shared_memory != MAP_FAILED) munmap(window.pixel_shared_memory, window.shm_size);
		if(window.shm_fd >= 0) close(window.shm_fd);

		if(window.xdg_wm_base) xdg_wm_base_destroy(window.xdg_wm_base);
		if(window.shm) wl_shm_destroy(window.shm);

		if(window.registry) wl_registry_destroy(window.registry);

		if(window.event_queue) wl_event_queue_destroy(window.event_queue);

		if(window.display) wl_display_disconnect(window.display);
	}

	inline void UpdateWaylandFrame(WaylandWindow& window, const uint8_t* raw_pixels)
	{
		if(!window.running || !window.wl_surface) return;
		if(!window.pixel_shared_memory || window.pixel_shared_memory == MAP_FAILED)
		{
			std::cerr << "[Wayland] Error: Shared memory is not mapped.\n";
			return;
		}

		if(!raw_pixels)
		{
			std::cerr << "[Wayland] Error: Source raw_pixels pointer is null.\n";
			return;
		}

		std::memcpy(window.pixel_shared_memory, raw_pixels, window.shm_size);

		wl_surface_damage(window.wl_surface, 0, 0, window.width, window.height);
		wl_surface_attach(window.wl_surface, window.wl_buffer, 0, 0);
		wl_surface_commit(window.wl_surface);

		wl_display_flush(window.display);
	}
}

struct AppWindow
{
	WaylandWindow wl;
	int width  = 800;
	int height = 600;
};
inline bool InitializeNativeWindow(AppWindow& appWindow, const std::string& title)
{
	(void)title;
	WaylandWindow window = {};
	bool out			 = InitializeWaylandScreen(window, appWindow.width, appWindow.height);
	if(out)
	{
		appWindow.wl = std::move(window);
	}
	return out;
}

inline bool CreateVulkanSurface(VkInstance instance, const AppWindow& appWindow, VkSurfaceKHR& outSurface)
{
	VkWaylandSurfaceCreateInfoKHR createInfo{};
	createInfo.sType   = VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR;
	createInfo.display = appWindow.wl.display;
	createInfo.surface = appWindow.wl.wl_surface;

	VkResult result = vkCreateWaylandSurfaceKHR(instance, &createInfo, nullptr, &outSurface);
	return result == VK_SUCCESS;
}

inline void TerminateNativeWindow(AppWindow& appWindow)
{
	appWindow.wl.running = false;
	CleanupWaylandScreen(appWindow.wl);
}
inline void StartEventLoop(AppWindow& appWindow)
{
	StartWaylandEventThread(appWindow.wl);
}
inline void UpdateFrame(AppWindow& appWindow, const uint8_t* raw_pixels)
{
	UpdateWaylandFrame(appWindow.wl, raw_pixels);
}
#include <wayland-util.h>

extern const struct wl_interface wl_surface_interface;
extern const struct wl_interface xdg_surface_interface;
extern const struct wl_interface xdg_toplevel_interface;
extern const struct wl_interface xdg_popup_interface;

inline const struct wl_message xdg_wm_base_requests[] = {
	{ "destroy",			 "",	 nullptr },
	{ "create_positioner", "n",	nullptr },
	{ "get_xdg_surface",	 "no", nullptr },
	{ "pong",			  "u",  nullptr },
};

inline const struct wl_message xdg_wm_base_events[] = {
	{ "ping", "u", nullptr },
};

inline const struct wl_interface xdg_wm_base_interface = {
	"xdg_wm_base",
	1,
	4,
	xdg_wm_base_requests,
	1,
	xdg_wm_base_events,
};

#endif
