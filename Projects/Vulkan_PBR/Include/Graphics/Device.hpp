#include "VulkanModule.hpp"

#include <vector>

namespace Vulkan::Instance
{
    struct InstanceState;
}

namespace Vulkan
{
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
        bool bReverseComponents = {};
    };
}

namespace Vulkan::Resource
{
    struct BufferDescriptor
    {
        uint64 SizeInBytes = {};
        VkBufferCreateFlags CreateFlags = {};
        VkBufferUsageFlags UsageFlags = {};
        VkMemoryPropertyFlags MemoryFlags = {};
    };

    struct FrameBufferDescriptor
    {
        VkRenderPass RenderPass = {};
        VkImageView * Attachments = {};
        uint32 AttachmentCount = {};
        uint32 Width = {};
        uint32 Height = {};
    };

    struct Buffer
    {
        VkDeviceSize SizeInBytes = {};
        uint64 MemoryAllocationHandle = {};
        VkBuffer Resource = {};
    };

    struct Image
    {
        uint64 MemoryAllocationHandle = {};
        VkImage Resource = {};
        uint32 Width = {};
        uint32 Height = {};
    };

    extern bool const GetBuffer(uint32 const kBufferHandle, Vulkan::Resource::Buffer & OutputBuffer);

    extern bool const GetImage(uint32 const kImageHandle, Vulkan::Resource::Image & OutputImage);

    extern bool const GetImageView(uint32 const kImageViewHandle, VkImageView & OutputImageView);

    extern bool const GetFrameBuffer(uint32 const kFrameBufferHandle, VkFramebuffer & OutputFrameBuffer);
}

namespace Vulkan::Device
{
    struct DeviceState
    {
        VkPhysicalDeviceFeatures PhysicalDeviceFeatures = {};
        VkPhysicalDevice PhysicalDevice;
        VkDevice Device;

        VkQueue GraphicsQueue;
        uint32 GraphicsQueueFamilyIndex;

        /* Can put this here to begin with, but for multithreading each thread should use its own Pool*/
        VkCommandPool CommandPool;
    };

    extern bool const CreateDevice(Vulkan::Instance::InstanceState const & kInstanceState, DeviceState & OutputDeviceState);

    extern void DestroyDevice(DeviceState & kDeviceState);

    extern void CreateCommandBuffers(DeviceState const & kDeviceState, VkCommandBufferLevel const kCommandBufferLevel, uint32 const kCommandBufferCount, std::vector<VkCommandBuffer> & OutputCommandBuffers);

    extern void DestroyCommandBuffers(DeviceState const & kDeviceState, std::vector<VkCommandBuffer> & CommandBuffers);

    extern void CreateSemaphore(DeviceState const & kDeviceState, VkSemaphoreCreateFlags const kFlags, VkSemaphore & OutputSemaphore);

    extern void DestroySemaphore(DeviceState const & kDeviceState, VkSemaphore & Semaphore);

    extern void CreateFence(DeviceState const & kDeviceState, VkFenceCreateFlags const kFlags, VkFence & OutputFence);

    extern void DestroyFence(DeviceState const & kDeviceState, VkFence & Fence);

    extern void CreateEvent(DeviceState const & kDeviceState, VkEventCreateFlags const kFlags, VkEvent & OutputEvent);

    extern void DestroyEvent(DeviceState const & kDeviceState, VkEvent & Event);

    extern void CreateFrameBuffer(DeviceState const & kDeviceState, Vulkan::Resource::FrameBufferDescriptor const & kDescriptor, uint32 & OutputFrameBufferHandle);

    extern void DestroyFrameBuffer(DeviceState const & kDeviceState, uint32 const kFrameBufferHandle, VkFence const FenceToWaitFor);

    extern void CreateBuffer(DeviceState const & kDeviceState, uint64 const kSizeInBytes, VkBufferUsageFlags const kUsageFlags, VkMemoryPropertyFlags const kMemoryFlags, uint32 & OutputBufferHandle);

    extern void CreateBuffer(DeviceState const & kDeviceState, Vulkan::Resource::BufferDescriptor const & kDescriptor, uint32 & OutputBufferHandle);

    extern void DestroyBuffer(DeviceState const & kDeviceState, uint32 const kBufferHandle, VkFence const kFenceToWaitFor);

    extern void CreateImage(DeviceState const & kDeviceState, Vulkan::ImageDescriptor const & kDescriptor, VkMemoryPropertyFlags const kMemoryFlags, uint32 & OutputImageHandle);

    extern void DestroyImage(DeviceState const & kDeviceState, uint32 const kImageHandle, VkFence const kFenceToWaitFor);

    extern void CreateImageView(DeviceState const & kDeviceState, uint32 const kImageHandle, Vulkan::ImageViewDescriptor const & kDescriptor, uint32 & OutputImageViewHandle);

    extern void DestroyImageView(DeviceState const & kDeviceState, uint32 const kImageViewHandle, VkFence const kFenceToWaitFor);

    extern void DestroyUnusedResources(DeviceState const & kDeviceState);
}