#pragma once

#include "Graphics/VulkanModule.hpp"

namespace Vulkan::Device
{
    struct DeviceState;
}

namespace Vulkan::Allocators::Types
{
    struct AllocationInfo
    {
        uint64 OffsetInBytes = {};
        uint64 SizeInBytes = {};
        void * MappedAddress = {};

        uint32 BufferHandle = {};
    };
}

namespace Vulkan::Allocators::LinearBufferAllocator
{
    extern bool const CreateAllocator(Vulkan::Device::DeviceState const & kDeviceState, uint64 const kAllocatorSizeInBytes, VkBufferUsageFlags const kUsageFlags, VkMemoryPropertyFlags const kMemoryFlags, uint16 & OutputAllocatorHandle);

    extern bool const DestroyAllocator(uint16 const kAllocatorHandle, Vulkan::Device::DeviceState const & kDeviceState);

    extern bool const Allocate(uint16 const kAllocatorHandle, uint64 const kBufferSizeInBytes, uint32 & OutputAllocationHandle);

    extern bool const Reset(uint16 const kAllocatorHandle);

    extern bool const GetAllocationInfo(uint32 const kAllocationHandle, Types::AllocationInfo & OutputAllocationInfo);

    extern bool const GetMappedAddress(uint32 const kAllocationHandle, void *& OutputMappedAddress);
}