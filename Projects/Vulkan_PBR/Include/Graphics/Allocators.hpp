#pragma once

#include "Graphics/VulkanModule.hpp"

namespace Vulkan::Device
{
    struct DeviceState;
}

namespace Vulkan::Allocators::LinearBufferAllocator
{
    extern bool const CreateAllocator(Vulkan::Device::DeviceState const & kDeviceState, uint64 const kAllocatorSizeInBytes, VkBufferUsageFlags const kUsageFlags, VkMemoryPropertyFlags const kMemoryFlags, uint16 & OutputAllocatorHandle);

    extern bool const DestroyAllocator(uint16 const kAllocatorHandle, Vulkan::Device::DeviceState const & kDeviceState);

    extern bool const Allocate(uint16 const kAllocatorHandle, uint64 const kBufferSizeInBytes, uint16 & OutputAllocationHandle);

    extern bool const Reset(uint16 const kAllocatorHandle);

    extern bool const GetMappedAddress(uint16 const kAllocationHandle, void *& OutputMappedAddress);
}

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

    extern bool const CreateAllocator(Vulkan::Device::DeviceState const & kDeviceState, uint64 const kBufferSizeInBytes, VkBufferUsageFlags const kUsageFlags, VkMemoryPropertyFlags const kMemoryFlags, AllocatorState & OutputState);

    extern bool const DestroyAllocator(AllocatorState & State, Vulkan::Device::DeviceState & DeviceState);

    extern bool const Allocate(AllocatorState & State, uint64 const BufferSizeInBytes, Allocation & OutputAllocation);

    extern void Reset(AllocatorState & State);
}