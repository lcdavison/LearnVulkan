#pragma once

#include "Vulkan.hpp"

#include <Windows.h>
#include <vulkan/vulkan_win32.h>

VULKAN_WRAPPER_API VkResult vkCreateWin32SurfaceKHR(VkInstance instance, struct VkWin32SurfaceCreateInfoKHR const * pCreateInfo, VkAllocationCallbacks const * pAllocator, VkSurfaceKHR * pSurface);