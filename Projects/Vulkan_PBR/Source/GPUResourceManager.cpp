#include "GPUResourceManager.hpp"

#include "AssetManager.hpp"

#include "Graphics/Device.hpp"
#include "Memory.hpp"

#include <vector>
#include <array>
#include <unordered_map>
#include <queue>

GPUResourceManager::ResourceCollection GPUResourceManager::ResourceLists = {};
GPUResourceManager::ResourceHandleCollection GPUResourceManager::FreeResourceHandles = {};
GPUResourceManager::ResourceDeletionCollection GPUResourceManager::ResourceDeletionLists = {};

void GPUResourceManager::DestroyUnusedResources(Vulkan::Device::DeviceState const & DeviceState)
{
    constexpr size_t TupleSize = std::tuple_size<ResourceDeletionCollection>();
    DestroyUnusedResources(DeviceState, std::make_index_sequence<TupleSize>());
}