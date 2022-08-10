#pragma once

#include "VulkanModule.hpp"

namespace Vulkan::Device
{
    struct DeviceState;
}
namespace Vulkan::Descriptors
{
    enum DescriptorTypes
    {
        Uniform = 0x1,
        SampledImage = 0x2,
        Sampler = 0x4,
    };

    extern bool const CreateDescriptorAllocator(Vulkan::Device::DeviceState const & DeviceState, uint32 const DescriptorTypeFlags, uint16 & OutputAllocatorHandle);

    extern bool const DestroyDescriptorAllocator(Vulkan::Device::DeviceState const & DeviceState, uint16 const AllocatorHandle);

    extern bool const ResetDescriptorAllocator(Vulkan::Device::DeviceState const & DeviceState, uint16 const AllocatorHandle);

    extern bool const AllocateDescriptorSet(Vulkan::Device::DeviceState const & DeviceState, uint16 const AllocatorHandle, uint16 const LayoutHandle, VkDescriptorSet & OutputDescriptorSet);

    extern bool const CreateDescriptorSetLayout(Vulkan::Device::DeviceState const & DeviceState, uint32 const BindingCount, VkDescriptorSetLayoutBinding const * const ResourceBindings, uint16 & OutputLayoutHandle);

    extern bool const DestroyDescriptorSetLayout(Vulkan::Device::DeviceState const & DeviceState, uint16 const LayoutHandle);

    extern bool const GetDescriptorSetLayout(uint16 const LayoutHandle, VkDescriptorSetLayout & OutputLayout);
}