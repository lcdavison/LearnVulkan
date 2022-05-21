#include "Memory.hpp"

#include "Graphics/Device.hpp"

#include <vector>
#include <array>

static uint8 const kMaximumMemoryPoolBlockCount = { 16u };

/* Free List Allocations */
struct DeviceMemoryPool
{
    std::array<std::vector<uint64>, kMaximumMemoryPoolBlockCount> BlockFreeListOffsetsInBytes;
    std::array<std::vector<uint64>, kMaximumMemoryPoolBlockCount> BlockFreeListSizesInBytes;

    std::array<VkDeviceMemory, kMaximumMemoryPoolBlockCount> MemoryBlocks;
    std::array<VkDeviceSize, kMaximumMemoryPoolBlockCount> MemoryBlockAllocatedSizeInBytes;
    std::array<void *, kMaximumMemoryPoolBlockCount> BlockBaseMemoryAddresses;

    uint8 NextUnallocatedMemoryBlockIndex = { 0u };
};

static uint64 const kDefaultBlockSizeInBytes = { 8192u };

static std::array<DeviceMemoryPool, VK_MAX_MEMORY_TYPES> MemoryPools;

static bool const GetDeviceMemoryTypeIndex(Vulkan::Device::DeviceState const & DeviceState, uint32 MemoryTypeMask, VkMemoryPropertyFlags const RequiredMemoryTypeFlags, uint32 & OutputMemoryTypeIndex)
{
    bool bResult = false;

    VkPhysicalDeviceMemoryProperties MemoryProperties = {};
    vkGetPhysicalDeviceMemoryProperties(DeviceState.PhysicalDevice, &MemoryProperties);

    uint32 CurrentMemoryTypeIndex = {};
    while (_BitScanForward(reinterpret_cast<unsigned long *>(&CurrentMemoryTypeIndex), MemoryTypeMask))
    {
        /* Clear the bit from the mask */
        MemoryTypeMask ^= (1u << CurrentMemoryTypeIndex);

        VkMemoryType const & CurrentMemoryType = MemoryProperties.memoryTypes [CurrentMemoryTypeIndex];

        if ((CurrentMemoryType.propertyFlags & RequiredMemoryTypeFlags) == RequiredMemoryTypeFlags)
        {
            OutputMemoryTypeIndex = CurrentMemoryTypeIndex;
            bResult = true;
            break;
        }
    }

    return bResult;
}

static bool const GetDeviceMemoryBlockIndex(DeviceMemoryPool const & SelectedMemoryPool, VkMemoryRequirements const & MemoryRequirements, uint32 & OutputMemoryBlockIndex, uint32 & OutputFreeListChunkIndex)
{
    bool bResult = false;

    for (uint8 CurrentBlockIndex = { 0u };
         CurrentBlockIndex < kMaximumMemoryPoolBlockCount && !bResult;
         CurrentBlockIndex++)
    {
        std::vector<uint64> const & CurrentFreeList = SelectedMemoryPool.BlockFreeListSizesInBytes [CurrentBlockIndex];

        /* Use a First-Fit method here, this should suffice currently */
        for (uint16 CurrentChunkIndex = { 0u };
             CurrentChunkIndex < CurrentFreeList.size();
             CurrentChunkIndex++)
        {
            if (MemoryRequirements.size <= CurrentFreeList [CurrentChunkIndex])
            {
                OutputMemoryBlockIndex = CurrentBlockIndex;
                OutputFreeListChunkIndex = CurrentChunkIndex;
                bResult = true;
                break;
            }
        }
    }

    return bResult;
}

static void CreateMemoryBlockForPool(DeviceMemoryPool & MemoryPool, Vulkan::Device::DeviceState const & DeviceState, uint32 const MemoryTypeIndex, uint32 & OutputMemoryBlockIndex, VkDeviceMemory & OutputDeviceMemory)
{
    VkMemoryAllocateInfo AllocateInfo = {};
    AllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    AllocateInfo.allocationSize = kDefaultBlockSizeInBytes;
    AllocateInfo.memoryTypeIndex = MemoryTypeIndex;

    VkDeviceMemory NewMemoryBlock = {};
    VERIFY_VKRESULT(vkAllocateMemory(DeviceState.Device, &AllocateInfo, nullptr, &NewMemoryBlock));

    uint32 MemoryBlockIndex = static_cast<uint32>(MemoryPool.NextUnallocatedMemoryBlockIndex);
    MemoryPool.NextUnallocatedMemoryBlockIndex++;

    MemoryPool.MemoryBlocks [MemoryBlockIndex] = NewMemoryBlock;

    MemoryPool.BlockFreeListOffsetsInBytes [MemoryBlockIndex].push_back(0u);
    MemoryPool.BlockFreeListSizesInBytes [MemoryBlockIndex].push_back(kDefaultBlockSizeInBytes);

    MemoryPool.MemoryBlockAllocatedSizeInBytes [MemoryBlockIndex] = 0u;

    OutputMemoryBlockIndex = MemoryBlockIndex;
    OutputDeviceMemory = NewMemoryBlock;
}

bool const DeviceMemoryAllocator::AllocateMemory(Vulkan::Device::DeviceState const & DeviceState, VkMemoryRequirements const & MemoryRequirements, VkMemoryPropertyFlags const MemoryFlags, Allocation & OutputAllocation)
{
    bool bResult = true;

    uint32 MemoryTypeIndex = {};
    ::GetDeviceMemoryTypeIndex(DeviceState, MemoryRequirements.memoryTypeBits, MemoryFlags, MemoryTypeIndex);

    DeviceMemoryPool & SelectedMemoryPool = MemoryPools [MemoryTypeIndex];

    bool const bCPUVisible = (MemoryFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) > 0u;

    uint32 MemoryBlockIndex = {};
    uint32 FreeListChunkIndex = {};
    if (!::GetDeviceMemoryBlockIndex(SelectedMemoryPool, MemoryRequirements, MemoryBlockIndex, FreeListChunkIndex))
    {
        VkDeviceMemory NewMemoryBlock = {};
        ::CreateMemoryBlockForPool(SelectedMemoryPool, DeviceState, MemoryTypeIndex, MemoryBlockIndex, NewMemoryBlock);

        if (bCPUVisible)
        {
            VERIFY_VKRESULT(vkMapMemory(DeviceState.Device, NewMemoryBlock, 0u, kDefaultBlockSizeInBytes, 0u, &SelectedMemoryPool.BlockBaseMemoryAddresses [MemoryBlockIndex]));
        }
    }

    /* Offsets should be aligned in the memory block */
    /* We will cause fragmentation due to the alignment, so when freeing it would be worth */
    uint64 const MemoryBlockBaseOffset = SelectedMemoryPool.BlockFreeListOffsetsInBytes [MemoryBlockIndex][FreeListChunkIndex];

    /* Alignment here may not be necessary, since VkMemoryRequirements already aligns the size */
    uint64 const MemoryBlockAlignedOffset = (MemoryBlockBaseOffset + MemoryRequirements.alignment - 1u) & ~(MemoryRequirements.alignment - 1u);
    uint16 const MemoryBlockAlignmentWasteInBytes = static_cast<uint16>(MemoryBlockAlignedOffset - MemoryBlockBaseOffset);

    uint64 const MemoryBlockSize = SelectedMemoryPool.BlockFreeListSizesInBytes [MemoryBlockIndex][FreeListChunkIndex];
    uint64 const AllocationSizeInBytes = MemoryBlockAlignmentWasteInBytes + MemoryRequirements.size;

    if (MemoryBlockSize > MemoryRequirements.size)
    {
        SelectedMemoryPool.BlockFreeListOffsetsInBytes [MemoryBlockIndex][FreeListChunkIndex] += AllocationSizeInBytes;
        SelectedMemoryPool.BlockFreeListSizesInBytes [MemoryBlockIndex][FreeListChunkIndex] -= AllocationSizeInBytes;
    }
    else
    {
        /* Don't really like this but it will do for now */
        auto OffsetIterator = SelectedMemoryPool.BlockFreeListOffsetsInBytes [MemoryBlockIndex].cbegin() + FreeListChunkIndex;
        auto SizeIterator = SelectedMemoryPool.BlockFreeListSizesInBytes [MemoryBlockIndex].cbegin() + FreeListChunkIndex;

        SelectedMemoryPool.BlockFreeListOffsetsInBytes [MemoryBlockIndex].erase(OffsetIterator);
        SelectedMemoryPool.BlockFreeListSizesInBytes [MemoryBlockIndex].erase(SizeIterator);
    }

    /* OffsetInBytes - AlignmentWaste == BaseAllocationOffset */

    OutputAllocation.Memory = SelectedMemoryPool.MemoryBlocks [MemoryBlockIndex];
    OutputAllocation.AllocationID = (static_cast<uint64>(MemoryTypeIndex) << 32u) | static_cast<uint64>(MemoryBlockIndex);
    OutputAllocation.OffsetInBytes = MemoryBlockAlignedOffset;
    OutputAllocation.SizeInBytes = MemoryRequirements.size;
    OutputAllocation.AlignmentWasteInBytes = MemoryBlockAlignmentWasteInBytes;
    OutputAllocation.MappedAddress = bCPUVisible ? reinterpret_cast<void *>(reinterpret_cast<std::byte *>(SelectedMemoryPool.BlockBaseMemoryAddresses [MemoryBlockIndex]) + MemoryBlockAlignedOffset) : nullptr;

    return bResult;
}

bool const DeviceMemoryAllocator::FreeMemory(DeviceMemoryAllocator::Allocation & Allocation)
{
    /* TODO: This needs to take into account alignment when freeing */

    bool bResult = false;

    /* First find pool, then find block that this allocation is from */
    uint32 const MemoryPoolIndex = (Allocation.AllocationID & 0xFFFFFFFF00000000) >> 32u;
    uint32 const PoolBlockIndex = (Allocation.AllocationID & 0x00000000FFFFFFFF);

    DeviceMemoryPool & SelectedMemoryPool = MemoryPools [MemoryPoolIndex];

    uint32 const CurrentFreeBlockCount = static_cast<uint32>(SelectedMemoryPool.BlockFreeListOffsetsInBytes [PoolBlockIndex].size());

    /* Get indices of lower and upper bound chunks */
    uint32 GreatestLowerOffsetIndex = {};
    uint32 SmallestHigherOffsetIndex = {};

    uint64 GreatestLowerOffset = { std::numeric_limits<uint64>::min() };
    uint64 SmallestHigherOffset = { std::numeric_limits<uint64>::max() };

    bool bFoundLowerOffset = false;
    bool bFoundHigherOffset = false;

    for (uint32 CurrentOffsetIndex = { 0u };
            CurrentOffsetIndex < SelectedMemoryPool.BlockFreeListOffsetsInBytes [PoolBlockIndex].size();
            CurrentOffsetIndex++)
    {
        uint64 CurrentOffsetInBytes = SelectedMemoryPool.BlockFreeListOffsetsInBytes [PoolBlockIndex][CurrentOffsetIndex];

        if (CurrentOffsetInBytes < Allocation.OffsetInBytes && CurrentOffsetInBytes >= GreatestLowerOffset)
        {
            GreatestLowerOffsetIndex = CurrentOffsetIndex;
            GreatestLowerOffset = CurrentOffsetInBytes;

            bFoundLowerOffset = true;
        }
        else if (CurrentOffsetInBytes > Allocation.OffsetInBytes && CurrentOffsetInBytes <= SmallestHigherOffset)
        {
            SmallestHigherOffsetIndex = CurrentOffsetIndex;
            SmallestHigherOffset = CurrentOffsetInBytes;

            bFoundHigherOffset = true;
        }
    }

    /* 4 Cases:
    *  0 - No offsets just append to free list
    *  1 - 1 offset to left, merge left
    *  2 - 1 offset to right, merge right
    *  3 - Offsets left and right, merge from left to right
    */
    bool const bCanMergeLeftBlock = 
        bFoundLowerOffset &&
        GreatestLowerOffset + SelectedMemoryPool.BlockFreeListSizesInBytes [PoolBlockIndex][GreatestLowerOffsetIndex] == Allocation.OffsetInBytes;

    bool const bCanMergeRightBlock = 
        bFoundHigherOffset &&
        Allocation.OffsetInBytes + Allocation.SizeInBytes == SelectedMemoryPool.BlockFreeListOffsetsInBytes [PoolBlockIndex][SmallestHigherOffsetIndex];

    if (bCanMergeLeftBlock && bCanMergeRightBlock)
    {
        SelectedMemoryPool.BlockFreeListSizesInBytes [PoolBlockIndex][GreatestLowerOffsetIndex] += Allocation.SizeInBytes + SelectedMemoryPool.BlockFreeListSizesInBytes [PoolBlockIndex][SmallestHigherOffsetIndex];

        SelectedMemoryPool.BlockFreeListSizesInBytes [PoolBlockIndex].erase(SelectedMemoryPool.BlockFreeListSizesInBytes [PoolBlockIndex].cbegin() + SmallestHigherOffsetIndex);
        SelectedMemoryPool.BlockFreeListOffsetsInBytes [PoolBlockIndex].erase(SelectedMemoryPool.BlockFreeListSizesInBytes [PoolBlockIndex].cbegin() + SmallestHigherOffsetIndex);
    }
    else if (bCanMergeLeftBlock)
    {
        SelectedMemoryPool.BlockFreeListSizesInBytes [PoolBlockIndex][GreatestLowerOffsetIndex] += Allocation.SizeInBytes;
    }
    else if (bCanMergeRightBlock)
    {
        SelectedMemoryPool.BlockFreeListOffsetsInBytes [PoolBlockIndex][SmallestHigherOffsetIndex] -= Allocation.SizeInBytes;
        SelectedMemoryPool.BlockFreeListSizesInBytes [PoolBlockIndex][SmallestHigherOffsetIndex] += Allocation.SizeInBytes;
    }
    else
    {
        SelectedMemoryPool.BlockFreeListOffsetsInBytes [PoolBlockIndex].push_back(Allocation.OffsetInBytes);
        SelectedMemoryPool.BlockFreeListSizesInBytes [PoolBlockIndex].push_back(Allocation.SizeInBytes);
    }

    return bResult;
}

void DeviceMemoryAllocator::FreeAllDeviceMemory(Vulkan::Device::DeviceState const & DeviceState)
{
    for (DeviceMemoryPool & Pool : MemoryPools)
    {
        for (VkDeviceMemory MemoryBlock : Pool.MemoryBlocks)
        {
            if (MemoryBlock)
            {
                vkFreeMemory(DeviceState.Device, MemoryBlock, nullptr);
                MemoryBlock = VK_NULL_HANDLE;
            }
        }
    }
}

bool const LinearBufferAllocator::CreateAllocator(Vulkan::Device::DeviceState const & DeviceState, uint64 const BufferSizeInBytes, VkBufferUsageFlags const UsageFlags, VkMemoryPropertyFlags const MemoryFlags, LinearBufferAllocator::AllocatorState & OutputState)
{
    bool bResult = false;

    LinearBufferAllocator::AllocatorState IntermediateState = {};

    Vulkan::Device::CreateBuffer(DeviceState, BufferSizeInBytes, UsageFlags, MemoryFlags, IntermediateState.Buffer, IntermediateState.DeviceMemory);

    IntermediateState.CapacityInBytes = BufferSizeInBytes;
    IntermediateState.CurrentOffsetInBytes = 0u;

    OutputState = IntermediateState;

    return bResult;
}

bool const LinearBufferAllocator::DestroyAllocator(LinearBufferAllocator::AllocatorState & State, Vulkan::Device::DeviceState & DeviceState)
{
    Vulkan::Device::DestroyBuffer(DeviceState, State.Buffer);

    DeviceMemoryAllocator::FreeMemory(State.DeviceMemory);

    return true;
}

bool const LinearBufferAllocator::Allocate(LinearBufferAllocator::AllocatorState & State, uint64 const BufferSizeInBytes, LinearBufferAllocator::Allocation & OutputAllocation)
{
    bool bResult = false;

    if (BufferSizeInBytes < State.CurrentSizeInBytes)
    {
        LinearBufferAllocator::Allocation NewAllocation = {};

        NewAllocation.Buffer = State.Buffer;

        NewAllocation.MappedAddress = reinterpret_cast<void *>(reinterpret_cast<std::uintptr_t>(State.DeviceMemory.MappedAddress) + State.CurrentOffsetInBytes);

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