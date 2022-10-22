#pragma once

#include "Graphics/VulkanModule.hpp"

namespace Vulkan::Device
{
    struct DeviceState;
}

namespace Vulkan::Memory
{
    struct AllocationInfo
    {
        VkDeviceMemory DeviceMemory = {};

        uint64 OffsetInBytes = {};
        uint64 SizeInBytes = {};

        void * MappedAddress = {};
    };

    extern bool const Allocate(Vulkan::Device::DeviceState const & kDevice, VkMemoryRequirements const & kMemoryRequirements, VkMemoryPropertyFlags const kMemoryFlags, uint64 & OutputAllocationHandle);

    extern bool const Free(uint64 const kAllocationHandle);

    extern bool const GetAllocationInfo(uint64 const kAllocationHandle, AllocationInfo & OutputAllocationInfo);
}