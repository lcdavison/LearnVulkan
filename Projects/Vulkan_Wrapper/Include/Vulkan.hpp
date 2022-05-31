#pragma once

#include <vulkan/vulkan_core.h>
#include <cstdint>

#ifdef VULKAN_WRAPPER_EXPORT
    #define VULKAN_WRAPPER_API __declspec(dllexport)
#else
    #define VULKAN_WRAPPER_API __declspec(dllimport)
#endif

namespace Vulkan
{
    inline VkBufferMemoryBarrier BufferMemoryBarrier(VkBuffer Buffer, VkAccessFlags SourceAccessFlags, VkAccessFlags DestinationAccessFlags, VkDeviceSize const SizeInBytes = VK_WHOLE_SIZE, VkDeviceSize const OffsetInBytes = 0u)
    {
        return VkBufferMemoryBarrier { VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER, nullptr, SourceAccessFlags, DestinationAccessFlags, 0u, 0u, Buffer, OffsetInBytes, SizeInBytes };
    }

    inline constexpr VkVertexInputBindingDescription VertexInputBinding(std::uint32_t const BindingSlotIndex, std::uint32_t const StrideInBytes, VkVertexInputRate InputRate = VK_VERTEX_INPUT_RATE_VERTEX)
    {
        return VkVertexInputBindingDescription { BindingSlotIndex, StrideInBytes, InputRate };
    }

    inline constexpr VkVertexInputAttributeDescription VertexInputAttribute(std::uint32_t const Location, std::uint32_t const BindingSlotIndex, std::uint32_t const OffsetInBytes, VkFormat const Format = VK_FORMAT_R32G32B32_SFLOAT)
    {
        return VkVertexInputAttributeDescription { Location, BindingSlotIndex, Format, OffsetInBytes };
    }

    inline constexpr VkPipelineVertexInputStateCreateInfo VertexInputState(std::uint32_t const VertexBindingCount, VkVertexInputBindingDescription * const VertexBindings, 
                                                                           std::uint32_t const VertexAttributeCount, VkVertexInputAttributeDescription * const VertexAttributes, 
                                                                           VkPipelineVertexInputStateCreateFlags const Flags = 0u)
    {
        return VkPipelineVertexInputStateCreateInfo { VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO, nullptr, Flags, VertexBindingCount, VertexBindings, VertexAttributeCount, VertexAttributes };
    }

    inline constexpr VkPipelineInputAssemblyStateCreateInfo InputAssemblyState(VkPrimitiveTopology const PrimitiveTopology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, VkBool32 const PrimitiveRestartEnabled = VK_FALSE, VkPipelineInputAssemblyStateCreateFlags const Flags = 0u)
    {
        return VkPipelineInputAssemblyStateCreateInfo { VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO, nullptr, Flags, PrimitiveTopology, PrimitiveRestartEnabled };
    }

    inline constexpr VkPipelineRasterizationStateCreateInfo RasterizationState(/*VkCullModeFlags const CullMode, VkPolygonMode PolygonMode = VK_POLYGON_MODE_FILL,*/ VkPipelineRasterizationStateCreateFlags const Flags = 0u)
    {
        return VkPipelineRasterizationStateCreateInfo { VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO, nullptr, Flags };
    }
}

VULKAN_WRAPPER_API bool const InitialiseVulkanWrapper();
VULKAN_WRAPPER_API bool const ShutdownVulkanWrapper();

VULKAN_WRAPPER_API bool const LoadExportedFunctions();
VULKAN_WRAPPER_API bool const LoadGlobalFunctions();
VULKAN_WRAPPER_API bool const LoadInstanceFunctions(VkInstance Instance);
VULKAN_WRAPPER_API bool const LoadDeviceFunctions(VkDevice Device);
VULKAN_WRAPPER_API bool const LoadInstanceExtensionFunctions(VkInstance Instance, std::uint32_t const ExtensionNameCount, char const * const * ExtensionNames);
VULKAN_WRAPPER_API bool const LoadDeviceExtensionFunctions(VkDevice Device, std::uint32_t const ExtensionNameCount, char const * const * ExtensionNames);

VULKAN_WRAPPER_API VkResult vkEnumerateInstanceExtensionProperties(char const * pLayerName, std::uint32_t * pPropertyCount, VkExtensionProperties * pProperties);
VULKAN_WRAPPER_API VkResult vkEnumerateInstanceLayerProperties(std::uint32_t * pPropertyCount, VkLayerProperties * pProperties);
VULKAN_WRAPPER_API VkResult vkEnumeratePhysicalDevices(VkInstance instance, std::uint32_t * pPhysicalDeviceCount, VkPhysicalDevice * pPhysicalDevice);
VULKAN_WRAPPER_API VkResult vkEnumerateDeviceExtensionProperties(VkPhysicalDevice device, char const * pLayerName, std::uint32_t * pPropertyCount, VkExtensionProperties * pProperties);
VULKAN_WRAPPER_API VkResult vkEnumerateDeviceLayerProperties(VkPhysicalDevice device, std::uint32_t * pPropertyCount, VkLayerProperties * pProperties);

VULKAN_WRAPPER_API void vkGetPhysicalDeviceProperties(VkPhysicalDevice physicalDevice, VkPhysicalDeviceProperties * pProperties);
VULKAN_WRAPPER_API void vkGetPhysicalDeviceFeatures(VkPhysicalDevice physicalDevice, VkPhysicalDeviceFeatures * pFeatures);
VULKAN_WRAPPER_API void vkGetPhysicalDeviceMemoryProperties(VkPhysicalDevice physicalDevice, VkPhysicalDeviceMemoryProperties * pMemoryProperties);
VULKAN_WRAPPER_API void vkGetPhysicalDeviceQueueFamilyProperties(VkPhysicalDevice physicalDevice, std::uint32_t * pQueueFamilyPropertyCount, VkQueueFamilyProperties * pQueueFamilyProperties);
VULKAN_WRAPPER_API VkResult vkGetPhysicalDeviceSurfaceSupportKHR(VkPhysicalDevice physicalDevice, std::uint32_t queueFamilyIndex, VkSurfaceKHR surface, VkBool32 * pSupported);
VULKAN_WRAPPER_API VkResult vkGetPhysicalDeviceSurfaceFormatsKHR(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, std::uint32_t * pFormatCount, VkSurfaceFormatKHR * pSurfaceFormat);
VULKAN_WRAPPER_API VkResult vkGetPhysicalDeviceSurfaceCapabilitiesKHR(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, VkSurfaceCapabilitiesKHR * pSurfaceCapabilities);
VULKAN_WRAPPER_API void vkGetDeviceQueue(VkDevice device, std::uint32_t queueFamilyIndex, std::uint32_t queueIndex, VkQueue * pQueue);
VULKAN_WRAPPER_API void vkGetBufferMemoryRequirements(VkDevice device, VkBuffer buffer, VkMemoryRequirements * pMemoryRequirements);
VULKAN_WRAPPER_API VkResult vkGetFenceStatus(VkDevice device, VkFence fence);
VULKAN_WRAPPER_API VkResult vkGetEventStatus(VkDevice device, VkEvent event);
VULKAN_WRAPPER_API VkResult vkGetSwapchainImagesKHR(VkDevice device, VkSwapchainKHR swapchain, std::uint32_t * pSwapchainImageCount, VkImage * pSwapchainImages);

VULKAN_WRAPPER_API VkResult vkCreateInstance(VkInstanceCreateInfo const * pCreateInfo, VkAllocationCallbacks const * pAllocator, VkInstance * pInstance);
VULKAN_WRAPPER_API VkResult vkCreateDevice(VkPhysicalDevice physicalDevice, VkDeviceCreateInfo const * pCreateInfo, VkAllocationCallbacks const * pAllocator, VkDevice * pDevice);
VULKAN_WRAPPER_API VkResult vkCreateCommandPool(VkDevice device, VkCommandPoolCreateInfo const * pCreateInfo, VkAllocationCallbacks const * pAllocator, VkCommandPool * pCommandPool);
VULKAN_WRAPPER_API VkResult vkCreateSemaphore(VkDevice device, VkSemaphoreCreateInfo const * pCreateInfo, VkAllocationCallbacks const * pAllocator, VkSemaphore * pSemaphore);
VULKAN_WRAPPER_API VkResult vkCreateFence(VkDevice device, VkFenceCreateInfo const * pCreateInfo, VkAllocationCallbacks const * pAllocator, VkFence * pFence);
VULKAN_WRAPPER_API VkResult vkCreateEvent(VkDevice device, VkEventCreateInfo const * pCreateInfo, VkAllocationCallbacks const * pAllocator, VkEvent * pEvent);
VULKAN_WRAPPER_API VkResult vkCreateRenderPass(VkDevice device, VkRenderPassCreateInfo const * pCreateInfo, VkAllocationCallbacks const * pAllocator, VkRenderPass * pRenderPass);
VULKAN_WRAPPER_API VkResult vkCreateFramebuffer(VkDevice device, VkFramebufferCreateInfo const * pCreateInfo, VkAllocationCallbacks const * pAllocator, VkFramebuffer * pFramebuffer);
VULKAN_WRAPPER_API VkResult vkCreateImageView(VkDevice device, VkImageViewCreateInfo const * pCreateInfo, VkAllocationCallbacks const * pAllocator, VkImageView * pImageView);
VULKAN_WRAPPER_API VkResult vkCreateShaderModule(VkDevice device, VkShaderModuleCreateInfo const * pCreateInfo, VkAllocationCallbacks const * pAllocator, VkShaderModule * pShaderModule);
VULKAN_WRAPPER_API VkResult vkCreateDescriptorPool(VkDevice device, VkDescriptorPoolCreateInfo const * pCreateInfo, VkAllocationCallbacks const * pAllocator, VkDescriptorPool * pDescriptorPool);
VULKAN_WRAPPER_API VkResult vkCreateDescriptorSetLayout(VkDevice device, VkDescriptorSetLayoutCreateInfo const * pCreateInfo, VkAllocationCallbacks const * pAllocator, VkDescriptorSetLayout * pSetLayout);
VULKAN_WRAPPER_API VkResult vkCreatePipelineLayout(VkDevice device, VkPipelineLayoutCreateInfo const * pCreateInfo, VkAllocationCallbacks const * pAllocator, VkPipelineLayout * pPipelineLayout);
VULKAN_WRAPPER_API VkResult vkCreateGraphicsPipelines(VkDevice device, VkPipelineCache pipelineCache, std::uint32_t createInfoCount, VkGraphicsPipelineCreateInfo const * pCreateInfos, VkAllocationCallbacks const * pAllocator, VkPipeline * pPipelines);
VULKAN_WRAPPER_API VkResult vkCreateBuffer(VkDevice device, VkBufferCreateInfo const * pCreateInfo, VkAllocationCallbacks const * pAllocator, VkBuffer * pBuffer);
VULKAN_WRAPPER_API VkResult vkCreateBufferView(VkDevice device, VkBufferViewCreateInfo const * pCreateInfo, VkAllocationCallbacks const * pAllocator, VkBufferView * pView);
VULKAN_WRAPPER_API VkResult vkCreateSwapchainKHR(VkDevice device, VkSwapchainCreateInfoKHR const * pCreateInfo, VkAllocationCallbacks const * pAllocator, VkSwapchainKHR * pSwapchain);

VULKAN_WRAPPER_API void vkDestroyInstance(VkInstance instance, VkAllocationCallbacks const * pAllocator);
VULKAN_WRAPPER_API void vkDestroyDevice(VkDevice device, VkAllocationCallbacks const * pAllocator);
VULKAN_WRAPPER_API void vkDestroyCommandPool(VkDevice device, VkCommandPool commandPool, VkAllocationCallbacks const * pAllocator);
VULKAN_WRAPPER_API void vkDestroySemaphore(VkDevice device, VkSemaphore semaphore, VkAllocationCallbacks const * pAllocator);
VULKAN_WRAPPER_API void vkDestroyFence(VkDevice device, VkFence fence, VkAllocationCallbacks const * pAllocator);
VULKAN_WRAPPER_API void vkDestroyEvent(VkDevice device, VkEvent event, VkAllocationCallbacks const * pAllocator);
VULKAN_WRAPPER_API void vkDestroyRenderPass(VkDevice device, VkRenderPass renderPass, VkAllocationCallbacks const * pAllocator);
VULKAN_WRAPPER_API void vkDestroyFramebuffer(VkDevice device, VkFramebuffer framebuffer, VkAllocationCallbacks const * pAllocator);
VULKAN_WRAPPER_API void vkDestroyImageView(VkDevice device, VkImageView imageView, VkAllocationCallbacks const * pAllocator);
VULKAN_WRAPPER_API void vkDestroyShaderModule(VkDevice device, VkShaderModule shaderModule, VkAllocationCallbacks const * pAllocator);
VULKAN_WRAPPER_API void vkDestroyDescriptorPool(VkDevice device, VkDescriptorPool descriptorPool, VkAllocationCallbacks const * pAllocator);
VULKAN_WRAPPER_API void vkDestroyDescriptorSetLayout(VkDevice device, VkDescriptorSetLayout descriptorSetLayout, VkAllocationCallbacks const * pAllocator);
VULKAN_WRAPPER_API void vkDestroyPipelineLayout(VkDevice device, VkPipelineLayout pipelineLayout, VkAllocationCallbacks const * pAllocator);
VULKAN_WRAPPER_API void vkDestroyPipeline(VkDevice device, VkPipeline pipeline, VkAllocationCallbacks const * pAllocator);
VULKAN_WRAPPER_API void vkDestroyBuffer(VkDevice device, VkBuffer buffer, VkAllocationCallbacks const * pAllocator);
VULKAN_WRAPPER_API void vkDestroyBufferView(VkDevice device, VkBufferView bufferView, VkAllocationCallbacks const * pAllocator);
VULKAN_WRAPPER_API void vkDestroySurfaceKHR(VkInstance instance, VkSurfaceKHR surface, VkAllocationCallbacks const * pAllocator);
VULKAN_WRAPPER_API void vkDestroySwapchainKHR(VkDevice device, VkSwapchainKHR swapchain, VkAllocationCallbacks const * pAllocator);

VULKAN_WRAPPER_API VkResult vkBindBufferMemory(VkDevice device, VkBuffer buffer, VkDeviceMemory memory, VkDeviceSize memoryOffset);

VULKAN_WRAPPER_API VkResult vkMapMemory(VkDevice device, VkDeviceMemory memory, VkDeviceSize offset, VkDeviceSize size, VkMemoryMapFlags flags, void ** ppData);
VULKAN_WRAPPER_API void vkUnmapMemory(VkDevice device, VkDeviceMemory memory);

VULKAN_WRAPPER_API VkResult vkWaitForFences(VkDevice device, std::uint32_t fenceCount, VkFence const * pFences, VkBool32 waitAll, std::uint64_t timeout);

VULKAN_WRAPPER_API VkResult vkResetFences(VkDevice device, std::uint32_t fenceCount, VkFence const * pFences);
VULKAN_WRAPPER_API VkResult vkResetCommandPool(VkDevice device, VkCommandPool commandPool, VkCommandPoolResetFlags flags);
VULKAN_WRAPPER_API VkResult vkResetCommandBuffer(VkCommandBuffer commandBuffer, VkCommandBufferResetFlags flags);

VULKAN_WRAPPER_API VkResult vkAllocateCommandBuffers(VkDevice device, VkCommandBufferAllocateInfo const * pAllocateInfo, VkCommandBuffer * pCommandBuffers);
VULKAN_WRAPPER_API VkResult vkAllocateDescriptorSets(VkDevice device, VkDescriptorSetAllocateInfo const * pAllocateInfo, VkDescriptorSet * pDescriptorSets);
VULKAN_WRAPPER_API VkResult vkAllocateMemory(VkDevice device, VkMemoryAllocateInfo const * pAllocateInfo, VkAllocationCallbacks const * pAllocator, VkDeviceMemory * pMemory);

VULKAN_WRAPPER_API void vkFreeCommandBuffers(VkDevice device, VkCommandPool commandPool, std::uint32_t commandBufferCount, VkCommandBuffer * pCommandBuffers);
VULKAN_WRAPPER_API void vkFreeDescriptorSets(VkDevice device, VkDescriptorPool descriptorPool, std::uint32_t descriptorSetCount, VkDescriptorSet * pDescriptorSets);
VULKAN_WRAPPER_API void vkFreeMemory(VkDevice device, VkDeviceMemory memory, VkAllocationCallbacks const * pAllocator);

VULKAN_WRAPPER_API VkResult vkBeginCommandBuffer(VkCommandBuffer commandBuffer, VkCommandBufferBeginInfo const * pBeginInfo);

VULKAN_WRAPPER_API VkResult vkEndCommandBuffer(VkCommandBuffer commandBuffer);

VULKAN_WRAPPER_API VkResult vkDeviceWaitIdle(VkDevice device);

VULKAN_WRAPPER_API void vkUpdateDescriptorSets(VkDevice device,
                                               std::uint32_t descriptorWriteCount, VkWriteDescriptorSet const* pDescriptorWrites,
                                               std::uint32_t descriptorCopyCount, VkCopyDescriptorSet const* pDescriptorCopies);

VULKAN_WRAPPER_API VkResult vkQueueSubmit(VkQueue queue, std::uint32_t submitCount, VkSubmitInfo const * pSubmits, VkFence fence);
VULKAN_WRAPPER_API VkResult vkQueuePresentKHR(VkQueue queue, VkPresentInfoKHR const * pPresentInfo);

VULKAN_WRAPPER_API VkResult vkAcquireNextImageKHR(VkDevice device, VkSwapchainKHR swapchain, std::uint64_t timeout, VkSemaphore semaphore, VkFence fence, std::uint32_t * pImageIndex);

VULKAN_WRAPPER_API void vkCmdClearColorImage(VkCommandBuffer commandBuffer, VkImage image, VkImageLayout imageLayout, VkClearColorValue const * pColor, std::uint32_t rangeCount, VkImageSubresourceRange const * pRanges);

VULKAN_WRAPPER_API void vkCmdPipelineBarrier(VkCommandBuffer commandBuffer,
                                             VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask,
                                             VkDependencyFlags dependencyFlags,
                                             std::uint32_t memoryBarrierCount, VkMemoryBarrier const * pMemoryBarriers,
                                             std::uint32_t bufferMemoryBarrierCount, VkBufferMemoryBarrier const * pBufferMemoryBarriers,
                                             std::uint32_t imageMemoryBarrierCount, VkImageMemoryBarrier const * pImageMemoryBarriers);

VULKAN_WRAPPER_API void vkCmdBeginRenderPass(VkCommandBuffer commandBuffer, VkRenderPassBeginInfo const * pRenderPassBegin, VkSubpassContents contents);
VULKAN_WRAPPER_API void vkCmdEndRenderPass(VkCommandBuffer commandBuffer);

VULKAN_WRAPPER_API void vkCmdBindPipeline(VkCommandBuffer commandBuffer, VkPipelineBindPoint pipelineBindPoint, VkPipeline pipeline);
VULKAN_WRAPPER_API void vkCmdBindDescriptorSets(VkCommandBuffer commandBuffer, 
                                                VkPipelineBindPoint pipelineBindPoint, VkPipelineLayout layout, 
                                                std::uint32_t firstSet, 
                                                std::uint32_t descriptorSetCount, VkDescriptorSet const * pDescriptorSets, 
                                                std::uint32_t dynamicOffsetCount, std::uint32_t const * pDynamicOffsets);
VULKAN_WRAPPER_API void vkCmdBindVertexBuffers(VkCommandBuffer commandBuffer, std::uint32_t firstBinding, std::uint32_t bindingCount, VkBuffer const * pBuffers, VkDeviceSize const * pOffsets);
VULKAN_WRAPPER_API void vkCmdBindIndexBuffer(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset, VkIndexType indexType);

VULKAN_WRAPPER_API void vkCmdDraw(VkCommandBuffer commandBuffer, std::uint32_t vertexCount, std::uint32_t instanceCount, std::uint32_t firstVertex, std::uint32_t firstInstance);
VULKAN_WRAPPER_API void vkCmdDrawIndexed(VkCommandBuffer commandBuffer, std::uint32_t indexCount, std::uint32_t instanceCount, std::uint32_t firstIndex, std::int32_t vertexOffset, std::uint32_t firstInstance);

VULKAN_WRAPPER_API void vkCmdSetViewport(VkCommandBuffer commandBuffer, std::uint32_t firstViewport, std::uint32_t viewportCount, VkViewport * pViewports);
VULKAN_WRAPPER_API void vkCmdSetScissor(VkCommandBuffer commandBuffer, std::uint32_t firstScissor, std::uint32_t scissorCount, VkRect2D * pScissors);
VULKAN_WRAPPER_API void vkCmdSetEvent(VkCommandBuffer commandBuffer, VkEvent event, VkPipelineStageFlags stageMask);

VULKAN_WRAPPER_API void vkCmdCopyBuffer(VkCommandBuffer commandBuffer, VkBuffer srcBuffer, VkBuffer dstBuffer, std::uint32_t regionCount, VkBufferCopy const * pRegions);