#include "Graphics/Descriptors.hpp"

#include "Graphics/Device.hpp"

#include <memory>
#include <queue>

struct LayoutBinding
{
    VkDescriptorType Type = {};
    uint32 BindingIndex = {};
};

struct DescriptorAllocator
{
    VkDescriptorPool Pool = {};
    Vulkan::Descriptors::DescriptorTypes DescriptorTypeMask = {};
};

struct DescriptorLayouts
{
    std::queue<uint16> FreeLayoutHandles = {};

    std::vector<VkDescriptorSetLayout> Layouts = {};
    std::vector<std::unique_ptr<LayoutBinding[]>> Bindings = {};  // atm dynamically allocate them, later we can write an allocator for this system to handle bindings more effectively
    std::vector<uint32> BindingCounts = {};
};

static uint8 const kMaximumDescriptorSetCount = { 64u };
static uint8 const kDefaultPoolDescriptorCount = { 16u };

static DescriptorLayouts DescriptorSetLayouts = {};
static std::vector<DescriptorAllocator> DescriptorAllocators = {};

bool const Vulkan::Descriptors::CreateDescriptorAllocator(Vulkan::Device::DeviceState const & DeviceState, uint32 const DescriptorTypeFlags, uint16 & OutputAllocatorHandle)
{
    DescriptorAllocator Allocator = DescriptorAllocator {};

    std::vector<VkDescriptorPoolSize> PoolSizes = {};

    uint32 DescriptorType = {};
    uint32 DescriptorTypeMask = { DescriptorTypeFlags };
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

    VkDescriptorPoolCreateInfo const CreateInfo = 
        VkDescriptorPoolCreateInfo 
    { 
        VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO, 
        nullptr, 
        0u, 
        kMaximumDescriptorSetCount, 
        static_cast<uint32>(PoolSizes.size()), 
        PoolSizes.data() 
    };

    VERIFY_VKRESULT(vkCreateDescriptorPool(DeviceState.Device, &CreateInfo, nullptr, &Allocator.Pool));

    DescriptorAllocators.emplace_back(std::move(Allocator));
    OutputAllocatorHandle = { static_cast<uint16>(DescriptorAllocators.size()) };

    return true;
}

bool const Vulkan::Descriptors::ResetDescriptorAllocator(Vulkan::Device::DeviceState const & DeviceState, uint16 const AllocatorHandle)
{
    if (AllocatorHandle == 0u)
    {
        /* ERROR */
        return false;
    }

    DescriptorAllocator & Allocator = DescriptorAllocators [AllocatorHandle - 1u];
    VERIFY_VKRESULT(vkResetDescriptorPool(DeviceState.Device, Allocator.Pool, 0u));

    return true;
}

bool const Vulkan::Descriptors::AllocateDescriptorSet(Vulkan::Device::DeviceState const & DeviceState, uint16 const AllocatorHandle, uint16 const LayoutHandle, VkDescriptorSet & OutputDescriptorSet)
{
    if (AllocatorHandle == 0u || LayoutHandle == 0u)
    {
        /* ERROR */
        return false;
    }

    DescriptorAllocator & Allocator = DescriptorAllocators [AllocatorHandle - 1u];
    VkDescriptorSetLayout const Layout = DescriptorSetLayouts.Layouts [LayoutHandle - 1u];

    VkDescriptorSetAllocateInfo const AllocateInfo = VkDescriptorSetAllocateInfo
    {
        VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        nullptr,
        Allocator.Pool,
        1u,
        &Layout
    };

    VkResult Result = vkAllocateDescriptorSets(DeviceState.Device, &AllocateInfo, &OutputDescriptorSet);

    /* TODO: On allocation failure try and create a new descriptor pool to allocate from */
    PBR_ASSERT(Result == VK_SUCCESS);

    return true;
}

bool const Vulkan::Descriptors::CreateDescriptorSetLayout(Vulkan::Device::DeviceState const & DeviceState, uint32 const BindingCount, VkDescriptorSetLayoutBinding const * const ResourceBindings, uint16 & OutputLayoutHandle)
{
    if (BindingCount == 0u)
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

    VkDescriptorSetLayoutCreateInfo const CreateInfo = Vulkan::DescriptorSetLayout(BindingCount, ResourceBindings);
    VERIFY_VKRESULT(vkCreateDescriptorSetLayout(DeviceState.Device, &CreateInfo, nullptr, &DescriptorSetLayouts.Layouts [kLayoutIndex]));

    DescriptorSetLayouts.Bindings [kLayoutIndex] = std::move(std::make_unique<LayoutBinding[]>(BindingCount));
    DescriptorSetLayouts.BindingCounts [kLayoutIndex] = BindingCount;

    LayoutBinding * const NewBindings = DescriptorSetLayouts.Bindings [kLayoutIndex].get();

    for (uint32 BindingIndex = {};
         BindingIndex < BindingCount;
         BindingIndex++)
    {
        VkDescriptorSetLayoutBinding const & Binding = ResourceBindings [BindingIndex];
        NewBindings [BindingIndex] = LayoutBinding { Binding.descriptorType, Binding.binding };
    }

    OutputLayoutHandle = LayoutHandle;

    return true;
}

bool const Vulkan::Descriptors::DestroyDescriptorSetLayout(Vulkan::Device::DeviceState const & DeviceState, uint16 const LayoutHandle)
{
    if (LayoutHandle == 0u)
    {
        Logging::Log(Logging::LogTypes::Error, PBR_TEXT("Cannot destroy descriptor set layout for NULL handle."));
        return false;
    }

    uint16 const LayoutIndex = LayoutHandle - 1u;

    VkDescriptorSetLayout const Layout = DescriptorSetLayouts.Layouts [LayoutIndex];
    vkDestroyDescriptorSetLayout(DeviceState.Device, Layout, nullptr);

    DescriptorSetLayouts.Bindings [LayoutIndex].reset();
    DescriptorSetLayouts.BindingCounts [LayoutIndex] = 0u;

    DescriptorSetLayouts.FreeLayoutHandles.push(LayoutHandle);

    return true;
}

bool const Vulkan::Descriptors::GetDescriptorSetLayout(uint16 const LayoutHandle, VkDescriptorSetLayout & OutputLayout)
{
    if (LayoutHandle == 0u)
    {
        /* ERROR */
        return false;
    }

    OutputLayout = DescriptorSetLayouts.Layouts [LayoutHandle - 1u];

    return true;
}