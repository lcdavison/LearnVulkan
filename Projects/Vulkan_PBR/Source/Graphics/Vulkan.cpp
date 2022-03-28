#include "Graphics/Vulkan.hpp"

namespace VulkanFunctions
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

#include "Graphics/VulkanFunctions.inl"

#undef VK_FUNCTION_DEFINITION
}

#include <Windows.h>

#include "CommonTypes.hpp"

#include "Graphics/Instance.hpp"
#include "Graphics/Device.hpp"
#include "Graphics/Viewport.hpp"

using namespace Vulkan;

/* The implementation for these functions can be found in MacroMagic.cpp */
/* Separated because the function implementation is a lil bit messy */
extern bool const LoadExportedFunctions(HMODULE);
extern bool const LoadGlobalFunctions();

static HMODULE VulkanLibraryHandle = {};

bool const VulkanModule::Start()
{
    bool bResult = false;

    VulkanLibraryHandle = ::LoadLibrary(TEXT("vulkan-1.dll"));
    if (VulkanLibraryHandle)
    {
        bResult = ::LoadExportedFunctions(VulkanLibraryHandle) &&
                  ::LoadGlobalFunctions();
    }
    else
    {
        ::MessageBox(nullptr, TEXT("Failed to load vulkan module"), TEXT("Fatal Error"), MB_OK);
    }

    return bResult;
}

bool const VulkanModule::Stop()
{
    return ::FreeLibrary(VulkanLibraryHandle) != 0u;
}

VkResult vkCreateInstance(VkInstanceCreateInfo const * pCreateInfo, VkAllocationCallbacks const * pAllocator, VkInstance * pInstance)
{
    return VulkanFunctions::vkCreateInstance(pCreateInfo, pAllocator, pInstance);
}

void vkDestroyInstance(VkInstance instance, VkAllocationCallbacks const * pAllocator)
{
    VulkanFunctions::vkDestroyInstance(instance, pAllocator);
}

inline VkResult vkCreateSemaphore(VkDevice device, VkSemaphoreCreateInfo const * pCreateInfo, VkAllocationCallbacks const * pAllocator, VkSemaphore * pSemaphore)
{
    return VulkanFunctions::vkCreateSemaphore(device, pCreateInfo, pAllocator, pSemaphore);
}

inline VkResult vkCreateFramebuffer(VkDevice device, VkFramebufferCreateInfo const * pCreateInfo, VkAllocationCallbacks const * pAllocator, VkFramebuffer * pFramebuffer)
{
    return VulkanFunctions::vkCreateFramebuffer(device, pCreateInfo, pAllocator, pFramebuffer);
}

inline VkResult vkBeginCommandBuffer(VkCommandBuffer commandBuffer, VkCommandBufferBeginInfo const * pBeginInfo)
{
    return VulkanFunctions::vkBeginCommandBuffer(commandBuffer, pBeginInfo);
}

inline VkResult vkEndCommandBuffer(VkCommandBuffer commandBuffer)
{
    return VulkanFunctions::vkEndCommandBuffer(commandBuffer);
}

inline VkResult vkDeviceWaitIdle(VkDevice device)
{
    return VulkanFunctions::vkDeviceWaitIdle(device);
}

inline VkResult vkResetCommandPool(VkDevice device, VkCommandPool commandPool, VkCommandPoolResetFlags flags)
{
    return VulkanFunctions::vkResetCommandPool(device, commandPool, flags);
}

inline VkResult vkQueueSubmit(VkQueue queue, uint32 submitCount, VkSubmitInfo const * pSubmits, VkFence fence)
{
    return VulkanFunctions::vkQueueSubmit(queue, submitCount, pSubmits, fence);
}

inline VkResult vkQueuePresentKHR(VkQueue queue, VkPresentInfoKHR const * pPresentInfo)
{
    return VulkanFunctions::vkQueuePresentKHR(queue, pPresentInfo);
}

inline VkResult vkAcquireNextImageKHR(VkDevice device, VkSwapchainKHR swapchain, uint64 timeout, VkSemaphore semaphore, VkFence fence, uint32 * pImageIndex)
{
    return VulkanFunctions::vkAcquireNextImageKHR(device, swapchain, timeout, semaphore, fence, pImageIndex);
}

inline void vkCmdClearColorImage(VkCommandBuffer commandBuffer, VkImage image, VkImageLayout imageLayout, VkClearColorValue const * pColor, uint32 rangeCount, VkImageSubresourceRange const * pRanges)
{
    VulkanFunctions::vkCmdClearColorImage(commandBuffer, image, imageLayout, pColor, rangeCount, pRanges);
}

inline void vkCmdPipelineBarrier(VkCommandBuffer commandBuffer,
                                 VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask,
                                 VkDependencyFlags dependencyFlags,
                                 uint32 memoryBarrierCount, VkMemoryBarrier const * pMemoryBarriers,
                                 uint32 bufferMemoryBarrierCount, VkBufferMemoryBarrier const * pBufferMemoryBarriers,
                                 uint32 imageMemoryBarrierCount, VkImageMemoryBarrier const * pImageMemoryBarriers)
{
    return VulkanFunctions::vkCmdPipelineBarrier(commandBuffer,
                                                 srcStageMask, dstStageMask,
                                                 dependencyFlags,
                                                 memoryBarrierCount, pMemoryBarriers,
                                                 bufferMemoryBarrierCount, pBufferMemoryBarriers,
                                                 imageMemoryBarrierCount, pImageMemoryBarriers);
}

inline void vkCmdBeginRenderPass(VkCommandBuffer commandBuffer, VkRenderPassBeginInfo const * pRenderPassBegin, VkSubpassContents contents)
{
    VulkanFunctions::vkCmdBeginRenderPass(commandBuffer, pRenderPassBegin, contents);
}

inline void vkCmdEndRenderPass(VkCommandBuffer commandBuffer)
{
    VulkanFunctions::vkCmdEndRenderPass(commandBuffer);
}