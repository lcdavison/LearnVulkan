#include "Graphics/Allocators.hpp"

#include "Graphics/Device.hpp"
#include "Graphics/Memory.hpp"
#include "Logging.hpp"

#include <vector>

using namespace Vulkan::Allocators;

namespace Vulkan::Allocators::Private
{
    struct LinearAllocatorState
    {
        struct Allocation
        {
            uint64 OffsetInBytes = {};
            uint64 SizeInBytes = {};
            void * MappedAddress = {};
        };

        std::vector<Allocation> Allocations = {};

        uint64 CapacityInBytes = {};
        uint64 CurrentOffsetInBytes = {};
        uint64 CurrentSizeInBytes = {};
        uint32 BufferHandle = {};
    };
}

static std::vector<Private::LinearAllocatorState> LinearAllocators = {};

bool const LinearBufferAllocator::CreateAllocator(Vulkan::Device::DeviceState const & kDeviceState, uint64 const kAllocatorSizeInBytes, VkBufferUsageFlags const kUsageFlags, VkMemoryPropertyFlags const kMemoryFlags, uint16 & OutputAllocatorHandle)
{
    if (kAllocatorSizeInBytes == 0u)
    {
        Logging::Log(Logging::LogTypes::Error, PBR_TEXT("Cannot create linear buffer allocator with size of 0 bytes."));
        return false;
    }

    Private::LinearAllocatorState & NewAllocator = { LinearAllocators.emplace_back() };
    NewAllocator.CapacityInBytes = kAllocatorSizeInBytes;
    NewAllocator.CurrentSizeInBytes = kAllocatorSizeInBytes;

    Vulkan::Device::CreateBuffer(kDeviceState, kAllocatorSizeInBytes, kUsageFlags, kMemoryFlags, NewAllocator.BufferHandle);

    OutputAllocatorHandle = static_cast<uint16>(LinearAllocators.size());

    return true;
}

bool const LinearBufferAllocator::DestroyAllocator(uint16 const kAllocatorHandle, Vulkan::Device::DeviceState const & kDeviceState)
{
    if (kAllocatorHandle == 0u)
    {
        /* ERROR */
        return false;
    }

    uint16 const kAllocatorIndex = { kAllocatorHandle - 1u };

    Private::LinearAllocatorState & LinearAllocator = { LinearAllocators [kAllocatorIndex] };

    Vulkan::Device::DestroyBuffer(kDeviceState, LinearAllocator.BufferHandle, VK_NULL_HANDLE);
    LinearAllocator.BufferHandle = 0u;

    return true;
}

bool const LinearBufferAllocator::Allocate(uint16 const kAllocatorHandle, uint64 const kBufferSizeInBytes, uint32 & OutputAllocationHandle)
{
    if (kAllocatorHandle == 0u)
    {
        Logging::Log(Logging::LogTypes::Error, PBR_TEXT("Cannot allocate memory from NULL allocator."));
        return false;
    }

    if (kBufferSizeInBytes == 0u)
    {
        Logging::Log(Logging::LogTypes::Error, PBR_TEXT("Attempting to allocate a buffer of 0 bytes in size."));
        return false;
    }

    uint16 const kAllocatorIndex = { kAllocatorHandle - 1u };

    Private::LinearAllocatorState & LinearAllocator = LinearAllocators [kAllocatorIndex];

    if (kBufferSizeInBytes > LinearAllocator.CurrentSizeInBytes)
    {
        Logging::Log(Logging::LogTypes::Error, PBR_TEXT("Failed to allocate memory. Not enough memory available."));
        return false;
    }

    Vulkan::Resource::Buffer Buffer = {};
    Vulkan::Resource::GetBuffer(LinearAllocator.BufferHandle, Buffer);

    uint64 const kOffsetInBytes = LinearAllocator.CurrentOffsetInBytes;
    LinearAllocator.CurrentOffsetInBytes += kBufferSizeInBytes;
    LinearAllocator.CurrentSizeInBytes -= kBufferSizeInBytes;

    Vulkan::Memory::AllocationInfo MemoryInfo = {};
    Vulkan::Memory::GetAllocationInfo(Buffer.MemoryAllocationHandle, MemoryInfo);

    void * const kMappedMemory = static_cast<void *>(static_cast<std::byte *>(MemoryInfo.MappedAddress) + kOffsetInBytes);

    LinearAllocator.Allocations.emplace_back(Private::LinearAllocatorState::Allocation { kOffsetInBytes, kBufferSizeInBytes, kMappedMemory });

    OutputAllocationHandle = { static_cast<uint32>(kAllocatorHandle << 16u) | static_cast<uint32>(LinearAllocator.Allocations.size()) };

    return true;
}

bool const LinearBufferAllocator::Reset(uint16 const kAllocatorHandle)
{
    if (kAllocatorHandle == 0u)
    {
        /* ERROR */
        return false;
    }

    uint16 const kAllocatorIndex = { kAllocatorHandle - 1u };

    Private::LinearAllocatorState & Allocator = LinearAllocators [kAllocatorIndex];
    Allocator.Allocations.clear();

    Allocator.CurrentOffsetInBytes = 0u;
    Allocator.CurrentSizeInBytes = Allocator.CapacityInBytes;

    return true;
}

bool const LinearBufferAllocator::GetAllocationInfo(uint32 const kAllocationHandle, Types::AllocationInfo & OutputAllocationInfo)
{
    if (kAllocationHandle == 0u)
    {
        Logging::Log(Logging::LogTypes::Error, PBR_TEXT("Invalid allocation provided"));
        return false;
    }

    uint16 const kAllocatorIndex = { static_cast<uint16>((kAllocationHandle & 0xFFFF0000) >> 16u) - 1u };
    uint16 const kAllocationIndex = { static_cast<uint16>(kAllocationHandle & 0x0000FFFF) - 1u };

    Private::LinearAllocatorState const & kAllocator = LinearAllocators [kAllocatorIndex];
    Private::LinearAllocatorState::Allocation const & kAllocation = kAllocator.Allocations [kAllocationIndex];

    OutputAllocationInfo.OffsetInBytes = kAllocation.OffsetInBytes;
    OutputAllocationInfo.SizeInBytes = kAllocation.SizeInBytes;
    OutputAllocationInfo.MappedAddress = kAllocation.MappedAddress;

    OutputAllocationInfo.BufferHandle = kAllocator.BufferHandle;

    return true;
}

bool const LinearBufferAllocator::GetMappedAddress(uint32 const kAllocationHandle, void *& OutputMappedAddress)
{
    if (kAllocationHandle == 0u)
    {
        Logging::Log(Logging::LogTypes::Error, PBR_TEXT("Invalid allocation provided"));
        return false;
    }

    uint16 const kAllocatorIndex = { static_cast<uint16>((kAllocationHandle & 0xFFFF0000) >> 16u) - 1u };
    uint16 const kAllocationIndex = { static_cast<uint16>(kAllocationHandle & 0x0000FFFF) - 1u };

    Private::LinearAllocatorState const & kAllocator = LinearAllocators [kAllocatorIndex];
    Private::LinearAllocatorState::Allocation const & kAllocation = kAllocator.Allocations [kAllocationIndex];

    OutputMappedAddress = kAllocation.MappedAddress;

    return true;
}