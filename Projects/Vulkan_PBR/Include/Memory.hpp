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

/* Creates a buffer for suballocation */
/* These are ideal for allocating small buffers for single use in a frame */
namespace LinearBufferAllocator
{
    struct Allocation
    {
        VkBuffer Buffer;
        void * MappedAddress;
        uint64 OffsetInBytes;
        uint64 SizeInBytes;
    };

    struct AllocatorState
    {
        DeviceMemoryAllocator::Allocation DeviceMemory;

        VkBuffer Buffer;
        uint64 CurrentOffsetInBytes;
        uint64 CurrentSizeInBytes;
        uint64 CapacityInBytes;
    };

    extern bool const CreateAllocator(Vulkan::Device::DeviceState const & DeviceState, uint64 const BufferSizeInBytes, VkBufferUsageFlags const UsageFlags, VkMemoryPropertyFlags const MemoryFlags, AllocatorState & OutputState);

    extern bool const DestroyAllocator(AllocatorState & State, Vulkan::Device::DeviceState & DeviceState);

    extern bool const Allocate(AllocatorState & State, uint64 const BufferSizeInBytes, Allocation & OutputAllocation);

    extern void Reset(AllocatorState & State);
}