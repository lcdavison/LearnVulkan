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
        uint64 AllocationID; /* Pool / Block */
        uint64 SizeInBytes;
        uint64 OffsetInBytes;
        
        uint64 AlignmentWasteInBytes;
    };

    extern bool const AllocateMemory(Vulkan::Device::DeviceState const & Device, VkMemoryRequirements const & MemoryRequirements, VkMemoryPropertyFlags const MemoryFlags, Allocation & OutputAllocation);

    extern bool const FreeMemory(Allocation & Allocation, Vulkan::Device::DeviceState const & Device);
}

/* Creates a buffer for suballocation */
namespace LinearBufferAllocator
{
    struct Allocation
    {
        VkBuffer Buffer;
        uint64 OffsetInBytes;
        uint64 SizeInBytes;
    };

    struct AllocatorState
    {
        DeviceMemoryAllocator::Allocation DeviceMemory;

        VkBuffer Buffer;
        uint64 CurrentOffsetInBytes;
        uint64 CapacityInBytes;
    };

    extern bool const CreateAllocator(Vulkan::Device::DeviceState const & DeviceState, uint64 const BufferSizeInBytes, VkBufferUsageFlags const UsageFlags, VkMemoryPropertyFlags const MemoryFlags, AllocatorState & OutputState);

    extern bool const DestroyAllocator(AllocatorState & State, Vulkan::Device::DeviceState & DeviceState);

    extern bool const Allocate(AllocatorState & State, Vulkan::Device::DeviceState const & DeviceState, uint64 const BufferSizeInBytes, Allocation & OutputAllocation);

    extern bool const Reset(AllocatorState & State);
}