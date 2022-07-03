#include "VulkanModule.hpp"

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

namespace Vulkan
{
    struct BufferDescriptor
    {
        uint64 SizeInBytes = {};
        VkBufferUsageFlags UsageFlags = {};
        VkBufferCreateFlags Flags = {};
    };

    struct ImageDescriptor
    {
        VkImageType ImageType = {};
        VkFormat Format = {};
        uint32 Width = {};
        uint32 Height = {};
        uint32 Depth = {};
        uint32 ArrayLayerCount = {};
        uint32 MipLevelCount = {};
        VkSampleCountFlags SampleCount = {};
        VkImageTiling Tiling = {};
        VkImageUsageFlags UsageFlags = {};
        VkImageCreateFlags Flags = {};
        bool bIsCPUWritable = {};
    };

    struct ImageViewDescriptor
    {
        VkImageViewType ViewType = {};
        VkFormat Format = {};
        VkImageAspectFlags AspectFlags = {};
        uint32 FirstArrayLayerIndex = {};
        uint32 ArrayLayerCount = {};
        uint32 FirstMipLevelIndex = {};
        uint32 MipLevelCount = {};
    };
}

namespace Vulkan::Resource
{
    struct Buffer
    {
        DeviceMemoryAllocator::Allocation const * MemoryAllocation = {};
        VkBuffer Resource = {};
        VkDeviceSize SizeInBytes = {};
    };

    struct Image
    {
        DeviceMemoryAllocator::Allocation const * MemoryAllocation = {};
        VkImage Resource = {};
        uint32 Width = {};
        uint32 Height = {};
    };

    extern bool const GetBuffer(uint32 const BufferIndex, Vulkan::Resource::Buffer & OutputBuffer);

    extern bool const GetImage(uint32 const ImageHandle, Vulkan::Resource::Image & OutputImage);

    extern bool const GetImageView(uint32 const ImageViewHandle, VkImageView & OutputImageView);

    extern bool const GetFrameBuffer(uint32 const FrameBufferHandle, VkFramebuffer & OutputFrameBuffer);
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

    extern void CreateEvent(DeviceState const & State, VkEventCreateFlags Flags, VkEvent & OutputEvent);

    extern void DestroyEvent(DeviceState const & State, VkEvent & Event);

    extern void CreateFrameBuffer(DeviceState const & State, uint32 const Width, uint32 const Height, VkRenderPass const RenderPass, std::vector<VkImageView> const & Attachments, uint32 & OutputFrameBufferHandle);

    extern void DestroyFrameBuffer(DeviceState const & State, uint32 const FrameBufferHandle, VkFence const FenceToWaitFor);

    extern void CreateBuffer(DeviceState const & State, uint64 const SizeInBytes, VkBufferUsageFlags UsageFlags, VkMemoryPropertyFlags MemoryFlags, uint32 & OutputBufferHandle);

    extern void DestroyBuffer(DeviceState const & State, uint32 const BufferHandle, VkFence const FenceToWaitFor);

    extern void CreateImage(DeviceState const & State, Vulkan::ImageDescriptor const & Descriptor, VkMemoryPropertyFlags const MemoryFlags, uint32 & OutputImageHandle);

    extern void DestroyImage(DeviceState const & State, uint32 const ImageHandle, VkFence const FenceToWaitFor);

    extern void CreateImageView(DeviceState const & State, uint32 const ImageHandle, Vulkan::ImageViewDescriptor const & Descriptor, uint32 & OutputImageViewHandle);

    extern void DestroyImageView(DeviceState const & State, uint32 const ImageViewHandle, VkFence const FenceToWaitFor);

    extern void DestroyUnusedResources(DeviceState const & State);
}