#include "Allocation.hpp"

#include "Graphics/Device.hpp"
#include "Graphics/Memory.hpp"

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