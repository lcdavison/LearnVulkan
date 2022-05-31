#include "VulkanModule.hpp"

#include "GPUResourceManager.hpp"

#include <vector>

/* TODO: Separate resource management, still use this for Create\Destroy Buffer */
namespace DeviceMemoryAllocator
{
    struct Allocation;
}

namespace Vulkan::Instance
{
    struct InstanceState;
}

namespace Vulkan::Device
{
    struct DeviceState
    {
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

    extern void CreateBuffer(DeviceState const & State, uint64 SizeInBytes, VkBufferUsageFlags UsageFlags, VkMemoryPropertyFlags MemoryFlags, GPUResourceManager::BufferHandle & OutputBuffer);

    extern void DestroyBuffer(DeviceState const & State, GPUResourceManager::BufferHandle const BufferHandle, VkFence const FenceToWaitFor);
}