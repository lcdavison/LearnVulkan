#include "VulkanModule.hpp"

#include <vector>

namespace Vulkan::Instance
{
    struct InstanceState;
}

namespace Vulkan::Device
{
    struct DeviceState
    {
        std::vector<VkDeviceMemory> MemoryAllocations;
        std::vector<uint64> MemoryAllocationOffsetsInBytes;

        /* Bit == 1 -> Allocated | Bit == 0 -> Free */
        std::vector<uint8> MemoryAllocationStatusMasks;

        VkPhysicalDeviceMemoryProperties MemoryProperties;

        VkPhysicalDevice PhysicalDevice;
        VkDevice Device;

        VkQueue GraphicsQueue;
        uint32 GraphicsQueueFamilyIndex;

        /* Can put this here to begin with, but for multithreading each thread should use its own Pool*/
        VkCommandPool CommandPool;
        VkDescriptorPool DescriptorPool;
    };

    extern bool const CreateDevice(Vulkan::Instance::InstanceState const & InstanceState, DeviceState & OutputDeviceState);

    extern void DestroyDevice(DeviceState & State);

    extern void CreateCommandBuffers(DeviceState const & State, VkCommandBufferLevel CommandBufferLevel, uint32 const CommandBufferCount, std::vector<VkCommandBuffer> & OutputCommandBuffers);

    extern void DestroyCommandBuffers(DeviceState const & State, std::vector<VkCommandBuffer> & CommandBuffers);
    
    extern void CreateSemaphore(DeviceState const & State, VkSemaphoreCreateFlags Flags, VkSemaphore & OutputSemaphore);

    extern void DestroySemaphore(DeviceState const & State, VkSemaphore & Semaphore);

    extern void CreateFence(DeviceState const & State, VkFenceCreateFlags Flags, VkFence & OutputFence);

    extern void DestroyFence(DeviceState const & State, VkFence & Fence);

    extern void CreateFrameBuffer(DeviceState const & State, uint32 Width, uint32 Height, VkRenderPass RenderPass, std::vector<VkImageView> const & Attachments, VkFramebuffer & OutputFrameBuffer);

    extern void DestroyFrameBuffer(DeviceState const & State, VkFramebuffer & FrameBuffer);

    extern void CreateBuffer(DeviceState & State, uint64 SizeInBytes, VkBufferUsageFlags UsageFlags, VkMemoryPropertyFlags MemoryFlags, VkBuffer & OutputBuffer, VkDeviceMemory & OutputDeviceMemory);

    extern void DestroyBuffer(DeviceState const & State, VkBuffer & Buffer);
}