#include "Vulkan.hpp"

namespace Functions
{
#define VK_FUNCTION_DEFINITION(Function)\
    PFN_##Function Function

#define VK_EXPORTED_FUNCTION(Function)\
    VK_FUNCTION_DEFINITION(Function)

#define VK_GLOBAL_FUNCTION(Function)\
    VK_FUNCTION_DEFINITION(Function)

#define VK_INSTANCE_FUNCTION(Function)\
    VK_FUNCTION_DEFINITION(Function)

#define VK_INSTANCE_FUNCTION_FROM_EXTENSION(Function, Extension)\
    VK_FUNCTION_DEFINITION(Function)

#define VK_DEVICE_FUNCTION(Function)\
    VK_FUNCTION_DEFINITION(Function)

#define VK_DEVICE_FUNCTION_FROM_EXTENSION(Function, Extension)\
    VK_FUNCTION_DEFINITION(Function)

#include "VulkanFunctions.inl"

#undef VK_FUNCTION_DEFINITION
}

bool const LoadExportedFunctions(HMODULE VulkanDLL)
{
#define VK_EXPORTED_FUNCTION(Function)\
    Functions::Function = reinterpret_cast<PFN_##Function>(::GetProcAddress(VulkanDLL, #Function));\
    if (!Functions::Function) {\
		return false;\
	}

#include "VulkanFunctions.inl"

    return true;
}

bool const LoadGlobalFunctions()
{
#define VK_GLOBAL_FUNCTION(Function)\
    Functions::Function = reinterpret_cast<PFN_##Function>(Functions::vkGetInstanceProcAddr(nullptr, #Function)); \
    if (!Functions::Function) {\
        return false; \
    }

#include "VulkanFunctions.inl"

    return true;
}

bool const LoadInstanceFunctions(VkInstance Instance)
{
#define VK_INSTANCE_FUNCTION(Function)\
	Functions::Function = reinterpret_cast<PFN_##Function>(Functions::vkGetInstanceProcAddr(Instance, #Function));\
	if (!Functions::Function) {\
		return false;\
	}

#include "VulkanFunctions.inl"

    return true;
}

bool const LoadInstanceExtensionFunctions(VkInstance Instance, std::unordered_set<std::string> const & ExtensionNames)
{
#define VK_INSTANCE_FUNCTION_FROM_EXTENSION(Function, Extension)\
	{\
		auto ExtensionFound = ExtensionNames.find(std::string(Extension));\
		if (ExtensionFound != ExtensionNames.end()) {\
			Functions::Function = reinterpret_cast<PFN_##Function>(Functions::vkGetInstanceProcAddr(Instance, #Function));\
			if (!Functions::Function) {\
				return false;\
			}\
		}\
	}

#include "VulkanFunctions.inl"

    return true;
}

bool const LoadDeviceFunctions(VkDevice Device)
{
#define VK_DEVICE_FUNCTION(Function)\
	Functions::Function = reinterpret_cast<PFN_##Function>(Functions::vkGetDeviceProcAddr(Device, #Function));\
	if (!Functions::Function) {\
		return false;\
	}

#include "VulkanFunctions.inl"

    return true;
}

bool const LoadDeviceExtensionFunctions(VkDevice Device)
{
#define VK_DEVICE_FUNCTION_FROM_EXTENSION(Function, Extension)\
	Functions::Function = reinterpret_cast<PFN_##Function>(Functions::vkGetDeviceProcAddr(Device, #Function));\
	if(!Functions::Function) {\
		return false;\
	}

#include "VulkanFunctions.inl"

    return true;
}


VkResult vkEnumerateInstanceExtensionProperties(char const * pLayerName, std::uint32_t * pPropertyCount, VkExtensionProperties * pProperties)
{
    return Functions::vkEnumerateInstanceExtensionProperties(pLayerName, pPropertyCount, pProperties);
}

VkResult vkEnumerateInstanceLayerProperties(std::uint32_t * pPropertyCount, VkLayerProperties * pProperties)
{
    return Functions::vkEnumerateInstanceLayerProperties(pPropertyCount, pProperties);
}

VkResult vkEnumeratePhysicalDevices(VkInstance instance, std::uint32_t * pPhysicalDeviceCount, VkPhysicalDevice * pPhysicalDevice)
{
    return Functions::vkEnumeratePhysicalDevices(instance, pPhysicalDeviceCount, pPhysicalDevice);
}

VkResult vkEnumerateDeviceExtensionProperties(VkPhysicalDevice device, char const * pLayerName, std::uint32_t * pPropertyCount, VkExtensionProperties * pProperties)
{
    return Functions::vkEnumerateDeviceExtensionProperties(device, pLayerName, pPropertyCount, pProperties);
}

VkResult vkEnumerateDeviceLayerProperties(VkPhysicalDevice device, std::uint32_t * pPropertyCount, VkLayerProperties * pProperties)
{
    return Functions::vkEnumerateDeviceLayerProperties(device, pPropertyCount, pProperties);
}


void vkGetPhysicalDeviceProperties(VkPhysicalDevice physicalDevice, VkPhysicalDeviceProperties * pProperties)
{
    Functions::vkGetPhysicalDeviceProperties(physicalDevice, pProperties);
}

void vkGetPhysicalDeviceFeatures(VkPhysicalDevice physicalDevice, VkPhysicalDeviceFeatures * pFeatures)
{
    Functions::vkGetPhysicalDeviceFeatures(physicalDevice, pFeatures);
}

void vkGetPhysicalDeviceQueueFamilyProperties(VkPhysicalDevice physicalDevice, std::uint32_t * pQueueFamilyPropertyCount, VkQueueFamilyProperties * pQueueFamilyProperties)
{
    Functions::vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, pQueueFamilyPropertyCount, pQueueFamilyProperties);
}

VkResult vkGetPhysicalDeviceSurfaceSupportKHR(VkPhysicalDevice physicalDevice, std::uint32_t queueFamilyIndex, VkSurfaceKHR surface, VkBool32 * pSupported)
{
    return Functions::vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, queueFamilyIndex, surface, pSupported);
}

VkResult vkGetPhysicalDeviceSurfaceFormatsKHR(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, std::uint32_t * pSurfaceFormatCount, VkSurfaceFormatKHR * pSurfaceFormats)
{
    return Functions::vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, pSurfaceFormatCount, pSurfaceFormats);
}

VkResult vkGetPhysicalDeviceSurfaceCapabilitiesKHR(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, VkSurfaceCapabilitiesKHR * pSurfaceCapabilities)
{
    return Functions::vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, pSurfaceCapabilities);
}

void vkGetDeviceQueue(VkDevice device, std::uint32_t queueFamilyIndex, std::uint32_t queueIndex, VkQueue * pQueue)
{
    Functions::vkGetDeviceQueue(device, queueFamilyIndex, queueIndex, pQueue);
}

VkResult vkGetSwapchainImagesKHR(VkDevice device, VkSwapchainKHR swapchain, std::uint32_t * pSwapchainImageCount, VkImage * pSwapchainImages)
{
    return Functions::vkGetSwapchainImagesKHR(device, swapchain, pSwapchainImageCount, pSwapchainImages);
}


VkResult vkCreateInstance(VkInstanceCreateInfo const * pCreateInfo, VkAllocationCallbacks const * pAllocator, VkInstance * pInstance)
{
    return Functions::vkCreateInstance(pCreateInfo, pAllocator, pInstance);
}

VkResult vkCreateDevice(VkPhysicalDevice physicalDevice, VkDeviceCreateInfo const * pCreateInfo, VkAllocationCallbacks const * pAllocator, VkDevice * pDevice)
{
    return Functions::vkCreateDevice(physicalDevice, pCreateInfo, pAllocator, pDevice);
}

VkResult vkCreateCommandPool(VkDevice device, VkCommandPoolCreateInfo const * pCreateInfo, VkAllocationCallbacks const * pAllocator, VkCommandPool * pCommandPool)
{
    return Functions::vkCreateCommandPool(device, pCreateInfo, pAllocator, pCommandPool);
}

VkResult vkCreateSemaphore(VkDevice device, VkSemaphoreCreateInfo const * pCreateInfo, VkAllocationCallbacks const * pAllocator, VkSemaphore * pSemaphore)
{
    return Functions::vkCreateSemaphore(device, pCreateInfo, pAllocator, pSemaphore);
}

VkResult vkCreateFramebuffer(VkDevice device, VkFramebufferCreateInfo const * pCreateInfo, VkAllocationCallbacks const * pAllocator, VkFramebuffer * pFramebuffer)
{
    return Functions::vkCreateFramebuffer(device, pCreateInfo, pAllocator, pFramebuffer);
}

VkResult vkCreateWin32SurfaceKHR(VkInstance instance, VkWin32SurfaceCreateInfoKHR const * pCreateInfo, VkAllocationCallbacks const * pAllocator, VkSurfaceKHR * pSurface)
{
    return Functions::vkCreateWin32SurfaceKHR(instance, pCreateInfo, pAllocator, pSurface);
}

VkResult vkCreateSwapchainKHR(VkDevice device, VkSwapchainCreateInfoKHR const * pCreateInfo, VkAllocationCallbacks const * pAllocator, VkSwapchainKHR * pSwapchain)
{
    return Functions::vkCreateSwapchainKHR(device, pCreateInfo, pAllocator, pSwapchain);
}


void vkDestroyInstance(VkInstance instance, VkAllocationCallbacks const * pAllocator)
{
    Functions::vkDestroyInstance(instance, pAllocator);
}

void vkDestroyDevice(VkDevice device, VkAllocationCallbacks const * pAllocator)
{
    Functions::vkDestroyDevice(device, pAllocator);
}

void vkDestroySurfaceKHR(VkInstance instance, VkSurfaceKHR surface, VkAllocationCallbacks const * pAllocator)
{
    Functions::vkDestroySurfaceKHR(instance, surface, pAllocator);
}

void vkDestroySwapchainKHR(VkDevice device, VkSwapchainKHR swapchain, VkAllocationCallbacks const * pAllocator)
{
    Functions::vkDestroySwapchainKHR(device, swapchain, pAllocator);
}


VkResult vkAllocateCommandBuffers(VkDevice device, VkCommandBufferAllocateInfo const * pAllocateInfo, VkCommandBuffer * pCommandBuffers)
{
    return Functions::vkAllocateCommandBuffers(device, pAllocateInfo, pCommandBuffers);
}

void vkFreeCommandBuffers(VkDevice device, VkCommandPool commandPool, std::uint32_t commandBufferCount, VkCommandBuffer * pCommandBuffers)
{
    return Functions::vkFreeCommandBuffers(device, commandPool, commandBufferCount, pCommandBuffers);
}


VkResult vkBeginCommandBuffer(VkCommandBuffer commandBuffer, VkCommandBufferBeginInfo const * pBeginInfo)
{
    return Functions::vkBeginCommandBuffer(commandBuffer, pBeginInfo);
}

VkResult vkEndCommandBuffer(VkCommandBuffer commandBuffer)
{
    return Functions::vkEndCommandBuffer(commandBuffer);
}

VkResult vkDeviceWaitIdle(VkDevice device)
{
    return Functions::vkDeviceWaitIdle(device);
}

VkResult vkResetCommandPool(VkDevice device, VkCommandPool commandPool, VkCommandPoolResetFlags flags)
{
    return Functions::vkResetCommandPool(device, commandPool, flags);
}

VkResult vkQueueSubmit(VkQueue queue, std::uint32_t submitCount, VkSubmitInfo const * pSubmits, VkFence fence)
{
    return Functions::vkQueueSubmit(queue, submitCount, pSubmits, fence);
}

VkResult vkQueuePresentKHR(VkQueue queue, VkPresentInfoKHR const * pPresentInfo)
{
    return Functions::vkQueuePresentKHR(queue, pPresentInfo);
}

VkResult vkAcquireNextImageKHR(VkDevice device, VkSwapchainKHR swapchain, std::uint64_t timeout, VkSemaphore semaphore, VkFence fence, std::uint32_t * pImageIndex)
{
    return Functions::vkAcquireNextImageKHR(device, swapchain, timeout, semaphore, fence, pImageIndex);
}

void vkCmdClearColorImage(VkCommandBuffer commandBuffer, VkImage image, VkImageLayout imageLayout, VkClearColorValue const * pColor, std::uint32_t rangeCount, VkImageSubresourceRange const * pRanges)
{
    Functions::vkCmdClearColorImage(commandBuffer, image, imageLayout, pColor, rangeCount, pRanges);
}

void vkCmdPipelineBarrier(VkCommandBuffer commandBuffer,
                          VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask,
                          VkDependencyFlags dependencyFlags,
                          std::uint32_t memoryBarrierCount, VkMemoryBarrier const * pMemoryBarriers,
                          std::uint32_t bufferMemoryBarrierCount, VkBufferMemoryBarrier const * pBufferMemoryBarriers,
                          std::uint32_t imageMemoryBarrierCount, VkImageMemoryBarrier const * pImageMemoryBarriers)
{
    return Functions::vkCmdPipelineBarrier(commandBuffer,
                                           srcStageMask, dstStageMask,
                                           dependencyFlags,
                                           memoryBarrierCount, pMemoryBarriers,
                                           bufferMemoryBarrierCount, pBufferMemoryBarriers,
                                           imageMemoryBarrierCount, pImageMemoryBarriers);
}

void vkCmdBeginRenderPass(VkCommandBuffer commandBuffer, VkRenderPassBeginInfo const * pRenderPassBegin, VkSubpassContents contents)
{
    Functions::vkCmdBeginRenderPass(commandBuffer, pRenderPassBegin, contents);
}

void vkCmdEndRenderPass(VkCommandBuffer commandBuffer)
{
    Functions::vkCmdEndRenderPass(commandBuffer);
}