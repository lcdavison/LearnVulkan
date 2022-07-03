#pragma once

#include "Graphics/VulkanModule.hpp"

namespace Vulkan::Device
{
    struct DeviceState;
}

/* Simple free list allocator for managing device memory */
namespace DeviceMemoryAllocator
{
    struct Allocation
    {
        VkDeviceMemory Memory;

        uint64 AllocationID; /* Pool / Block */
        uint64 SizeInBytes;
        uint64 OffsetInBytes;
        
        uint64 AlignmentWasteInBytes;

        void * MappedAddress; /* The memory will only be mapped if it's host visible */
    };

    extern bool const AllocateMemory(Vulkan::Device::DeviceState const & Device, VkMemoryRequirements const & MemoryRequirements, VkMemoryPropertyFlags const MemoryFlags, Allocation & OutputAllocation);

    extern bool const FreeMemory(Allocation & Allocation);

    extern void FreeAllDeviceMemory(Vulkan::Device::DeviceState const & Device);
}