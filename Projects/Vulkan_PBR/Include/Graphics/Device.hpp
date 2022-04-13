#include "VulkanModule.hpp"

#include "CommonTypes.hpp"

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
    };

    extern bool const CreateDevice(Vulkan::Instance::InstanceState const & InstanceState, DeviceState & OutputDeviceState);

    extern void DestroyDevice(DeviceState & State);

    extern void CreateCommandPool(DeviceState const & State, VkCommandPool & OutputCommandPool);

    extern void CreateCommandBuffer(DeviceState const & State, VkCommandBufferLevel CommandBufferLevel, VkCommandBuffer & OutputCommandBuffer);

    extern void CreateFrameBuffer(DeviceState const & State, uint32 Width, uint32 Height, VkRenderPass RenderPass, std::vector<VkImageView> const & Attachments, VkFramebuffer & OutputFrameBuffer);

    extern void CreateBuffer(DeviceState const & State, VkBuffer & OutputBuffer);
}