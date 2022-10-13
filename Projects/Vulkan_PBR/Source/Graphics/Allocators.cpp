#include "Graphics/Allocators.hpp"

#include "Graphics/Device.hpp"
#include "Graphics/Memory.hpp"
#include "Logging.hpp"

#include <vector>

using namespace Vulkan::Allocators;

namespace Vulkan::Allocators::LinearBufferAllocator::Private
{
    struct AllocationCollection
    {
        std::vector<uint16> AllocatorHandles = {};
        std::vector<void *> MappedAddresses = {};
        std::vector<uint64> OffsetsInBytes = {};
        std::vector<uint64> SizesInBytes = {};
    };

    struct AllocatorCollection
    {
        std::vector<uint32> BufferHandles = {};
        std::vector<uint64> CapacitiesInBytes = {};
        std::vector<uint64> CurrentOffsetsInBytes = {};
        std::vector<uint64> CurrentSizesInBytes = {};

        std::vector<AllocationCollection> Allocations = {};
    };
}

static Vulkan::Allocators::LinearBufferAllocator::Private::AllocatorCollection LinearAllocators = {};

bool const Vulkan::Allocators::LinearBufferAllocator::CreateAllocator(Vulkan::Device::DeviceState const & kDeviceState, uint64 const kAllocatorSizeInBytes, VkBufferUsageFlags const kUsageFlags, VkMemoryPropertyFlags const kMemoryFlags, uint16 & OutputAllocatorHandle)
{
    if (kAllocatorSizeInBytes == 0u)
    {
        Logging::Log(Logging::LogTypes::Error, PBR_TEXT("Cannot create linear buffer allocator with size of 0 bytes."));
        return false;
    }

    uint32 & BufferHandle = LinearAllocators.BufferHandles.emplace_back();
    LinearAllocators.CapacitiesInBytes.push_back(kAllocatorSizeInBytes);
    LinearAllocators.CurrentSizesInBytes.push_back(kAllocatorSizeInBytes);
    LinearAllocators.CurrentOffsetsInBytes.emplace_back();

    Vulkan::Device::CreateBuffer(kDeviceState, kAllocatorSizeInBytes, kUsageFlags, kMemoryFlags, BufferHandle);

    OutputAllocatorHandle = static_cast<uint32>(LinearAllocators.BufferHandles.size());

    return true;
}

bool const Vulkan::Allocators::LinearBufferAllocator::DestroyAllocator(uint16 const kAllocatorHandle, Vulkan::Device::DeviceState const & kDeviceState)
{
    if (kAllocatorHandle == 0u)
    {
        /* ERROR */
        return false;
    }

    uint16 const kAllocatorIndex = { kAllocatorHandle - 1u };

    uint32 const kBufferHandle = { LinearAllocators.BufferHandles [kAllocatorIndex] };
    Vulkan::Device::DestroyBuffer(kDeviceState, kBufferHandle, VK_NULL_HANDLE);

    return true;
}

bool const Vulkan::Allocators::LinearBufferAllocator::Allocate(uint16 const kAllocatorHandle, uint64 const kBufferSizeInBytes, uint16 & OutputAllocationHandle)
{

    return true;
}

bool const Vulkan::Allocators::LinearBufferAllocator::Reset(uint16 const kAllocatorHandle)
{
    return true;
}

bool const LinearBufferAllocator::CreateAllocator(Vulkan::Device::DeviceState const & DeviceState, uint64 const BufferSizeInBytes, VkBufferUsageFlags const UsageFlags, VkMemoryPropertyFlags const MemoryFlags, LinearBufferAllocator::AllocatorState & OutputState)
{
    bool bResult = false;

    LinearBufferAllocator::AllocatorState IntermediateState = {};

    uint32 BufferHandle = {};
    Vulkan::Device::CreateBuffer(DeviceState, BufferSizeInBytes, UsageFlags, MemoryFlags, BufferHandle);

    IntermediateState.Buffer = BufferHandle;
    IntermediateState.CapacityInBytes = BufferSizeInBytes;
    IntermediateState.CurrentOffsetInBytes = 0u;

    OutputState = IntermediateState;

    return bResult;
}

bool const LinearBufferAllocator::DestroyAllocator(LinearBufferAllocator::AllocatorState & State, Vulkan::Device::DeviceState & DeviceState)
{
    Vulkan::Device::DestroyBuffer(DeviceState, State.Buffer, VK_NULL_HANDLE);

    return true;
}

bool const LinearBufferAllocator::Allocate(LinearBufferAllocator::AllocatorState & State, uint64 const BufferSizeInBytes, LinearBufferAllocator::Allocation & OutputAllocation)
{
    bool bResult = false;

    if (BufferSizeInBytes < State.CurrentSizeInBytes)
    {
        LinearBufferAllocator::Allocation NewAllocation = {};

        NewAllocation.Buffer = State.Buffer;

        Vulkan::Resource::Buffer Buffer = {};
        Vulkan::Resource::GetBuffer(State.Buffer, Buffer);

        NewAllocation.MappedAddress = reinterpret_cast<void *>(reinterpret_cast<std::uintptr_t>(Buffer.MemoryAllocation->MappedAddress) + State.CurrentOffsetInBytes);

        NewAllocation.OffsetInBytes = State.CurrentOffsetInBytes;
        State.CurrentOffsetInBytes += BufferSizeInBytes;

        NewAllocation.SizeInBytes = BufferSizeInBytes;
        State.CurrentSizeInBytes -= BufferSizeInBytes;

        OutputAllocation = NewAllocation;

        bResult = true;
    }

    return bResult;
}

void LinearBufferAllocator::Reset(LinearBufferAllocator::AllocatorState & State)
{
    State.CurrentOffsetInBytes = 0u;
    State.CurrentSizeInBytes = State.CapacityInBytes;
}