#include "Memory.hpp"

#include "Graphics/Device.hpp"

#include <vector>
#include <array>

static uint8 const kMaximumMemoryPoolBlockCount = { 16u };

/* TODO: Set this up as a free list */
/* Fragmentation will be an issue */
struct MemoryPool
{
    std::array<std::vector<uint64>, kMaximumMemoryPoolBlockCount> BlockFreeListOffsetsInBytes;
    std::array<std::vector<uint64>, kMaximumMemoryPoolBlockCount> BlockFreeListSizesInBytes;

    std::vector<VkDeviceMemory> MemoryBlocks;
    std::vector<uint64> MemoryBlockAllocatedSizeInBytes;
};

static uint64 const kDefaultBlockSizeInBytes = { 2048u };

static std::array<MemoryPool, VK_MAX_MEMORY_TYPES> MemoryPools;

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

static bool const GetDeviceMemoryBlockIndex(MemoryPool const & SelectedMemoryPool, VkMemoryRequirements const & MemoryRequirements, uint32 & OutputMemoryBlockIndex, uint32 & OutputFreeListIndex)
{
    bool bResult = false;

    for (uint8 CurrentBlockIndex = { 0u };
         CurrentBlockIndex < kMaximumMemoryPoolBlockCount && !bResult;
         CurrentBlockIndex++)
    {
        std::vector<uint64> const & CurrentFreeList = SelectedMemoryPool.BlockFreeListSizesInBytes [CurrentBlockIndex];
        if (CurrentFreeList.size() > 0u)
        {
            /* Place the biggest chunks at the end and split if necessary */
            /* Perform an insertion sort on free to perform an iterative sort */
            /* Not the nicest strategy, but simple and we can try and merge some blocks on free to reduce fragmentation */
            if (MemoryRequirements.size <= CurrentFreeList [CurrentFreeList.size() - 1u])
            {
                OutputMemoryBlockIndex = CurrentBlockIndex;
                OutputFreeListIndex = static_cast<uint32>(CurrentFreeList.size() - 1u);
                bResult = true;
                break;
            }
        }
    }

    return bResult;
}

bool const DeviceMemoryAllocator::AllocateMemory(Vulkan::Device::DeviceState const & DeviceState, VkMemoryRequirements const & MemoryRequirements, VkMemoryPropertyFlags const MemoryFlags, Allocation & OutputAllocation)
{
    bool bResult = true;

    uint32 MemoryTypeIndex = {};
    ::GetDeviceMemoryTypeIndex(DeviceState, MemoryRequirements.memoryTypeBits, MemoryFlags, MemoryTypeIndex);

    MemoryPool & SelectedMemoryPool = MemoryPools [MemoryTypeIndex];

    uint32 MemoryBlockIndex = {};
    uint32 FreeListIndex = {};
    if (!::GetDeviceMemoryBlockIndex(SelectedMemoryPool, MemoryRequirements, MemoryBlockIndex, FreeListIndex))
    {
        VkMemoryAllocateInfo AllocateInfo = {};
        AllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        AllocateInfo.allocationSize = kDefaultBlockSizeInBytes;
        AllocateInfo.memoryTypeIndex = MemoryTypeIndex;

        VkDeviceMemory NewMemoryBlock = {};
        VERIFY_VKRESULT(vkAllocateMemory(DeviceState.Device, &AllocateInfo, nullptr, &NewMemoryBlock));

        SelectedMemoryPool.MemoryBlocks.push_back(NewMemoryBlock);
        MemoryBlockIndex = static_cast<uint32>(SelectedMemoryPool.MemoryBlocks.size() - 1u);

        SelectedMemoryPool.BlockFreeListOffsetsInBytes [MemoryBlockIndex].push_back(0u);
        SelectedMemoryPool.BlockFreeListSizesInBytes [MemoryBlockIndex].push_back(kDefaultBlockSizeInBytes);
        FreeListIndex = 0u;
    }

    /* Offsets should be aligned in the memory block */
    /* We will cause fragmentation due to the alignment, so when freeing it would be worth */
    uint64 const MemoryBlockBaseOffset = SelectedMemoryPool.BlockFreeListOffsetsInBytes [MemoryBlockIndex][FreeListIndex];
    uint64 const MemoryBlockAlignedOffset = (MemoryBlockBaseOffset + MemoryRequirements.alignment - 1u) & ~(MemoryRequirements.alignment - 1u);
    uint16 const MemoryBlockAlignmentWasteInBytes = static_cast<uint16>(MemoryBlockAlignedOffset - MemoryBlockBaseOffset);

    uint64 const MemoryBlockSize = SelectedMemoryPool.BlockFreeListSizesInBytes [MemoryBlockIndex][FreeListIndex];
    uint64 const AllocationSizeInBytes = MemoryBlockAlignmentWasteInBytes + MemoryRequirements.size;

    SelectedMemoryPool.BlockFreeListOffsetsInBytes [MemoryBlockIndex].pop_back();
    SelectedMemoryPool.BlockFreeListSizesInBytes [MemoryBlockIndex].pop_back();

    if (MemoryBlockSize > MemoryRequirements.size)
    {
        uint64 const OffsetAfterSplit = MemoryBlockBaseOffset + AllocationSizeInBytes;
        uint64 const SizeAfterSplit = MemoryBlockSize - AllocationSizeInBytes;

        SelectedMemoryPool.BlockFreeListOffsetsInBytes [MemoryBlockIndex].push_back(OffsetAfterSplit);
        SelectedMemoryPool.BlockFreeListSizesInBytes [MemoryBlockIndex].push_back(SizeAfterSplit);

        /* Keep in sorted order of size, worst case O(N) */
        for (uint32 CurrentFreeListIndex = { static_cast<uint32>(SelectedMemoryPool.BlockFreeListSizesInBytes [MemoryBlockIndex].size() - 1u) };
             CurrentFreeListIndex > 0u && SelectedMemoryPool.BlockFreeListSizesInBytes [MemoryBlockIndex][CurrentFreeListIndex - 1u] > MemoryRequirements.size;
             CurrentFreeListIndex--)
        {
            std::swap(SelectedMemoryPool.BlockFreeListSizesInBytes [MemoryBlockIndex][CurrentFreeListIndex - 1u], SelectedMemoryPool.BlockFreeListSizesInBytes [MemoryBlockIndex][CurrentFreeListIndex]);
            std::swap(SelectedMemoryPool.BlockFreeListOffsetsInBytes [MemoryBlockIndex][CurrentFreeListIndex - 1u], SelectedMemoryPool.BlockFreeListOffsetsInBytes [MemoryBlockIndex][CurrentFreeListIndex]);
        }
    }

    /* OffsetInBytes - AlignmentWaste == BaseAllocationOffset */

    OutputAllocation.AllocationID = (static_cast<uint64>(MemoryTypeIndex) << 32u) | static_cast<uint64>(MemoryBlockIndex);
    OutputAllocation.OffsetInBytes = MemoryBlockAlignedOffset;
    OutputAllocation.SizeInBytes = AllocationSizeInBytes;
    OutputAllocation.AlignmentWasteInBytes = MemoryBlockAlignmentWasteInBytes;

    return bResult;
}

bool const DeviceMemoryAllocator::FreeMemory(DeviceMemoryAllocator::Allocation & /*Allocation*/, Vulkan::Device::DeviceState const & /*DeviceState*/)
{
    bool bResult = false;

    return bResult;
}

bool const LinearBufferAllocator::CreateAllocator(Vulkan::Device::DeviceState const & DeviceState, uint64 const BufferSizeInBytes, VkBufferUsageFlags const UsageFlags, VkMemoryPropertyFlags const MemoryFlags, LinearBufferAllocator::AllocatorState & OutputState)
{
    bool bResult = false;

    LinearBufferAllocator::AllocatorState IntermediateState = {};

    {
        VkBufferCreateInfo CreateInfo = {};
        CreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        CreateInfo.usage = UsageFlags;
        CreateInfo.size = BufferSizeInBytes;

        VERIFY_VKRESULT(vkCreateBuffer(DeviceState.Device, &CreateInfo, nullptr, &IntermediateState.Buffer));
    }
    
    VkMemoryRequirements MemoryRequirements = {};
    vkGetBufferMemoryRequirements(DeviceState.Device, IntermediateState.Buffer, &MemoryRequirements);

    DeviceMemoryAllocator::Allocation NewAllocation = {};
    bResult = DeviceMemoryAllocator::AllocateMemory(DeviceState, MemoryRequirements, MemoryFlags, NewAllocation);

    IntermediateState.CapacityInBytes = BufferSizeInBytes;
    IntermediateState.CurrentOffsetInBytes = 0u;
    IntermediateState.DeviceMemory = NewAllocation;

    OutputState = IntermediateState;

    return bResult;
}

bool const LinearBufferAllocator::DestroyAllocator(LinearBufferAllocator::AllocatorState & /*State*/, Vulkan::Device::DeviceState & /*DeviceState*/)
{
    bool bResult = false;

    return bResult;
}

bool const LinearBufferAllocator::Allocate(LinearBufferAllocator::AllocatorState & /*State*/, Vulkan::Device::DeviceState const & /*DeviceState*/, uint64 const /*BufferSizeInBytes*/, Allocation & /*OutputAllocation*/)
{
    bool bResult = true;

   /* NewAllocation.AllocationID |= (static_cast<uint64>(MemoryTypeIndex) << 32u);
    NewAllocation.AllocationID |= (static_cast<uint64>(MemoryBlockIndex));

    NewAllocation.OffsetInBytes = CurrentMemoryBlockOffsetInBytes;
    CurrentMemoryBlockOffsetInBytes += MemoryRequirements.size;

    NewAllocation.SizeInBytes = MemoryRequirements.size;
    CurrentMemoryBlockAllocatedSizeInBytes += MemoryRequirements.size;*/

    return bResult;
}