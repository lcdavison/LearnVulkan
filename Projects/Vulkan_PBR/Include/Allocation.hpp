#pragma once

#include "Graphics/VulkanModule.hpp"

namespace Vulkan::Device
{
    struct DeviceState;
}

/* Creates a buffer for suballocation */
/* These are ideal for allocating small buffers for single use in a frame */
namespace LinearBufferAllocator
{
    struct Allocation
    {
        void * MappedAddress;
        uint64 OffsetInBytes;
        uint64 SizeInBytes;

        uint32 Buffer;
    };

    struct AllocatorState
    {
        uint64 CurrentOffsetInBytes;
        uint64 CurrentSizeInBytes;
        uint64 CapacityInBytes;

        uint32 Buffer;
    };

    extern bool const CreateAllocator(Vulkan::Device::DeviceState const & DeviceState, uint64 const BufferSizeInBytes, VkBufferUsageFlags const UsageFlags, VkMemoryPropertyFlags const MemoryFlags, AllocatorState & OutputState);

    extern bool const DestroyAllocator(AllocatorState & State, Vulkan::Device::DeviceState & DeviceState);

    extern bool const Allocate(AllocatorState & State, uint64 const BufferSizeInBytes, Allocation & OutputAllocation);

    extern void Reset(AllocatorState & State);
}