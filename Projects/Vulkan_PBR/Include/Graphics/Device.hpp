#include "Vulkan.hpp"

namespace Vulkan::Device
{
    struct DeviceState
    {
        VkPhysicalDevice PhysicalDevice;
        VkDevice Device;
        uint32 GraphicsQueueFamilyIndex;
    };

    extern bool const CreateDevice(VkInstance Instance, DeviceState & OutputDeviceState);

    extern void DestroyDevice(DeviceState & State);
}