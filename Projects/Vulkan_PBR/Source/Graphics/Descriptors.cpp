#include "Graphics/Descriptors.hpp"

#include "Graphics/Device.hpp"

#include <array>
#include <memory>
#include <queue>

struct LayoutBinding
{
    VkDescriptorType Type = {};
    uint32 BindingIndex = {};
};

struct DescriptorLayouts
{
    std::queue<uint16> FreeLayoutHandles = {};

    std::vector<VkDescriptorSetLayout> Layouts = {};
    std::vector<std::unique_ptr<LayoutBinding[]>> Bindings = {};  // atm dynamically allocate them, later we can write an allocator for this system to handle bindings more effectively
    std::vector<uint32> BindingCounts = {};
};

struct DescriptorPoolCollection
{
    std::vector<VkDescriptorPool> Pools = {};
    std::vector<VkDescriptorSet> DescriptorSets = {};
    std::vector<uint64> AvailableDescriptorSetMasks = {};
};

struct DescriptorCache
{
    std::vector<uint16> AllocatorHandles = {};
    std::vector<uint16> DescriptorSetHandles = {};
    std::vector<VkDescriptorType> DescriptorTypes = {};
    std::vector<uint8> DescriptorCounts = {};
    std::vector<uint8> FirstBindingIndices = {};
    std::vector<uint8> FirstDescriptorOffset = {};

    std::vector<VkDescriptorBufferInfo> BufferDescriptors = {};
    std::vector<VkDescriptorImageInfo> ImageDescriptors = {};
};

static uint16 const kMaximumDescriptorAllocatorCount = { 6u };
static uint8 const kMaximumDescriptorSetCount = { 64u };
static uint8 const kDefaultPoolDescriptorCount = { 16u };

static uint16 NextAvailableAllocatorIndex = {};
static std::array<DescriptorPoolCollection, kMaximumDescriptorAllocatorCount> DescriptorAllocators = {};
static std::array<uint32, kMaximumDescriptorAllocatorCount> DescriptorTypeMasks = {};

static DescriptorLayouts DescriptorSetLayouts = {};
static DescriptorCache Cache = {};

bool const Vulkan::Descriptors::CreateDescriptorAllocator(Vulkan::Device::DeviceState const & kDeviceState, uint32 const kDescriptorTypeFlags, uint16 & OutputAllocatorHandle)
{
    uint16 const kAllocatorHandle = ++NextAvailableAllocatorIndex;

    std::vector<VkDescriptorPoolSize> PoolSizes = {};

    uint32 DescriptorType = {};
    uint32 DescriptorTypeMask = { kDescriptorTypeFlags };
    while (::_BitScanForward(reinterpret_cast<unsigned long *>(&DescriptorType), DescriptorTypeMask))
    {
        DescriptorTypeMask ^= 1u << DescriptorType;

        VkDescriptorPoolSize & PoolSize = PoolSizes.emplace_back();
        PoolSize.descriptorCount = kDefaultPoolDescriptorCount;

        switch (DescriptorType)
        {
            case Vulkan::Descriptors::DescriptorTypes::Uniform:
                PoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                break;
            case Vulkan::Descriptors::DescriptorTypes::Sampler:
                PoolSize.type = VK_DESCRIPTOR_TYPE_SAMPLER;
                break;
            case Vulkan::Descriptors::DescriptorTypes::SampledImage:
                PoolSize.type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
                break;
        }
    }

    VkDescriptorPoolCreateInfo const kPoolCreateInfo = 
        VkDescriptorPoolCreateInfo 
    { 
        VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO, 
        nullptr, 
        0u, 
        kMaximumDescriptorSetCount, 
        static_cast<uint32>(PoolSizes.size()), 
        PoolSizes.data() 
    };

    uint16 const kAllocatorIndex = { kAllocatorHandle - 1u };

    DescriptorPoolCollection & Allocator = DescriptorAllocators [kAllocatorIndex];
    Allocator.Pools.emplace_back();
    Allocator.AvailableDescriptorSetMasks.push_back(std::numeric_limits<uint64>::max());
    Allocator.DescriptorSets.resize(Allocator.DescriptorSets.size() + kMaximumDescriptorSetCount);

    DescriptorTypeMasks [kAllocatorIndex] = kDescriptorTypeFlags;

    VERIFY_VKRESULT(vkCreateDescriptorPool(kDeviceState.Device, &kPoolCreateInfo, nullptr, &Allocator.Pools.back()));

    OutputAllocatorHandle = { kAllocatorHandle };

    return true;
}

bool const Vulkan::Descriptors::ResetDescriptorAllocator(Vulkan::Device::DeviceState const & kDeviceState, uint16 const kAllocatorHandle)
{
    if (kAllocatorHandle == 0u)
    {
        /* ERROR */
        return false;
    }

    /* TODO: This will need to iterate the descriptor pools when we allocate additional */

    DescriptorPoolCollection & Allocator = DescriptorAllocators [kAllocatorHandle - 1u];
    VERIFY_VKRESULT(vkResetDescriptorPool(kDeviceState.Device, Allocator.Pools.back(), 0u));

    Allocator.AvailableDescriptorSetMasks [0u] = std::numeric_limits<uint64>::max();

    return true;
}

bool const Vulkan::Descriptors::AllocateDescriptorSet(Vulkan::Device::DeviceState const & kDeviceState, uint16 const kAllocatorHandle, uint16 const kLayoutHandle, uint16 & OutputDescriptorSetHandle)
{
    if (kAllocatorHandle == 0u || kLayoutHandle == 0u)
    {
        return false;
    }

    uint32 const kAllocatorIndex = { kAllocatorHandle - 1u };
    uint32 const kLayoutIndex = { kLayoutHandle - 1u };

    DescriptorPoolCollection & Allocator = DescriptorAllocators [kAllocatorIndex];
    VkDescriptorSetLayout const kLayout = DescriptorSetLayouts.Layouts [kLayoutIndex];

    VkDescriptorSetAllocateInfo const kAllocationInfo =
    {
        VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        nullptr,
        Allocator.Pools.back(),
        1u,
        &kLayout
    };

    /* TODO: Prior to this we need to find an available pool */
    /* TODO: Bundle descriptor set allocator handle and descriptor set handle into a single uint32 */

    uint64 const kDescriptorSetMask = { Allocator.AvailableDescriptorSetMasks [0u] };
    uint32 FirstFreeIndex = {};
    _BitScanForward64(reinterpret_cast<unsigned long*>(&FirstFreeIndex), kDescriptorSetMask);

    VERIFY_VKRESULT(vkAllocateDescriptorSets(kDeviceState.Device, &kAllocationInfo, &Allocator.DescriptorSets [FirstFreeIndex]));

    Allocator.AvailableDescriptorSetMasks [0u] ^= 1ull << FirstFreeIndex;

    OutputDescriptorSetHandle = { static_cast<uint16>(FirstFreeIndex + 1u) };

    return true;
}

bool const Vulkan::Descriptors::BindBufferDescriptors(uint16 const kAllocatorHandle, uint16 const kDescriptorSetHandle, VkDescriptorType const kDescriptorType, uint8 const kFirstBindingIndex, uint8 const kDescriptorCount, VkDescriptorBufferInfo const * const kBufferDescriptors)
{
    if (kAllocatorHandle == 0u)
    {
        Logging::Log(Logging::LogTypes::Error, PBR_TEXT("Cannot retrieve descriptor set from a NULL allocator."));
        return false;
    }

    if (kDescriptorSetHandle == 0u)
    {
        Logging::Log(Logging::LogTypes::Error, PBR_TEXT("Cannot bind buffer descriptors to NULL descriptor set."));
        return false;
    }

    if (kDescriptorCount == 0u)
    {
        Logging::Log(Logging::LogTypes::Error, PBR_TEXT("Attempting to bind 0 buffer descriptors."));
        return false;
    }

    PBR_ASSERT((Cache.BufferDescriptors.size() & ~0xFF) == 0u);

    uint8 const kFirstDescriptorIndex = { static_cast<uint8>(Cache.BufferDescriptors.size()) };

    /* Not the greatest, but we need to make sure the descriptor info is kept valid until we flush the descriptors */
    for (uint8 DescriptorIndex = {};
         DescriptorIndex < kDescriptorCount;
         DescriptorIndex++)
    {
        Cache.BufferDescriptors.push_back(kBufferDescriptors [DescriptorIndex]);
    }

    Cache.AllocatorHandles.push_back(kAllocatorHandle);
    Cache.DescriptorSetHandles.push_back(kDescriptorSetHandle);
    Cache.DescriptorTypes.push_back(kDescriptorType);
    Cache.DescriptorCounts.push_back(kDescriptorCount);
    Cache.FirstBindingIndices.push_back(kFirstBindingIndex);
    Cache.FirstDescriptorOffset.push_back(kFirstDescriptorIndex);
    
    return true;
}

bool const Vulkan::Descriptors::BindImageDescriptors(uint16 const kAllocatorHandle, uint16 const kDescriptorSetHandle, VkDescriptorType const kDescriptorType, uint8 const kFirstBindingIndex, uint8 const kDescriptorCount, VkDescriptorImageInfo const * const kImageDescriptors)
{
    if (kAllocatorHandle == 0u)
    {
        Logging::Log(Logging::LogTypes::Error, PBR_TEXT("Cannot retrieve descriptor set from a NULL allocator."));
        return false;
    }

    if (kDescriptorSetHandle == 0u)
    {
        Logging::Log(Logging::LogTypes::Error, PBR_TEXT("Cannot bind image descriptors to NULL descriptor set."));
        return false;
    }

    if (kDescriptorCount == 0u)
    {
        Logging::Log(Logging::LogTypes::Error, PBR_TEXT("Attempting to bind 0 image descriptors."));
        return false;
    }

    PBR_ASSERT((Cache.ImageDescriptors.size() & ~0xFF) == 0u);

    uint8 const kFirstDescriptorIndex = { static_cast<uint8>(Cache.ImageDescriptors.size()) };

    for (uint8 DescriptorIndex = {};
         DescriptorIndex < kDescriptorCount;
         DescriptorIndex++)
    {
        Cache.ImageDescriptors.push_back(kImageDescriptors [DescriptorIndex]);
    }

    Cache.AllocatorHandles.push_back(kAllocatorHandle);
    Cache.DescriptorSetHandles.push_back(kDescriptorSetHandle);
    Cache.DescriptorTypes.push_back(kDescriptorType);
    Cache.DescriptorCounts.push_back(kDescriptorCount);
    Cache.FirstBindingIndices.push_back(kFirstBindingIndex);
    Cache.FirstDescriptorOffset.push_back(kFirstDescriptorIndex);

    return true;
}

bool const Vulkan::Descriptors::FlushDescriptorWrites(Vulkan::Device::DeviceState const & kDeviceState)
{
    if (Cache.DescriptorSetHandles.size() == 0u)
    {
        return false;
    }

    /* The cache won't get resized here, so run through and create the vkwritedescriptorsets here */

    std::vector DescriptorWrites = std::vector<VkWriteDescriptorSet>(Cache.FirstBindingIndices.size());

    for (uint16 DescriptorIndex = {};
         DescriptorIndex < DescriptorWrites.size();
         DescriptorIndex++)
    {
        DescriptorPoolCollection const & kAllocator = DescriptorAllocators [Cache.AllocatorHandles [DescriptorIndex] - 1u];

        VkWriteDescriptorSet & DescriptorWriteInfo = DescriptorWrites [DescriptorIndex];
        DescriptorWriteInfo = Vulkan::WriteDescriptorSet(kAllocator.DescriptorSets [Cache.DescriptorSetHandles [DescriptorIndex] - 1u],
                                                         Cache.DescriptorTypes [DescriptorIndex],
                                                         Cache.FirstBindingIndices [DescriptorIndex],
                                                         Cache.DescriptorCounts [DescriptorIndex],
                                                         nullptr, nullptr);

        switch (Cache.DescriptorTypes [DescriptorIndex])
        {
            case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
            {
                DescriptorWriteInfo.pBufferInfo = (&Cache.BufferDescriptors [Cache.FirstDescriptorOffset [DescriptorIndex]]);
            }
            break;
            case VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT:
            case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
            case VK_DESCRIPTOR_TYPE_SAMPLER:
            {
                DescriptorWriteInfo.pImageInfo = (&Cache.ImageDescriptors [Cache.FirstDescriptorOffset [DescriptorIndex]]);
            }
            break;
            default:
            {
                // Unknown encountered descriptor type
                return false;
            }
        }
    }

    vkUpdateDescriptorSets(kDeviceState.Device, static_cast<uint32>(DescriptorWrites.size()), DescriptorWrites.data(), 0u, nullptr);

    Cache.AllocatorHandles.clear();
    Cache.DescriptorSetHandles.clear();
    Cache.DescriptorTypes.clear();
    Cache.DescriptorCounts.clear();
    Cache.FirstBindingIndices.clear();
    Cache.FirstDescriptorOffset.clear();

    Cache.BufferDescriptors.clear();
    Cache.ImageDescriptors.clear();

    return true;
}

bool const Vulkan::Descriptors::CreateDescriptorSetLayout(Vulkan::Device::DeviceState const & kDeviceState, uint32 const kBindingCount, VkDescriptorSetLayoutBinding const * const kResourceBindings, uint16 & OutputLayoutHandle)
{
    if (kBindingCount == 0u)
    {
        /* ERROR */
        return false;
    }

    uint16 LayoutHandle = {};

    if (DescriptorSetLayouts.FreeLayoutHandles.size() == 0u)
    {
        DescriptorSetLayouts.Layouts.emplace_back();
        DescriptorSetLayouts.Bindings.emplace_back();
        DescriptorSetLayouts.BindingCounts.emplace_back();

        LayoutHandle = { static_cast<uint16>(DescriptorSetLayouts.Layouts.size()) };
    }
    else
    {
        LayoutHandle = { DescriptorSetLayouts.FreeLayoutHandles.front() };
        DescriptorSetLayouts.FreeLayoutHandles.pop();
    }

    uint16 const kLayoutIndex = { LayoutHandle - 1u };

    VkDescriptorSetLayoutCreateInfo const CreateInfo = Vulkan::DescriptorSetLayout(kBindingCount, kResourceBindings);
    VERIFY_VKRESULT(vkCreateDescriptorSetLayout(kDeviceState.Device, &CreateInfo, nullptr, &DescriptorSetLayouts.Layouts [kLayoutIndex]));

    DescriptorSetLayouts.Bindings [kLayoutIndex] = std::move(std::make_unique<LayoutBinding[]>(kBindingCount));
    DescriptorSetLayouts.BindingCounts [kLayoutIndex] = kBindingCount;

    LayoutBinding * const NewBindings = DescriptorSetLayouts.Bindings [kLayoutIndex].get();

    for (uint32 BindingIndex = {};
         BindingIndex < kBindingCount;
         BindingIndex++)
    {
        VkDescriptorSetLayoutBinding const & Binding = kResourceBindings [BindingIndex];
        NewBindings [BindingIndex] = LayoutBinding { Binding.descriptorType, Binding.binding };
    }

    OutputLayoutHandle = LayoutHandle;

    return true;
}

bool const Vulkan::Descriptors::DestroyDescriptorSetLayout(Vulkan::Device::DeviceState const & kDeviceState, uint16 const kLayoutHandle)
{
    if (kLayoutHandle == 0u)
    {
        Logging::Log(Logging::LogTypes::Error, PBR_TEXT("Cannot destroy descriptor set layout for NULL handle."));
        return false;
    }

    uint16 const LayoutIndex = kLayoutHandle - 1u;

    VkDescriptorSetLayout const Layout = DescriptorSetLayouts.Layouts [LayoutIndex];
    vkDestroyDescriptorSetLayout(kDeviceState.Device, Layout, nullptr);

    DescriptorSetLayouts.Bindings [LayoutIndex].reset();
    DescriptorSetLayouts.BindingCounts [LayoutIndex] = 0u;

    DescriptorSetLayouts.FreeLayoutHandles.push(kLayoutHandle);

    return true;
}

bool const Vulkan::Descriptors::GetDescriptorSetLayout(uint16 const kLayoutHandle, VkDescriptorSetLayout & OutputLayout)
{
    if (kLayoutHandle == 0u)
    {
        /* ERROR */
        return false;
    }

    OutputLayout = DescriptorSetLayouts.Layouts [kLayoutHandle - 1u];

    return true;
}

bool const Vulkan::Descriptors::GetDescriptorSet(uint16 const kDescriptorAllocator, uint16 const kDescriptorSetHandle, VkDescriptorSet & OutputDescriptorSet)
{
    if (kDescriptorAllocator == 0u)
    {
        /* ERROR */
        return false;
    }

    if (kDescriptorSetHandle == 0u)
    {
        /* ERROR */
        return false;
    }

    OutputDescriptorSet = DescriptorAllocators [kDescriptorAllocator - 1u].DescriptorSets [kDescriptorSetHandle - 1u];

    return true;
}