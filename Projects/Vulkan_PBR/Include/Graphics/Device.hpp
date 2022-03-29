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

        VkSemaphore PresentSemaphore; // Signalled when presentation engine is finished with image
        VkSemaphore RenderSemaphore; // Signalled when rendering is complete
    };

    extern bool const CreateDevice(Vulkan::Instance::InstanceState const & InstanceState, DeviceState & OutputDeviceState);

    extern void DestroyDevice(DeviceState & State);

    extern void CreateCommandPool(DeviceState const & State, VkCommandPool & OutputCommandPool);

    extern void CreateCommandBuffer(DeviceState const & State, VkCommandBufferLevel CommandBufferLevel, VkCommandBuffer & OutputCommandBuffer);

    extern void CreateBuffer(DeviceState const & State, VkBuffer & OutputBuffer);
}