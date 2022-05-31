#pragma once

#include "Graphics/VulkanModule.hpp"

#include "Memory.hpp"

#include <vector>
#include <queue>
#include <unordered_map>

namespace Vulkan::Device
{
    struct DeviceState;
}

namespace GPUResource
{
    template<typename TResourceType>
    struct Resource
    {
        TResourceType VkResource;
        DeviceMemoryAllocator::Allocation MemoryAllocation;
    };

    using Buffer = Resource<VkBuffer>;
    using Image = Resource<VkImage>;
    using ImageView = Resource<VkImageView>;
}

/* Overengineered but compiler can do the work for me :) */
namespace GPUResourceManager
{
    template<class TResourceType>
    struct ResourceHandle
    {
        uint32 ResourceIndex;
    };

    template<class TResourceType>
    using ResourceList = std::vector<GPUResource::Resource<TResourceType>>;

    template<class TResourceType>
    using ResourceDeletionTable = std::unordered_map<VkFence, std::vector<ResourceHandle<TResourceType>>>;

    using ResourceCollection = std::tuple<
        ResourceList<VkBuffer>,
        ResourceList<VkImage>,
        ResourceList<VkFramebuffer>>;

    using ResourceHandleCollection = std::tuple<
        std::queue<ResourceHandle<VkBuffer>>,
        std::queue<ResourceHandle<VkImage>>,
        std::queue<ResourceHandle<VkFramebuffer>>>;

    using ResourceDeletionCollection = std::tuple<
        ResourceDeletionTable<VkBuffer>,
        ResourceDeletionTable<VkImage>,
        ResourceDeletionTable<VkFramebuffer>>;

    /* TODO: Keep a list of free indices and only push if that list is empty */

    using BufferHandle = ResourceHandle<VkBuffer>;
    using ImageHandle = ResourceHandle<VkImage>;
    using FrameBufferHandle = ResourceHandle<VkFramebuffer>;

    extern ResourceCollection ResourceLists;
    extern ResourceHandleCollection FreeResourceHandles;
    extern ResourceDeletionCollection ResourceDeletionLists;

    /* Maybe create instead of register? */
    template<class TResourceType>
    bool const RegisterResource(GPUResource::Resource<TResourceType> & Resource, ResourceHandle<TResourceType> & OutputResourceHandle)
    {
        ResourceList<TResourceType> & Resources = std::get<ResourceList<TResourceType>>(ResourceLists);

#pragma warning(push)
#pragma warning(disable: 4003)
        if (Resources.size() > std::numeric_limits<uint32>::max())
        {
            return false;
        }
#pragma warning(pop)

        std::queue<ResourceHandle<TResourceType>> & FreeHandleQueue = std::get<std::queue<ResourceHandle<TResourceType>>>(FreeResourceHandles);

        if (FreeHandleQueue.size() > 0u)
        {
            OutputResourceHandle = FreeHandleQueue.front();
            FreeHandleQueue.pop();

            Resources [OutputResourceHandle.ResourceIndex] = Resource;
        }
        else
        {
            Resources.push_back(Resource);
            OutputResourceHandle.ResourceIndex = static_cast<uint32>(Resources.size() - 1u);
        }

        return true;
    }

    template<class TResourceType>
    bool const GetResource(ResourceHandle<TResourceType> const ResourceHandle, GPUResource::Resource<TResourceType> & OutputResource)
    {
        bool bResult = false;

        ResourceList<TResourceType> const & Resources = std::get<ResourceList<TResourceType>>(ResourceLists);

        if (ResourceHandle.ResourceIndex < Resources.size())
        {
            OutputResource = Resources [ResourceHandle.ResourceIndex];
            bResult = true;
        }

        return bResult;
    }

    template<class TResourceType>
    bool const QueueResourceDeletion(VkFence Fence, ResourceHandle<TResourceType> const ResourceHandle)
    {
        ResourceDeletionTable<TResourceType> & DeletionQueue = std::get<ResourceDeletionTable<TResourceType>>(ResourceDeletionLists);

        DeletionQueue [Fence].push_back(ResourceHandle);

        return true;
    }

    template<class TResourceType>
    void DestroyResource(ResourceHandle<TResourceType> ResourceHandle, Vulkan::Device::DeviceState const & DeviceState)
    {
        GPUResource::Resource<TResourceType> Resource = {};
        GetResource(ResourceHandle, Resource);

        /* Discard the statements for different resource types */
        /* This bit doesn't benefit from the generic code, still less code to add a new resource type though */
        if constexpr (std::is_same<TResourceType, VkBuffer>())
        {
            if (Resource.VkResource != VK_NULL_HANDLE)
            {
                vkDestroyBuffer(DeviceState.Device, Resource.VkResource, nullptr);
                Resource.VkResource = VK_NULL_HANDLE;

                DeviceMemoryAllocator::FreeMemory(Resource.MemoryAllocation);

                std::queue<decltype(ResourceHandle)> & FreeHandleQueue = std::get<std::queue<decltype(ResourceHandle)>>(FreeResourceHandles);
                FreeHandleQueue.push(ResourceHandle);
            }
        }
    }

    /* Templated lambda functions might be cool for hiding this scary looking stuff */
    template<size_t CurrentIndex, size_t ... RemainingIndices>
    inline void DestroyUnusedResources(Vulkan::Device::DeviceState const & DeviceState, std::index_sequence<CurrentIndex, RemainingIndices ...>)
    {
        auto & DeletionTable = std::get<CurrentIndex>(ResourceDeletionLists);

        for (auto & DeletionEntry : DeletionTable)
        {
            VkResult FenceStatus = vkGetFenceStatus(DeviceState.Device, DeletionEntry.first);
            if (FenceStatus == VK_SUCCESS)
            {
                for (auto & Handle : DeletionEntry.second)
                {
                    DestroyResource(Handle, DeviceState);
                }

                DeletionEntry.second.clear();
            }
        }

        /* Recursively expand for each type of resource we need to cleanup */
        DestroyUnusedResources<RemainingIndices ...>(DeviceState, std::index_sequence<RemainingIndices ...>());
    }

    /* Base case in template recursion */
    template<size_t LastIndex = std::tuple_size<ResourceDeletionCollection>()>
    inline void DestroyUnusedResources(Vulkan::Device::DeviceState const & DeviceState, std::index_sequence<LastIndex>)
    {
        auto & DeletionTable = std::get<LastIndex>(ResourceDeletionLists);

        for (auto & DeletionEntry : DeletionTable)
        {
            VkResult FenceStatus = vkGetFenceStatus(DeviceState.Device, DeletionEntry.first);
            if (FenceStatus == VK_SUCCESS)
            {
                for (auto & Handle : DeletionEntry.second)
                {
                    DestroyResource(Handle, DeviceState);
                }

                DeletionEntry.second.clear();
            }
        }
    }

    void DestroyUnusedResources(Vulkan::Device::DeviceState const & DeviceState);
}