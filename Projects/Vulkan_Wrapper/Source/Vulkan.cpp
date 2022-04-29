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

void vkGetPhysicalDeviceMemoryProperties(VkPhysicalDevice physicalDevice, VkPhysicalDeviceMemoryProperties * pMemoryProperties)
{
    Functions::vkGetPhysicalDeviceMemoryProperties(physicalDevice, pMemoryProperties);
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

void vkGetBufferMemoryRequirements(VkDevice device, VkBuffer buffer, VkMemoryRequirements * pMemoryRequirements)
{
    Functions::vkGetBufferMemoryRequirements(device, buffer, pMemoryRequirements);
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

VkResult vkCreateFence(VkDevice device, VkFenceCreateInfo const * pCreateInfo, VkAllocationCallbacks const * pAllocator, VkFence * pFence)
{
    return Functions::vkCreateFence(device, pCreateInfo, pAllocator, pFence);
}

VkResult vkCreateRenderPass(VkDevice device, VkRenderPassCreateInfo const * pCreateInfo, VkAllocationCallbacks const * pAllocator, VkRenderPass * pRenderPass)
{
    return Functions::vkCreateRenderPass(device, pCreateInfo, pAllocator, pRenderPass);
}

VkResult vkCreateFramebuffer(VkDevice device, VkFramebufferCreateInfo const * pCreateInfo, VkAllocationCallbacks const * pAllocator, VkFramebuffer * pFramebuffer)
{
    return Functions::vkCreateFramebuffer(device, pCreateInfo, pAllocator, pFramebuffer);
}

VkResult vkCreateImageView(VkDevice device, VkImageViewCreateInfo const * pCreateInfo, VkAllocationCallbacks const * pAllocator, VkImageView * pImageView)
{
    return Functions::vkCreateImageView(device, pCreateInfo, pAllocator, pImageView);
}

VkResult vkCreateShaderModule(VkDevice device, VkShaderModuleCreateInfo const * pCreateInfo, VkAllocationCallbacks const * pAllocator, VkShaderModule * pShaderModule)
{
    return Functions::vkCreateShaderModule(device, pCreateInfo, pAllocator, pShaderModule);
}

VkResult vkCreateDescriptorPool(VkDevice device, VkDescriptorPoolCreateInfo const * pCreateInfo, VkAllocationCallbacks const * pAllocator, VkDescriptorPool * pDescriptorPool)
{
    return Functions::vkCreateDescriptorPool(device, pCreateInfo, pAllocator, pDescriptorPool);
}

VkResult vkCreateDescriptorSetLayout(VkDevice device, VkDescriptorSetLayoutCreateInfo const * pCreateInfo, VkAllocationCallbacks const * pAllocator, VkDescriptorSetLayout * pSetLayout)
{
    return Functions::vkCreateDescriptorSetLayout(device, pCreateInfo, pAllocator, pSetLayout);
}

VkResult vkCreatePipelineLayout(VkDevice device, VkPipelineLayoutCreateInfo const * pCreateInfo, VkAllocationCallbacks const * pAllocator, VkPipelineLayout * pPipelineLayout)
{
    return Functions::vkCreatePipelineLayout(device, pCreateInfo, pAllocator, pPipelineLayout);
}

VkResult vkCreateGraphicsPipelines(VkDevice device, VkPipelineCache pipelineCache, std::uint32_t createInfoCount, VkGraphicsPipelineCreateInfo const * pCreateInfos, VkAllocationCallbacks const * pAllocator, VkPipeline * pPipelines)
{
    return Functions::vkCreateGraphicsPipelines(device, pipelineCache, createInfoCount, pCreateInfos, pAllocator, pPipelines);
}

VkResult vkCreateBuffer(VkDevice device, VkBufferCreateInfo const * pCreateInfo, VkAllocationCallbacks const * pAllocator, VkBuffer * pBuffer)
{
    return Functions::vkCreateBuffer(device, pCreateInfo, pAllocator, pBuffer);
}

VkResult vkCreateBufferView(VkDevice device, VkBufferViewCreateInfo const * pCreateInfo, VkAllocationCallbacks const * pAllocator, VkBufferView * pView)
{
    return Functions::vkCreateBufferView(device, pCreateInfo, pAllocator, pView);
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

void vkDestroyCommandPool(VkDevice device, VkCommandPool commandPool, VkAllocationCallbacks const * pAllocator)
{
    Functions::vkDestroyCommandPool(device, commandPool, pAllocator);
}

void vkDestroySemaphore(VkDevice device, VkSemaphore semaphore, VkAllocationCallbacks const * pAllocator)
{
    Functions::vkDestroySemaphore(device, semaphore, pAllocator);
}

void vkDestroyFence(VkDevice device, VkFence fence, VkAllocationCallbacks const * pAllocator)
{
    Functions::vkDestroyFence(device, fence, pAllocator);
}

void vkDestroyRenderPass(VkDevice device, VkRenderPass renderPass, VkAllocationCallbacks const * pAllocator)
{
    Functions::vkDestroyRenderPass(device, renderPass, pAllocator);
}

void vkDestroyFramebuffer(VkDevice device, VkFramebuffer framebuffer, VkAllocationCallbacks const * pAllocator)
{
    Functions::vkDestroyFramebuffer(device, framebuffer, pAllocator);
}

void vkDestroyImageView(VkDevice device, VkImageView imageView, VkAllocationCallbacks const * pAllocator)
{
    Functions::vkDestroyImageView(device, imageView, pAllocator);
}

void vkDestroyShaderModule(VkDevice device, VkShaderModule shaderModule, VkAllocationCallbacks const * pAllocator)
{
    Functions::vkDestroyShaderModule(device, shaderModule, pAllocator);
}

void vkDestroyDescriptorPool(VkDevice device, VkDescriptorPool descriptorPool, VkAllocationCallbacks const * pAllocator)
{
    Functions::vkDestroyDescriptorPool(device, descriptorPool, pAllocator);
}

void vkDestroyDescriptorSetLayout(VkDevice device, VkDescriptorSetLayout descriptorSetLayout, VkAllocationCallbacks const * pAllocator)
{
    Functions::vkDestroyDescriptorSetLayout(device, descriptorSetLayout, pAllocator);
}

void vkDestroyPipelineLayout(VkDevice device, VkPipelineLayout pipelineLayout, VkAllocationCallbacks const * pAllocator)
{
    Functions::vkDestroyPipelineLayout(device, pipelineLayout, pAllocator);
}

void vkDestroyPipeline(VkDevice device, VkPipeline pipeline, VkAllocationCallbacks const * pAllocator)
{
    Functions::vkDestroyPipeline(device, pipeline, pAllocator);
}

void vkDestroyBuffer(VkDevice device, VkBuffer buffer, VkAllocationCallbacks const * pAllocator)
{
    Functions::vkDestroyBuffer(device, buffer, pAllocator);
}

void vkDestroyBufferView(VkDevice device, VkBufferView bufferView, VkAllocationCallbacks const * pAllocator)
{
    Functions::vkDestroyBufferView(device, bufferView, pAllocator);
}

void vkDestroySurfaceKHR(VkInstance instance, VkSurfaceKHR surface, VkAllocationCallbacks const * pAllocator)
{
    Functions::vkDestroySurfaceKHR(instance, surface, pAllocator);
}

void vkDestroySwapchainKHR(VkDevice device, VkSwapchainKHR swapchain, VkAllocationCallbacks const * pAllocator)
{
    Functions::vkDestroySwapchainKHR(device, swapchain, pAllocator);
}


VkResult vkBindBufferMemory(VkDevice device, VkBuffer buffer, VkDeviceMemory memory, VkDeviceSize memoryOffset)
{
    return Functions::vkBindBufferMemory(device, buffer, memory, memoryOffset);
}


VkResult vkMapMemory(VkDevice device, VkDeviceMemory memory, VkDeviceSize offset, VkDeviceSize size, VkMemoryMapFlags flags, void ** ppData)
{
    return Functions::vkMapMemory(device, memory, offset, size, flags, ppData);
}

void vkUnmapMemory(VkDevice device, VkDeviceMemory memory)
{
    Functions::vkUnmapMemory(device, memory);
}


void vkUpdateDescriptorSets(VkDevice device, std::uint32_t descriptorWriteCount, VkWriteDescriptorSet const * pDescriptorWrites, std::uint32_t descriptorCopyCount, VkCopyDescriptorSet const * pDescriptorCopies)
{
    Functions::vkUpdateDescriptorSets(device, descriptorWriteCount, pDescriptorWrites, descriptorCopyCount, pDescriptorCopies);
}


VkResult vkWaitForFences(VkDevice device, std::uint32_t fenceCount, VkFence const * pFences, VkBool32 waitAll, std::uint64_t timeout)
{
    return Functions::vkWaitForFences(device, fenceCount, pFences, waitAll, timeout);
}


VkResult vkResetFences(VkDevice device, std::uint32_t fenceCount, VkFence const * pFences)
{
    return Functions::vkResetFences(device, fenceCount, pFences);
}

VkResult vkResetCommandPool(VkDevice device, VkCommandPool commandPool, VkCommandPoolResetFlags flags)
{
    return Functions::vkResetCommandPool(device, commandPool, flags);
}

VkResult vkResetCommandBuffer(VkCommandBuffer commandBuffer, VkCommandBufferResetFlags flags)
{
    return Functions::vkResetCommandBuffer(commandBuffer, flags);
}


VkResult vkAllocateCommandBuffers(VkDevice device, VkCommandBufferAllocateInfo const * pAllocateInfo, VkCommandBuffer * pCommandBuffers)
{
    return Functions::vkAllocateCommandBuffers(device, pAllocateInfo, pCommandBuffers);
}

VkResult vkAllocateDescriptorSets(VkDevice device, VkDescriptorSetAllocateInfo const * pAllocateInfo, VkDescriptorSet * pDescriptorSets)
{
    return Functions::vkAllocateDescriptorSets(device, pAllocateInfo, pDescriptorSets);
}

VkResult vkAllocateMemory(VkDevice device, VkMemoryAllocateInfo const * pAllocateInfo, VkAllocationCallbacks const * pAllocator, VkDeviceMemory * pMemory)
{
    return Functions::vkAllocateMemory(device, pAllocateInfo, pAllocator, pMemory);
}


void vkFreeCommandBuffers(VkDevice device, VkCommandPool commandPool, std::uint32_t commandBufferCount, VkCommandBuffer * pCommandBuffers)
{
    return Functions::vkFreeCommandBuffers(device, commandPool, commandBufferCount, pCommandBuffers);
}

void vkFreeDescriptorSets(VkDevice device, VkDescriptorPool descriptorPool, std::uint32_t descriptorSetCount, VkDescriptorSet * pDescriptorSets)
{
    Functions::vkFreeDescriptorSets(device, descriptorPool, descriptorSetCount, pDescriptorSets);
}

void vkFreeMemory(VkDevice device, VkDeviceMemory memory, VkAllocationCallbacks const * pAllocator)
{
    Functions::vkFreeMemory(device, memory, pAllocator);
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


void vkUpdateDescriptorSets(VkDevice device,
                            std::uint32_t descriptorWriteCount, VkWriteDescriptorSet const* pDescriptorWrites,
                            std::uint32_t descriptorCopyCount, VkCopyDescriptorSet const* pDescriptorCopies)
{
    Functions::vkUpdateDescriptorSets(device, descriptorWriteCount, pDescriptorWrites, descriptorCopyCount, pDescriptorCopies);
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

void vkCmdBindPipeline(VkCommandBuffer commandBuffer, VkPipelineBindPoint pipelineBindPoint, VkPipeline pipeline)
{
    Functions::vkCmdBindPipeline(commandBuffer, pipelineBindPoint, pipeline);
}

void vkCmdBindDescriptorSets(VkCommandBuffer commandBuffer, 
                             VkPipelineBindPoint pipelineBindPoint, VkPipelineLayout layout, 
                             std::uint32_t firstSet, 
                             std::uint32_t descriptorSetCount, VkDescriptorSet const * pDescriptorSets, 
                             std::uint32_t dynamicOffsetCount, std::uint32_t const * pDynamicOffsets)
{
    Functions::vkCmdBindDescriptorSets(commandBuffer, pipelineBindPoint, layout, firstSet, descriptorSetCount, pDescriptorSets, dynamicOffsetCount, pDynamicOffsets);
}

void vkCmdDraw(VkCommandBuffer commandBuffer, std::uint32_t vertexCount, std::uint32_t instanceCount, std::uint32_t firstVertex, std::uint32_t firstInstance)
{
    Functions::vkCmdDraw(commandBuffer, vertexCount, instanceCount, firstVertex, firstInstance);
}

void vkCmdSetViewport(VkCommandBuffer commandBuffer, std::uint32_t firstViewport, std::uint32_t viewportCount, VkViewport * pViewports)
{
    Functions::vkCmdSetViewport(commandBuffer, firstViewport, viewportCount, pViewports);
}

void vkCmdSetScissor(VkCommandBuffer commandBuffer, std::uint32_t firstScissor, std::uint32_t scissorCount, VkRect2D * pScissors)
{
    Functions::vkCmdSetScissor(commandBuffer, firstScissor, scissorCount, pScissors);
}