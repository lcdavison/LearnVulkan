#include "Assets/Texture.hpp"

#include <BMPLoader/BMPLoader.hpp>

#include "Graphics/Device.hpp"
#include "Graphics/Memory.hpp"

#include <vector>
#include <queue>
#include <unordered_set>
#include <unordered_map>

/* This is only designed for 2D textures atm */

struct TextureCollection
{
    std::vector<std::byte *> RawDatas = {};
    std::vector<uint32> WidthsInPixels = {};
    std::vector<uint32> HeightsInPixels = {};

    std::vector<uint32> ImageHandles = {};
    std::vector<uint32> ViewHandles = {};
};

static TextureCollection Textures = {};

/* Keep track of new texture handles for transfer to GPU */
static std::queue<uint32> NewTextureHandles = {};

static std::unordered_set<std::string> ImportedTextureSet = {};
static std::unordered_map<std::string, uint32> TextureNameToHandleMap = {};

static void CreateTextureResources(uint32 const TextureIndex, Vulkan::Device::DeviceState const & DeviceState)
{
    uint32 const WidthInPixels = { Textures.WidthsInPixels [TextureIndex] };
    uint32 const HeightInPixels = { Textures.HeightsInPixels [TextureIndex] };

    Vulkan::ImageDescriptor const TextureDesc =
    {
        VK_IMAGE_TYPE_2D,
        VK_FORMAT_R8G8B8A8_UNORM,
        WidthInPixels, HeightInPixels, 1u,
        1u, 1u,
        VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
        0u, false
    };

    Vulkan::Device::CreateImage(DeviceState, TextureDesc, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, Textures.ImageHandles [TextureIndex]);
}

static void TransferTextureDataToGPU(uint32 const TextureIndex, VkCommandBuffer CommandBuffer, Vulkan::Device::DeviceState const & DeviceState, VkFence const TransferFence)
{
    uint32 const WidthInPixels = Textures.WidthsInPixels [TextureIndex];
    uint32 const HeightInPixels = Textures.HeightsInPixels [TextureIndex];

    uint64 const ImageSizeInBytes = { 4u * WidthInPixels * HeightInPixels };

    uint32 StagingBufferHandle = {};
    Vulkan::Device::CreateBuffer(DeviceState, ImageSizeInBytes, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, StagingBufferHandle);

    Vulkan::Resource::Buffer StagingBuffer = {};
    Vulkan::Resource::GetBuffer(StagingBufferHandle, StagingBuffer);

    Vulkan::Memory::AllocationInfo AllocationInfo = {};
    Vulkan::Memory::GetAllocationInfo(StagingBuffer.MemoryAllocationHandle, AllocationInfo);

    ::memcpy_s(AllocationInfo.MappedAddress, AllocationInfo.SizeInBytes, Textures.RawDatas [TextureIndex], ImageSizeInBytes);

    Vulkan::Resource::Image Image = {};
    Vulkan::Resource::GetImage(Textures.ImageHandles [TextureIndex], Image);

    VkImageSubresourceRange const ImageRange =
    {
        VK_IMAGE_ASPECT_COLOR_BIT,
        0u, 1u,
        0u, 1u,
    };

    VkImageMemoryBarrier ImageBarrier = Vulkan::ImageMemoryBarrier(Image.Resource, VK_ACCESS_NONE, VK_ACCESS_NONE, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, ImageRange);
    vkCmdPipelineBarrier(CommandBuffer,
                         VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                         VK_DEPENDENCY_BY_REGION_BIT,
                         0u, nullptr,
                         0u, nullptr,
                         1u, &ImageBarrier);

    VkBufferImageCopy const CopyRegion =
    {
        0u, 0u, 0u, { VK_IMAGE_ASPECT_COLOR_BIT, 0u, 0u, 1u }, { 0u, 0u, 0u },{ WidthInPixels, HeightInPixels, 1u },
    };

    vkCmdCopyBufferToImage(CommandBuffer, StagingBuffer.Resource, Image.Resource, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1u, &CopyRegion);

    ImageBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    ImageBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    ImageBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    ImageBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    vkCmdPipelineBarrier(CommandBuffer,
                         VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                         VK_DEPENDENCY_BY_REGION_BIT,
                         0u, nullptr,
                         0u, nullptr,
                         1u, &ImageBarrier);

    Vulkan::Device::DestroyBuffer(DeviceState, StagingBufferHandle, TransferFence);
}

bool const Assets::Texture::ImportTexture(std::filesystem::path const & FilePath, std::string AssetName, uint32 & OutputAssetHandle)
{
    bool bResult = false;

    /* Only support .bmp atm */
    if (FilePath.extension() == ".bmp")
    {
        std::string PathString = FilePath.string();

        auto FoundPath = ImportedTextureSet.find(PathString);
        if (FoundPath == ImportedTextureSet.cend())
        {
            BMPLoader::BMPImageData ImageData = {};
            bResult = BMPLoader::LoadFile(FilePath, ImageData);

            if (bResult)
            {
                Textures.RawDatas.push_back(ImageData.RawData);
                Textures.WidthsInPixels.push_back(ImageData.WidthInPixels);
                Textures.HeightsInPixels.push_back(ImageData.HeightInPixels);
                Textures.ImageHandles.emplace_back();
                Textures.ViewHandles.emplace_back();

                OutputAssetHandle = static_cast<uint32>(Textures.RawDatas.size());

                TextureNameToHandleMap [std::move(AssetName)] = OutputAssetHandle;
                
                ImportedTextureSet.emplace(std::move(PathString));

                NewTextureHandles.push(OutputAssetHandle);
            }
        }
    }

    return bResult;
}

bool const Assets::Texture::FindTexture(std::string const & AssetName, uint32 & OutputAssetHandle)
{
    auto FoundAsset = TextureNameToHandleMap.find(AssetName);
    if (FoundAsset == TextureNameToHandleMap.cend())
    {
        /* ERROR */
        return false;
    }

    OutputAssetHandle = FoundAsset->second;

    return true;
}

bool const Assets::Texture::GetTextureData(uint32 const AssetHandle, Assets::Texture::TextureData & OutputTextureData)
{
    if (AssetHandle == 0u)
    {
        /* ERROR */
        return false;
    }

    uint32 const TextureIndex = { AssetHandle - 1u };

    OutputTextureData.Data = Textures.RawDatas [TextureIndex];
    OutputTextureData.WidthInPixels = Textures.WidthsInPixels [TextureIndex];
    OutputTextureData.HeightInPixels = Textures.HeightsInPixels [TextureIndex];
    OutputTextureData.ImageHandle = Textures.ImageHandles [TextureIndex];
    OutputTextureData.ViewHandle = Textures.ViewHandles [TextureIndex];

    return true;
}

bool const Assets::Texture::InitialiseGPUResources(VkCommandBuffer CommandBuffer, Vulkan::Device::DeviceState const & DeviceState, VkFence const TransferFence)
{
    while (NewTextureHandles.size() > 0u)
    {
        uint32 const TextureIndex = NewTextureHandles.front() - 1u;
        NewTextureHandles.pop();

        ::CreateTextureResources(TextureIndex, DeviceState);
        ::TransferTextureDataToGPU(TextureIndex, CommandBuffer, DeviceState, TransferFence);

        Vulkan::ImageViewDescriptor const ViewDesc =
        {
            VK_IMAGE_VIEW_TYPE_2D,
            VK_FORMAT_R8G8B8A8_UNORM,
            VK_IMAGE_ASPECT_COLOR_BIT,
            0u, 1u,
            0u, 1u,
            true,
        };

        Vulkan::Device::CreateImageView(DeviceState, Textures.ImageHandles [TextureIndex], ViewDesc, Textures.ViewHandles [TextureIndex]);
    }

    return true;
}