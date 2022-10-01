#pragma once

#include "VulkanModule.hpp"

namespace Vulkan::Device
{
    struct DeviceState;
}

/*
*   Descriptor Set had bindings so lets go with a simple slot based binding system for handling descriptors.
*
*   Create and manage descriptor pools
*       - We want the ability to manage multiple pools
*       - We want to know how many of each descriptor we have left
*       - We want to know what descriptors are supported
*       - Create a new pool when running out of descriptors
*
*   Create and store metadata about descriptor set layouts
*       - This can be used to validate bound resources before making a draw call
*
*   Allocate descriptor sets from available pools
*       - VK_ERROR_OUT_OF_POOL_MEMORY => Allocate a new pool
*       - Keep a series of pools available for each buffered frame, then just reset them at the start of the frame
*       - When allocating use a first fit method of allocating to begin with
*
*   Below Should be handled by some global render state.
* 
*   Store resources currently bound to descriptor sets
*       - Set an initial limit on how many of each type can be bound, then use a simple array to hold the bindings
*
*   By storing the currently bound resources, we can batch descriptor writes and flush before draws
*
*   Bind to pipeline layout based on descriptor set index and binding slot index
*
*   Create pipeline layout using descriptor set layout handles
*/

namespace Vulkan::Descriptors
{
    enum DescriptorTypes
    {
        Uniform = 0x1,
        SampledImage = 0x2,
        Sampler = 0x4,
    };

    extern bool const CreateDescriptorAllocator(Vulkan::Device::DeviceState const & kDeviceState, uint32 const kDescriptorTypeFlags, uint16 & OutputAllocatorHandle);

    extern bool const DestroyDescriptorAllocator(Vulkan::Device::DeviceState const & kDeviceState, uint16 const kAllocatorHandle);

    extern bool const ResetDescriptorAllocator(Vulkan::Device::DeviceState const & kDeviceState, uint16 const kAllocatorHandle);

    extern bool const AllocateDescriptorSet(Vulkan::Device::DeviceState const & kDeviceState, uint16 const kAllocatorHandle, uint16 const kLayoutHandle, uint16 & OutputDescriptorSetHandle);

    extern bool const BindBufferDescriptors(uint16 const kAllocatorHandle, uint16 const kDescriptorSetHandle, VkDescriptorType const kDescriptorType, uint8 const kFirstBindingIndex, uint8 const kDescriptorCount, VkDescriptorBufferInfo const * const kBufferDescriptors);

    extern bool const BindImageDescriptors(uint16 const kAllocatorHandle, uint16 const kDescriptorSetHandle, VkDescriptorType const kDescriptorType, uint8 const kFirstBindingIndex, uint8 const kDescriptorCount, VkDescriptorImageInfo const * const kImageDescriptors);

    extern bool const FlushDescriptorWrites(Vulkan::Device::DeviceState const & kDeviceState);

    extern bool const CreateDescriptorSetLayout(Vulkan::Device::DeviceState const & kDeviceState, uint32 const kBindingCount, VkDescriptorSetLayoutBinding const * const kResourceBindings, uint16 & OutputLayoutHandle);

    extern bool const DestroyDescriptorSetLayout(Vulkan::Device::DeviceState const & kDeviceState, uint16 const kLayoutHandle);

    extern bool const GetDescriptorSetLayout(uint16 const kLayoutHandle, VkDescriptorSetLayout & OutputLayout);

    extern bool const GetDescriptorSet(uint16 const kAllocatorHandle, uint16 const kDescriptorSetHandle, VkDescriptorSet & OutputDescriptorSet);
}