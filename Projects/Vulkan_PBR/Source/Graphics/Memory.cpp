#include "Graphics/Memory.hpp"

#include "Graphics/Device.hpp"

#include <array>
#include <list>
#include <map>
#include <unordered_map>
#include <vector>
#include <queue>

using namespace Vulkan::Memory;

// TODO: Need a way to free all device memory

namespace Vulkan::Memory::Private
{
    namespace HandleConstants
    {
        static uint64 const kMemoryTypeMask = { 0x000000FF00000000 };
        static uint64 const kHandleMask = { 0x00000000FFFFFFFF };
    }

    static uint64 const kMemoryBlockSizeInBytes = { 256u << 10u << 10u }; // 512 MiB
    static uint8 const kMaximumPoolBlockCount = { 16u };

    /* Free list allocator with memory coalescing */
    /* Just use a single block of memory atm */
    struct MemoryPool
    {
        struct MemoryChunk
        {
            uint64 OffsetInBytes = {};
            uint64 SizeInBytes = {};
            uint32 AllocationHandle = {}; // this is so that we can remap the chunk in memory later
            uint16 AlignmentOffsetInBytes = {};
        };

        std::unordered_map<uint32, uint32> AllocationHandleRemappingTable = {};
        std::vector<MemoryChunk> AllocatedChunks = {};

        std::multimap<uint64, MemoryChunk> ChunkSizeInBytesToFreeChunkMap = {};
        std::map<uint64, std::multimap<uint64, MemoryChunk>::iterator> ChunkOffsetToFreeChunkMap = {};

        VkDeviceMemory DeviceMemory = {};
        void * DeviceMemoryStartAddress = {};
    };
}

static std::unordered_map<uint8, Private::MemoryPool> DeviceMemoryPools = {};

static bool const GetDeviceMemoryType(Vulkan::Device::DeviceState const & kDeviceState, VkMemoryRequirements const & kMemoryRequirements, VkMemoryPropertyFlags const & kMemoryProperties, uint32 & OutputMemoryTypeIndex)
{
    bool bResult = false;

    VkPhysicalDeviceMemoryProperties MemoryProperties = {};
    vkGetPhysicalDeviceMemoryProperties(kDeviceState.PhysicalDevice, &MemoryProperties);

    uint32 CurrentMemoryTypeIndex = {};
    uint32 MemoryTypeMask = { kMemoryRequirements.memoryTypeBits };
    while (_BitScanForward(reinterpret_cast<unsigned long *>(&CurrentMemoryTypeIndex), MemoryTypeMask))
    {
        MemoryTypeMask ^= 1u << CurrentMemoryTypeIndex;

        if ((MemoryProperties.memoryTypes [CurrentMemoryTypeIndex].propertyFlags & kMemoryProperties) == kMemoryProperties)
        {
            OutputMemoryTypeIndex = CurrentMemoryTypeIndex;
            bResult = true;
            break;
        }
    }

    return bResult;
}

static bool const InitialiseMemoryPool(Private::MemoryPool & MemoryPool, Vulkan::Device::DeviceState const & kDeviceState, uint32 const kMemoryTypeIndex, bool const bIsCPUVisible)
{
    VkMemoryAllocateInfo const kAllocationInfo = 
    {
        VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        nullptr,
        Private::kMemoryBlockSizeInBytes,
        kMemoryTypeIndex,
    };

    VERIFY_VKRESULT(vkAllocateMemory(kDeviceState.Device, &kAllocationInfo, nullptr, &MemoryPool.DeviceMemory));

    if (bIsCPUVisible)
    {
        VERIFY_VKRESULT(vkMapMemory(kDeviceState.Device, MemoryPool.DeviceMemory, 0u, VK_WHOLE_SIZE, static_cast<VkMemoryMapFlags>(0u), &MemoryPool.DeviceMemoryStartAddress));
    }

    decltype(MemoryPool.ChunkSizeInBytesToFreeChunkMap)::iterator FreeChunkIterator = MemoryPool.ChunkSizeInBytesToFreeChunkMap.insert(std::make_pair(Private::kMemoryBlockSizeInBytes, Private::MemoryPool::MemoryChunk { 0u, Private::kMemoryBlockSizeInBytes, 0u, 0u }));
    MemoryPool.ChunkOffsetToFreeChunkMap.insert(std::make_pair(0u, FreeChunkIterator));

    return true;
}

bool const Vulkan::Memory::Allocate(Vulkan::Device::DeviceState const & kDeviceState, VkMemoryRequirements const & kMemoryRequirements, VkMemoryPropertyFlags const kMemoryFlags, uint64 & OutputAllocationHandle)
{
    uint32 MemoryTypeIndex = {};
    bool bResult = ::GetDeviceMemoryType(kDeviceState, kMemoryRequirements, static_cast<uint32>(kMemoryFlags), MemoryTypeIndex);
    
    if (bResult)
    {
        Private::MemoryPool & MemoryPool = DeviceMemoryPools [static_cast<uint8>(MemoryTypeIndex)];

        bool const kbIsCPUVisible = { (kMemoryFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) != 0u };

        // check if the memory block has been allocated, allocate if not
        if (MemoryPool.DeviceMemory == VK_NULL_HANDLE)
        {
            /* we need to initialise the memory pool */
            ::InitialiseMemoryPool(MemoryPool, kDeviceState, MemoryTypeIndex, kbIsCPUVisible);
        }

        // worst case size since it needs to be aligned properly relative to the memory block
        // this will cause some wastage
        uint64 const kAllocationSizeInBytes = { kMemoryRequirements.size + kMemoryRequirements.alignment };

        // search the memory block for the first chunk that satisfies the allocation size
        // this is just a first-fit strategy, which should be fine atm
        decltype(MemoryPool.ChunkSizeInBytesToFreeChunkMap)::iterator FreeChunkIterator = MemoryPool.ChunkSizeInBytesToFreeChunkMap.lower_bound(kAllocationSizeInBytes);

        // if there are no free chunks then report a fatal error
        if (FreeChunkIterator == MemoryPool.ChunkSizeInBytesToFreeChunkMap.cend())
        {
            /* FATAL ERROR: No space left in the memory block */
            bResult = false;
        }
        else
        {
            Private::MemoryPool::MemoryChunk & FreeChunk = FreeChunkIterator->second;

            uint64 const kAlignedMemoryOffset = (FreeChunk.OffsetInBytes + kMemoryRequirements.alignment - 1u) & ~(kMemoryRequirements.alignment - 1u);
            uint16 const kAlignmentOffsetInBytes = { static_cast<uint16>(kAlignedMemoryOffset - FreeChunk.OffsetInBytes) };

            // if the aligned offset is the same as the previous offset then we should reclaim the additional alignment bytes since they aren't needed

            // store the allocation and return a handle
            Private::MemoryPool::MemoryChunk & NewChunk = MemoryPool.AllocatedChunks.emplace_back(Private::MemoryPool::MemoryChunk { FreeChunk.OffsetInBytes, kAllocationSizeInBytes, 0u, kAlignmentOffsetInBytes });

            uint32 const kAllocationHandle = { static_cast<uint32>(MemoryPool.AllocatedChunks.size()) };
            MemoryPool.AllocationHandleRemappingTable.insert(std::make_pair(kAllocationHandle, kAllocationHandle - 1u));

            NewChunk.AllocationHandle = kAllocationHandle;

            // try to split the chunk if there is residual memory
            if (FreeChunk.SizeInBytes > kAllocationSizeInBytes)
            {
                Private::MemoryPool::MemoryChunk const kNewChunk =
                {
                    FreeChunk.OffsetInBytes + kAllocationSizeInBytes,
                    FreeChunk.SizeInBytes - kAllocationSizeInBytes,
                    0u, 0u
                };

                decltype(MemoryPool.ChunkSizeInBytesToFreeChunkMap)::iterator const NewChunkIterator = MemoryPool.ChunkSizeInBytesToFreeChunkMap.insert(std::make_pair(kNewChunk.SizeInBytes, kNewChunk));
                MemoryPool.ChunkOffsetToFreeChunkMap.insert(std::make_pair(kNewChunk.OffsetInBytes, NewChunkIterator));
            }

            MemoryPool.ChunkOffsetToFreeChunkMap.erase(FreeChunk.OffsetInBytes);
            MemoryPool.ChunkSizeInBytesToFreeChunkMap.erase(FreeChunkIterator);

            OutputAllocationHandle = { (static_cast<uint64>(MemoryTypeIndex) << 32u) | static_cast<uint64>(kAllocationHandle) };
        }
    }

    return bResult;
}

bool const Vulkan::Memory::Free(uint64 const kAllocationHandle)
{
    if (kAllocationHandle == 0u)
    {
        /* ERROR */
        return false;
    }

    uint32 const kMemoryTypeIndex = (kAllocationHandle & Private::HandleConstants::kMemoryTypeMask) >> 32u;
    uint32 const kAllocationHandleBits = (kAllocationHandle & Private::HandleConstants::kHandleMask);

    Private::MemoryPool & MemoryPool = DeviceMemoryPools [static_cast<uint8>(kMemoryTypeIndex)];

    uint32 const kAllocationIndex = MemoryPool.AllocationHandleRemappingTable [kAllocationHandleBits];

    Private::MemoryPool::MemoryChunk & Chunk = MemoryPool.AllocatedChunks [kAllocationIndex];

    decltype(MemoryPool.ChunkOffsetToFreeChunkMap)::iterator NextValidOffset = { MemoryPool.ChunkOffsetToFreeChunkMap.upper_bound(Chunk.OffsetInBytes - Chunk.AlignmentOffsetInBytes) };
    decltype(MemoryPool.ChunkOffsetToFreeChunkMap)::iterator PreviousValidOffset = { MemoryPool.ChunkOffsetToFreeChunkMap.end() };

    if (NextValidOffset != MemoryPool.ChunkOffsetToFreeChunkMap.cbegin())
    {
        // if it is equal to the end then there isn't 1 greater, but there is still possibly 1 less than
        PreviousValidOffset = NextValidOffset;
        PreviousValidOffset--;
    }

    if (PreviousValidOffset != MemoryPool.ChunkOffsetToFreeChunkMap.cend())
    {
        Private::MemoryPool::MemoryChunk const & PreviousChunk = PreviousValidOffset->second->second;
        uint64 const kPreviousChunkEndOffset = { PreviousChunk.OffsetInBytes + PreviousChunk.SizeInBytes };

        if (kPreviousChunkEndOffset == Chunk.OffsetInBytes)
        {
            // merge into chunk
            Chunk.OffsetInBytes = PreviousChunk.OffsetInBytes;
            Chunk.SizeInBytes += PreviousChunk.SizeInBytes;

            // remove from the maps
            MemoryPool.ChunkSizeInBytesToFreeChunkMap.erase(PreviousValidOffset->second);
            MemoryPool.ChunkOffsetToFreeChunkMap.erase(PreviousValidOffset);            
        }
    }

    if (NextValidOffset != MemoryPool.ChunkOffsetToFreeChunkMap.cend())
    {
        Private::MemoryPool::MemoryChunk const & NextChunk = NextValidOffset->second->second;
        uint64 const kChunkEndOffset = { Chunk.OffsetInBytes + Chunk.SizeInBytes };

        if (kChunkEndOffset == NextChunk.OffsetInBytes)
        {
            Chunk.SizeInBytes += NextChunk.SizeInBytes;

            // remove from map
            MemoryPool.ChunkSizeInBytesToFreeChunkMap.erase(NextValidOffset->second);
            MemoryPool.ChunkOffsetToFreeChunkMap.erase(NextValidOffset);
        }
    }

    // add the new chunk back into the free list
    decltype(MemoryPool.ChunkSizeInBytesToFreeChunkMap)::iterator const kNewChunkIterator = MemoryPool.ChunkSizeInBytesToFreeChunkMap.insert(std::make_pair(Chunk.SizeInBytes, Chunk));
    MemoryPool.ChunkOffsetToFreeChunkMap.insert(std::make_pair(Chunk.OffsetInBytes, kNewChunkIterator));

    // remove from allocated chunks
    MemoryPool.AllocationHandleRemappingTable.erase(kAllocationHandleBits);

    if (MemoryPool.AllocatedChunks.size() > 1u)
    {
        // remap the swapped chunk
        std::swap(MemoryPool.AllocatedChunks [kAllocationIndex], MemoryPool.AllocatedChunks.back());
        MemoryPool.AllocationHandleRemappingTable [MemoryPool.AllocatedChunks [kAllocationIndex].AllocationHandle] = kAllocationIndex;
    }

    MemoryPool.AllocatedChunks.pop_back();

    return true;
}

bool const Vulkan::Memory::GetAllocationInfo(uint64 const kAllocationHandle, Vulkan::Memory::AllocationInfo & OutputAllocationInfo)
{
    if (kAllocationHandle == 0u)
    {
        /* ERROR */
        return false;
    }

    uint8 const kMemoryTypeIndex = { static_cast<uint8>((kAllocationHandle & Private::HandleConstants::kMemoryTypeMask) >> 32u) };
    uint32 const kAllocationHandleBits = { (kAllocationHandle & Private::HandleConstants::kHandleMask) };

    Private::MemoryPool const & kMemoryPool = DeviceMemoryPools [kMemoryTypeIndex];

    /* need to do some additional checks for handle validity */
    uint32 const kAllocationIndex = kMemoryPool.AllocationHandleRemappingTable.at(kAllocationHandleBits);

    Private::MemoryPool::MemoryChunk const & kMemoryChunk = kMemoryPool.AllocatedChunks [kAllocationIndex];

    uint64 const kAllocationOffset = { kMemoryChunk.OffsetInBytes + kMemoryChunk.AlignmentOffsetInBytes };

    OutputAllocationInfo.DeviceMemory = kMemoryPool.DeviceMemory;
    OutputAllocationInfo.OffsetInBytes = kAllocationOffset;
    OutputAllocationInfo.SizeInBytes = kMemoryChunk.SizeInBytes;
    OutputAllocationInfo.MappedAddress = static_cast<void *>(static_cast<std::byte *>(kMemoryPool.DeviceMemoryStartAddress) + kAllocationOffset);

    return true;
}