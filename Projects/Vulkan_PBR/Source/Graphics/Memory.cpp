#include "Graphics/Memory.hpp"

#include "Graphics/Device.hpp"

#include <vector>
#include <array>

static uint8 const kMaximumMemoryPoolBlockCount = { 16u };

/* Free List Allocations */
struct DeviceMemoryPool
{
    /* TODO: Instead of using vectors for each block, we could use a hash table indexed by size and chained with offsets */
    /* Just take the first offset from the chain */
    /* Coalescing the memory back in will be a bit more involved though */
    std::array<std::vector<uint64>, kMaximumMemoryPoolBlockCount> BlockFreeListOffsetsInBytes;
    std::array<std::vector<uint64>, kMaximumMemoryPoolBlockCount> BlockFreeListSizesInBytes;

    std::array<VkDeviceMemory, kMaximumMemoryPoolBlockCount> MemoryBlocks;
    std::array<VkDeviceSize, kMaximumMemoryPoolBlockCount> MemoryBlockAllocatedSizeInBytes;
    std::array<void *, kMaximumMemoryPoolBlockCount> BlockBaseMemoryAddresses;

    uint8 NextUnallocatedMemoryBlockIndex = { 0u };
};

static uint64 const kDefaultBlockSizeInBytes = { 128u << 10u << 10u }; // 128 MiB

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
        for (int16 CurrentChunkIndex = { static_cast<int16>(CurrentFreeList.size() - 1l) };
             CurrentChunkIndex >= 0l;
             CurrentChunkIndex--)
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

    /* TODO: Have a fallback for allocations bigger than default block size */

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
    uint64 const MemoryBlockBaseOffset = SelectedMemoryPool.BlockFreeListOffsetsInBytes [MemoryBlockIndex][FreeListChunkIndex];

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
    bool bResult = false;

    /* First find pool, then find block that this allocation is from */
    uint32 const MemoryPoolIndex = (Allocation.AllocationID & 0xFFFFFFFF00000000) >> 32u;
    uint32 const PoolBlockIndex = (Allocation.AllocationID & 0x00000000FFFFFFFF);

    DeviceMemoryPool & SelectedMemoryPool = MemoryPools [MemoryPoolIndex];

    uint64 const AllocationOffsetInBytes = Allocation.OffsetInBytes - Allocation.AlignmentWasteInBytes;
    uint64 const AllocationSizeInBytes = Allocation.SizeInBytes + Allocation.AlignmentWasteInBytes;

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

        if (CurrentOffsetInBytes < AllocationOffsetInBytes && CurrentOffsetInBytes >= GreatestLowerOffset)
        {
            GreatestLowerOffsetIndex = CurrentOffsetIndex;
            GreatestLowerOffset = CurrentOffsetInBytes;

            bFoundLowerOffset = true;
        }
        else if (CurrentOffsetInBytes > AllocationOffsetInBytes && CurrentOffsetInBytes <= SmallestHigherOffset)
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
        GreatestLowerOffset + SelectedMemoryPool.BlockFreeListSizesInBytes [PoolBlockIndex][GreatestLowerOffsetIndex] == AllocationOffsetInBytes;

    bool const bCanMergeRightBlock =
        bFoundHigherOffset &&
        AllocationOffsetInBytes + AllocationSizeInBytes == SelectedMemoryPool.BlockFreeListOffsetsInBytes [PoolBlockIndex][SmallestHigherOffsetIndex];

    if (bCanMergeLeftBlock && bCanMergeRightBlock)
    {
        SelectedMemoryPool.BlockFreeListSizesInBytes [PoolBlockIndex][GreatestLowerOffsetIndex] += AllocationSizeInBytes + SelectedMemoryPool.BlockFreeListSizesInBytes [PoolBlockIndex][SmallestHigherOffsetIndex];

        SelectedMemoryPool.BlockFreeListSizesInBytes [PoolBlockIndex].erase(SelectedMemoryPool.BlockFreeListSizesInBytes [PoolBlockIndex].cbegin() + SmallestHigherOffsetIndex);
        SelectedMemoryPool.BlockFreeListOffsetsInBytes [PoolBlockIndex].erase(SelectedMemoryPool.BlockFreeListOffsetsInBytes [PoolBlockIndex].cbegin() + SmallestHigherOffsetIndex);
    }
    else if (bCanMergeLeftBlock)
    {
        SelectedMemoryPool.BlockFreeListSizesInBytes [PoolBlockIndex][GreatestLowerOffsetIndex] += AllocationSizeInBytes;
    }
    else if (bCanMergeRightBlock)
    {
        SelectedMemoryPool.BlockFreeListOffsetsInBytes [PoolBlockIndex][SmallestHigherOffsetIndex] -= AllocationSizeInBytes;
        SelectedMemoryPool.BlockFreeListSizesInBytes [PoolBlockIndex][SmallestHigherOffsetIndex] += AllocationSizeInBytes;
    }
    else
    {
        SelectedMemoryPool.BlockFreeListOffsetsInBytes [PoolBlockIndex].push_back(AllocationOffsetInBytes);
        SelectedMemoryPool.BlockFreeListSizesInBytes [PoolBlockIndex].push_back(AllocationSizeInBytes);
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