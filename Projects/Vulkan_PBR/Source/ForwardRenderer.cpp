#include "ForwardRenderer.hpp"

#include "Graphics/VulkanModule.hpp"

#include "Graphics/Instance.hpp"
#include "Graphics/Device.hpp"
#include "Graphics/Viewport.hpp"
#include "Graphics/ShaderLibrary.hpp"
#include "Graphics/Descriptors.hpp"

#include "Components/StaticMeshComponent.hpp"
#include "Components/TransformComponent.hpp"
#include "Components/MaterialComponent.hpp"

#include "Assets/StaticMesh.hpp"
#include "Assets/Texture.hpp"
#include "Assets/Material.hpp"

#include "Graphics/Memory.hpp"
#include "Graphics/Allocators.hpp"

#include <Math/Matrix.hpp>
#include <Math/Transform.hpp>
#include <Math/Utilities.hpp>

#include "VulkanPBR.hpp"

#include "Scene.hpp"

#include <array>

#include "Camera.hpp"

struct PerFrameUniformBufferData
{
    Math::Matrix4x4 WorldToViewMatrix;
    Math::Matrix4x4 ViewToClipMatrix;
};

struct PerDrawUniformBufferData
{
    Math::Matrix4x4 ModelToWorldMatrix;
};

struct FrameStateCollection
{
    std::vector<LinearBufferAllocator::AllocatorState> LinearAllocators = {};

    std::vector<VkCommandBuffer> CommandBuffers = {};

    std::vector<VkSemaphore> Semaphores = {};
    std::vector<VkFence> Fences = {};

    std::vector<VkDescriptorSet> DescriptorSets = {};

    std::vector<uint32> FrameBuffers = {};

    std::vector<uint32> SceneColourImages = {};
    std::vector<uint32> SceneColourImageViews = {};

    std::vector<uint32> DepthStencilImages = {};
    std::vector<uint32> DepthStencilImageViews = {};

    std::vector<uint16> DescriptorAllocators = {};

    uint32 CurrentImageIndex = {};

    uint8 CurrentFrameStateIndex = {};
};

static std::string const kDefaultShaderEntryPointName = "main";
static uint8 const kFrameStateCount = { 2u };

static Vulkan::Instance::InstanceState InstanceState = {};
static Vulkan::Device::DeviceState DeviceState = {};
static Vulkan::Viewport::ViewportState ViewportState = {};
static FrameStateCollection FrameState = {};

static VkRenderPass MainRenderPass = {};

static VkPipelineLayout RenderPipelineLayout = {};
static VkPipeline RenderPipeline = {};

static VkPipelineLayout OutputPipelineLayout = {};
static VkPipeline OutputPipelineState = {};

static uint16 ProjectVerticesVSHandle = {};
static uint16 TorranceSparrowFSHandle = {};

static uint16 FullScreenTriangleVSHandle = {};
static uint16 PostProcessFSHandle = {};

static VkShaderModule ProjectVerticesVSShaderModule = {};
static VkShaderModule TorranceSparrowFSShaderModule = {};

static VkShaderModule FullScreenTriangleVSShaderModule = {};
static VkShaderModule PostProcessFSShaderModule = {};

static uint16 RenderDescriptorSetLayoutHandle = {};
static uint16 OutputDescriptorSetLayoutHandle = {};

static std::array<uint32, 4u> TextureHandles = {};

static VkSampler LinearImageSampler = {};
static VkSampler AnisotropicImageSampler = {};

/*
    For reference.
    World Space is:

  z |  / y
    | /
    .____ x

    View Space is:
      / z
     /
    ._____ x
    |
    | y
*/

static void CreateFrameState()
{
    FrameState.CurrentFrameStateIndex = 0u;

    FrameState.CommandBuffers.resize(kFrameStateCount); // Pre-Render Command Buffer and Render Command Buffer
    FrameState.FrameBuffers.resize(kFrameStateCount);
    FrameState.SceneColourImages.resize(kFrameStateCount);
    FrameState.SceneColourImageViews.resize(kFrameStateCount);
    FrameState.DepthStencilImages.resize(kFrameStateCount);
    FrameState.DepthStencilImageViews.resize(kFrameStateCount);
    FrameState.Semaphores.resize(kFrameStateCount * 2u);    // Each frame uses 2 semaphores
    FrameState.Fences.resize(kFrameStateCount);
    FrameState.DescriptorSets.resize(kFrameStateCount * 2u); // Each pipeline uses a different set
    FrameState.LinearAllocators.resize(kFrameStateCount);
    FrameState.DescriptorAllocators.resize(kFrameStateCount);

    Vulkan::Device::CreateCommandBuffers(DeviceState, VK_COMMAND_BUFFER_LEVEL_PRIMARY, kFrameStateCount, FrameState.CommandBuffers);

    for (uint8 CurrentImageIndex = {};
         CurrentImageIndex < kFrameStateCount;
         CurrentImageIndex++)
    {
        Vulkan::ImageDescriptor ImageDescriptor =
        {
            VK_IMAGE_TYPE_2D,
            VK_FORMAT_R16G16B16A16_SFLOAT,
            ViewportState.ImageExtents.width,
            ViewportState.ImageExtents.height,
            1u,
            1u,
            1u,
            VK_SAMPLE_COUNT_1_BIT,
            VK_IMAGE_TILING_OPTIMAL,
            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT,
            0u,
            false
        };

        Vulkan::Device::CreateImage(DeviceState, ImageDescriptor, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, FrameState.SceneColourImages [CurrentImageIndex]);

        ImageDescriptor.Format = VK_FORMAT_D24_UNORM_S8_UINT;
        ImageDescriptor.UsageFlags = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

        Vulkan::Device::CreateImage(DeviceState, ImageDescriptor, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, FrameState.DepthStencilImages [CurrentImageIndex]);

        Vulkan::ImageViewDescriptor ImageViewDesc =
        {
            VK_IMAGE_VIEW_TYPE_2D,
            VK_FORMAT_R16G16B16A16_SFLOAT,
            VK_IMAGE_ASPECT_COLOR_BIT,
            0u, 1u,
            0u, 1u,
        };

        Vulkan::Device::CreateImageView(DeviceState, FrameState.SceneColourImages [CurrentImageIndex], ImageViewDesc, FrameState.SceneColourImageViews [CurrentImageIndex]);

        ImageViewDesc.Format = VK_FORMAT_D24_UNORM_S8_UINT;
        ImageViewDesc.AspectFlags = VK_IMAGE_ASPECT_DEPTH_BIT;

        Vulkan::Device::CreateImageView(DeviceState, FrameState.DepthStencilImages [CurrentImageIndex], ImageViewDesc, FrameState.DepthStencilImageViews [CurrentImageIndex]);
    }

    for (VkSemaphore & Semaphore : FrameState.Semaphores)
    {
        Vulkan::Device::CreateSemaphore(DeviceState, 0u, Semaphore);
    }

    for (VkFence & Fence : FrameState.Fences)
    {
        Vulkan::Device::CreateFence(DeviceState, VK_FENCE_CREATE_SIGNALED_BIT, Fence);
    }

    for (LinearBufferAllocator::AllocatorState & AllocatorState : FrameState.LinearAllocators)
    {
        LinearBufferAllocator::CreateAllocator(DeviceState, 1024u, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, AllocatorState);
    }

    {
        uint8 CurrentStateIndex = {};
        for (uint16 & AllocatorHandle : FrameState.DescriptorAllocators)
        {
            Vulkan::Descriptors::CreateDescriptorAllocator(DeviceState,
                                                           Vulkan::Descriptors::DescriptorTypes::Uniform | Vulkan::Descriptors::DescriptorTypes::SampledImage | Vulkan::Descriptors::DescriptorTypes::Sampler,
                                                           AllocatorHandle);

            Vulkan::Descriptors::AllocateDescriptorSet(DeviceState, AllocatorHandle, RenderDescriptorSetLayoutHandle, FrameState.DescriptorSets [(CurrentStateIndex << 1u) + 0u]);
            Vulkan::Descriptors::AllocateDescriptorSet(DeviceState, AllocatorHandle, OutputDescriptorSetLayoutHandle, FrameState.DescriptorSets [(CurrentStateIndex << 1u) + 1u]);

            CurrentStateIndex++;
        }
    }
}

static void DestroyFrameState()
{
    for (LinearBufferAllocator::AllocatorState & AllocatorState : FrameState.LinearAllocators)
    {
        LinearBufferAllocator::DestroyAllocator(AllocatorState, DeviceState);
    }

    for (VkFence & Fence : FrameState.Fences)
    {
        Vulkan::Device::DestroyFence(DeviceState, Fence);
    }

    for (VkSemaphore & Semaphore : FrameState.Semaphores)
    {
        Vulkan::Device::DestroySemaphore(DeviceState, Semaphore);
    }

    for (uint32 & FrameBuffer : FrameState.FrameBuffers)
    {
        Vulkan::Device::DestroyFrameBuffer(DeviceState, FrameBuffer, VK_NULL_HANDLE);
    }

    Vulkan::Device::DestroyCommandBuffers(DeviceState, FrameState.CommandBuffers);
}

static bool const CreateMainRenderPass()
{
    bool bResult = true;

    std::array<VkAttachmentDescription, 3u> const Attachments =
    {
        Vulkan::Attachment(VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE, VK_SAMPLE_COUNT_1_BIT),
        Vulkan::Attachment(VK_FORMAT_D24_UNORM_S8_UINT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE, VK_SAMPLE_COUNT_1_BIT),
        Vulkan::Attachment(VK_FORMAT_B8G8R8A8_UNORM, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE, VK_SAMPLE_COUNT_1_BIT),
    };

    std::array<VkAttachmentReference, 4u> const AttachmentReferences =
    {
        VkAttachmentReference { 0u, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL },
        VkAttachmentReference { 1u, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL },
        VkAttachmentReference { 0u, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL },
        VkAttachmentReference { 2u, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL },
    };

    std::array<VkSubpassDependency, 2u> const SubpassDependencies =
    {
        VkSubpassDependency { VK_SUBPASS_EXTERNAL, 0u, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0u, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_DEPENDENCY_BY_REGION_BIT },
        VkSubpassDependency { 0u, 1u, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, VK_DEPENDENCY_BY_REGION_BIT },
    };

    uint32 const DepthStencilAttachmentIndex = { 1u };

    std::array<VkSubpassDescription, 2u> const Subpasses =
    {
        Vulkan::Subpass(VK_PIPELINE_BIND_POINT_GRAPHICS, 1u, &AttachmentReferences [0u], &AttachmentReferences [1u]),
        Vulkan::Subpass(VK_PIPELINE_BIND_POINT_GRAPHICS, 1u, &AttachmentReferences [3u], nullptr, nullptr, 1u, &AttachmentReferences [2u], 1u, &DepthStencilAttachmentIndex),
    };

    {
        VkRenderPassCreateInfo const CreateInfo = Vulkan::RenderPass(static_cast<uint32>(Attachments.size()), Attachments.data(),
                                                                     static_cast<uint32>(Subpasses.size()), Subpasses.data(),
                                                                     static_cast<uint32>(SubpassDependencies.size()), SubpassDependencies.data());

        VERIFY_VKRESULT(vkCreateRenderPass(DeviceState.Device, &CreateInfo, nullptr, &MainRenderPass));
    }

    return bResult;
}

static bool const CreateDescriptorSetLayout()
{
    std::vector<VkDescriptorSetLayoutBinding> const DescriptorBindings =
    {
        Vulkan::DescriptorSetBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1u, 0u, VK_SHADER_STAGE_VERTEX_BIT),
        Vulkan::DescriptorSetBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1u, 1u, VK_SHADER_STAGE_VERTEX_BIT),
        Vulkan::DescriptorSetBinding(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1u, 2u, VK_SHADER_STAGE_FRAGMENT_BIT),
        Vulkan::DescriptorSetBinding(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1u, 3u, VK_SHADER_STAGE_FRAGMENT_BIT),
        Vulkan::DescriptorSetBinding(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1u, 4u, VK_SHADER_STAGE_FRAGMENT_BIT),
        Vulkan::DescriptorSetBinding(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1u, 5u, VK_SHADER_STAGE_FRAGMENT_BIT),
        Vulkan::DescriptorSetBinding(VK_DESCRIPTOR_TYPE_SAMPLER, 1u, 6u, VK_SHADER_STAGE_FRAGMENT_BIT),
        Vulkan::DescriptorSetBinding(VK_DESCRIPTOR_TYPE_SAMPLER, 1u, 7u, VK_SHADER_STAGE_FRAGMENT_BIT),
        Vulkan::DescriptorSetBinding(VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1u, 0u, VK_SHADER_STAGE_FRAGMENT_BIT),
    };

    Vulkan::Descriptors::CreateDescriptorSetLayout(DeviceState, 8u, DescriptorBindings.data(), RenderDescriptorSetLayoutHandle);
    Vulkan::Descriptors::CreateDescriptorSetLayout(DeviceState, 1u, DescriptorBindings.data() + 8u, OutputDescriptorSetLayoutHandle);

    return true;
}

static bool const CreateTorranceSparrowPipeline()
{
    {
        VkDescriptorSetLayout SetLayout = {};
        Vulkan::Descriptors::GetDescriptorSetLayout(RenderDescriptorSetLayoutHandle, SetLayout);

        VkPipelineLayoutCreateInfo const CreateInfo = Vulkan::PipelineLayout(1u, &SetLayout);
        VERIFY_VKRESULT(vkCreatePipelineLayout(DeviceState.Device, &CreateInfo, nullptr, &RenderPipelineLayout));
    }

    ShaderLibrary::CreateShaderModule(DeviceState, ProjectVerticesVSHandle, ProjectVerticesVSShaderModule);
    ShaderLibrary::CreateShaderModule(DeviceState, TorranceSparrowFSHandle, TorranceSparrowFSShaderModule);

    if (ProjectVerticesVSShaderModule == VK_NULL_HANDLE || TorranceSparrowFSShaderModule == VK_NULL_HANDLE)
    {
        return false;
    }

    std::array<VkPipelineShaderStageCreateInfo, 2u> const ShaderStages =
    {
        Vulkan::PipelineShaderStage(VK_SHADER_STAGE_VERTEX_BIT, ProjectVerticesVSShaderModule, kDefaultShaderEntryPointName.c_str()),
        Vulkan::PipelineShaderStage(VK_SHADER_STAGE_FRAGMENT_BIT, TorranceSparrowFSShaderModule, kDefaultShaderEntryPointName.c_str()),
    };

    {
        std::array<VkVertexInputBindingDescription, 3u> VertexInputBindings =
        {
            Vulkan::VertexInputBinding(0u, sizeof(Math::Vector3)),
            Vulkan::VertexInputBinding(1u, sizeof(Math::Vector3)),
            Vulkan::VertexInputBinding(2u, sizeof(Math::Vector3)),
        };

        std::array<VkVertexInputAttributeDescription, 3u> VertexAttributes =
        {
            Vulkan::VertexInputAttribute(0u, 0u, 0u),
            Vulkan::VertexInputAttribute(1u, 1u, 0u),
            Vulkan::VertexInputAttribute(2u, 2u, 0u),
        };

        VkPipelineVertexInputStateCreateInfo const VertexInputState = Vulkan::VertexInputState(static_cast<uint32>(VertexInputBindings.size()), VertexInputBindings.data(),
                                                                                               static_cast<uint32>(VertexAttributes.size()), VertexAttributes.data());

        VkPipelineInputAssemblyStateCreateInfo const InputAssemblerState = Vulkan::InputAssemblyState();

        VkPipelineRasterizationStateCreateInfo const RasterizationState = Vulkan::RasterizationState(VK_CULL_MODE_BACK_BIT);

        VkPipelineColorBlendAttachmentState const ColourAttachmentState = Vulkan::ColourAttachmentBlendState();
        VkPipelineColorBlendStateCreateInfo const ColourBlendState = Vulkan::ColourBlendState(1u, &ColourAttachmentState);

        VkPipelineDepthStencilStateCreateInfo const DepthStencilState = Vulkan::DepthStencilState(VK_TRUE, VK_TRUE, VK_COMPARE_OP_GREATER);
        VkPipelineMultisampleStateCreateInfo const MultiSampleState = Vulkan::MultiSampleState();

        VkPipelineViewportStateCreateInfo const PipelineViewportState = Vulkan::ViewportState(1u, nullptr, 1u, nullptr);

        std::array<VkDynamicState, 2u> const DynamicStates =
        {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR,
        };

        VkPipelineDynamicStateCreateInfo const DynamicState = Vulkan::DynamicState(static_cast<uint32>(DynamicStates.size()), DynamicStates.data());

        VkGraphicsPipelineCreateInfo const CreateInfo = Vulkan::GraphicsPipelineState(RenderPipelineLayout, MainRenderPass, 0u,
                                                                                      static_cast<uint32>(ShaderStages.size()), ShaderStages.data(),
                                                                                      &VertexInputState, &InputAssemblerState, &PipelineViewportState,
                                                                                      &RasterizationState, &ColourBlendState, &DepthStencilState, &DynamicState, &MultiSampleState);

        VERIFY_VKRESULT(vkCreateGraphicsPipelines(DeviceState.Device, VK_NULL_HANDLE, 1u, &CreateInfo, nullptr, &RenderPipeline));
    }

    return true;
}

static bool const CreateOutputPipeline()
{
    {
        VkDescriptorSetLayout SetLayout = {};
        Vulkan::Descriptors::GetDescriptorSetLayout(OutputDescriptorSetLayoutHandle, SetLayout);

        VkPipelineLayoutCreateInfo const PipelineLayout = Vulkan::PipelineLayout(1u, &SetLayout);
        VERIFY_VKRESULT(vkCreatePipelineLayout(DeviceState.Device, &PipelineLayout, nullptr, &OutputPipelineLayout));
    }

    bool const bLoaded = ShaderLibrary::CreateShaderModule(DeviceState, FullScreenTriangleVSHandle, FullScreenTriangleVSShaderModule) 
        && ShaderLibrary::CreateShaderModule(DeviceState, PostProcessFSHandle, PostProcessFSShaderModule);

    if (!bLoaded)
    {
        Logging::Log(Logging::LogTypes::Error, PBR_TEXT("Failed to create shader module for Output pipeline"));
        return false;
    }

    std::array<VkPipelineShaderStageCreateInfo, 2u> const ShaderStages =
    {
        Vulkan::PipelineShaderStage(VK_SHADER_STAGE_VERTEX_BIT, FullScreenTriangleVSShaderModule, kDefaultShaderEntryPointName.c_str()),
        Vulkan::PipelineShaderStage(VK_SHADER_STAGE_FRAGMENT_BIT, PostProcessFSShaderModule, kDefaultShaderEntryPointName.c_str()),
    };

    VkPipelineVertexInputStateCreateInfo const VertexInputState = Vulkan::VertexInputState(0u, nullptr, 0u, nullptr);
    VkPipelineInputAssemblyStateCreateInfo const InputAssemblerState = Vulkan::InputAssemblyState();

    VkPipelineRasterizationStateCreateInfo const RasterizationState = Vulkan::RasterizationState(VK_CULL_MODE_NONE);

    VkPipelineColorBlendAttachmentState const ColourAttachmentState = Vulkan::ColourAttachmentBlendState();
    VkPipelineColorBlendStateCreateInfo const ColourBlendState = Vulkan::ColourBlendState(1u, &ColourAttachmentState);

    VkPipelineDepthStencilStateCreateInfo const DepthStencilState = Vulkan::DepthStencilState(VK_FALSE, VK_FALSE, VK_COMPARE_OP_NEVER);
    VkPipelineMultisampleStateCreateInfo const MultiSampleState = Vulkan::MultiSampleState();

    VkPipelineViewportStateCreateInfo const Viewport = Vulkan::ViewportState(1u, nullptr, 1u, nullptr);

    std::array<VkDynamicState, 2u> DynamicStateFlags =
    {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR,
    };

    VkPipelineDynamicStateCreateInfo const DynamicState = Vulkan::DynamicState(static_cast<uint32>(DynamicStateFlags.size()), DynamicStateFlags.data());

    VkGraphicsPipelineCreateInfo const PipelineCreateInfo = Vulkan::GraphicsPipelineState(OutputPipelineLayout, MainRenderPass, 1u,
                                                                                          static_cast<uint32>(ShaderStages.size()), ShaderStages.data(),
                                                                                          &VertexInputState, &InputAssemblerState, &Viewport,
                                                                                          &RasterizationState, &ColourBlendState, &DepthStencilState,
                                                                                          &DynamicState, &MultiSampleState);

    VERIFY_VKRESULT(vkCreateGraphicsPipelines(DeviceState.Device, VK_NULL_HANDLE, 1u, &PipelineCreateInfo, nullptr, &OutputPipelineState));

    return true;
}

static bool const CreateAndFillUniformBuffers(Scene::SceneData const & Scene, std::vector<LinearBufferAllocator::Allocation> & OutputUniformBufferAllocations)
{
    bool bResult = false;

    {
        LinearBufferAllocator::Allocation & FrameUniformBufferAllocation = { OutputUniformBufferAllocations.emplace_back() };
        bResult = LinearBufferAllocator::Allocate(FrameState.LinearAllocators [FrameState.CurrentFrameStateIndex], sizeof(PerFrameUniformBufferData), FrameUniformBufferAllocation);

        if (bResult)
        {
            float const AspectRatio = static_cast<float>(ViewportState.ImageExtents.width) / static_cast<float>(ViewportState.ImageExtents.height);

            PerFrameUniformBufferData PerFrameData =
            {
                Math::Matrix4x4::Identity(),
                Math::PerspectiveMatrix(Math::ConvertDegreesToRadians(90.0f), AspectRatio, 0.01f, 1000.0f),
            };

            Camera::GetViewMatrix(Scene.MainCamera, PerFrameData.WorldToViewMatrix);
            PerFrameData.WorldToViewMatrix = Math::Matrix4x4::Inverse(PerFrameData.WorldToViewMatrix);

            ::memcpy_s(FrameUniformBufferAllocation.MappedAddress, FrameUniformBufferAllocation.SizeInBytes, &PerFrameData, sizeof(PerFrameData));
        }
    }

    if (Scene.ActorCount > 0u)
    {
        LinearBufferAllocator::Allocation & ActorUniformBufferAllocation = { OutputUniformBufferAllocations.emplace_back() };
        bResult = LinearBufferAllocator::Allocate(FrameState.LinearAllocators [FrameState.CurrentFrameStateIndex], sizeof(PerDrawUniformBufferData) * Scene.ActorCount, ActorUniformBufferAllocation);

        if (bResult)
        {
            std::vector PerDrawDatas = std::vector<PerDrawUniformBufferData>(Scene.ActorCount);

            for (uint32 CurrentActorHandle = { 1u };
                 CurrentActorHandle <= Scene.ActorCount;
                 CurrentActorHandle++)
            {
                Components::Transform::GetTransformationMatrix(CurrentActorHandle, PerDrawDatas [CurrentActorHandle - 1u].ModelToWorldMatrix);
            }

            ::memcpy_s(ActorUniformBufferAllocation.MappedAddress, ActorUniformBufferAllocation.SizeInBytes, PerDrawDatas.data(), sizeof(PerDrawDatas [0u]) * PerDrawDatas.size());
        }
    }

    return bResult;
}

static void UpdateFrameUniformBufferDescriptor(std::vector<LinearBufferAllocator::Allocation> const & UniformBufferAllocations)
{
    std::array<Vulkan::Resource::Buffer, 2u> UniformBuffers = {};
    Vulkan::Resource::GetBuffer(UniformBufferAllocations [0u].Buffer, UniformBuffers [0u]);
    Vulkan::Resource::GetBuffer(UniformBufferAllocations [1u].Buffer, UniformBuffers [1u]);

    std::array<VkDescriptorBufferInfo, 2u> const UniformBufferDescs =
    {
        Vulkan::DescriptorBufferInfo(UniformBuffers [0u].Resource, UniformBufferAllocations [0u].OffsetInBytes, UniformBufferAllocations [0u].SizeInBytes),
        Vulkan::DescriptorBufferInfo(UniformBuffers [1u].Resource, UniformBufferAllocations [1u].OffsetInBytes, UniformBufferAllocations [1u].SizeInBytes),
    };

    /* TODO: Add descriptor image info helper function and then write the descriptors to the descriptor set */
    VkImageView InputAttachmentView = {};
    Vulkan::Resource::GetImageView(FrameState.SceneColourImageViews [FrameState.CurrentFrameStateIndex], InputAttachmentView);

    VkDescriptorImageInfo InputAttachmentDesc = {};
    InputAttachmentDesc.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    InputAttachmentDesc.imageView = InputAttachmentView;

    std::array<VkDescriptorImageInfo, TextureHandles.size() + 2u> ImageViewsAndSamplers = {};

    Assets::Texture::TextureData TextureData = {};

    for (uint8 TextureIndex = {};
         TextureIndex < TextureHandles.size();
         TextureIndex++)
    {
        if (TextureHandles [TextureIndex] > 0u)
        {
            Assets::Texture::GetTextureData(TextureHandles [TextureIndex], TextureData);

            ImageViewsAndSamplers [TextureIndex].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

            Vulkan::Resource::GetImageView(TextureData.ViewHandle, ImageViewsAndSamplers [TextureIndex].imageView);
        }
    }

    ImageViewsAndSamplers [ImageViewsAndSamplers.size() - 2u].sampler = AnisotropicImageSampler;
    ImageViewsAndSamplers [ImageViewsAndSamplers.size() - 1u].sampler = LinearImageSampler;

    std::array<VkWriteDescriptorSet, 5u> const kDescriptors =
    {
        Vulkan::WriteDescriptorSet(FrameState.DescriptorSets [FrameState.CurrentFrameStateIndex << 1u], VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0u, 1u, &UniformBufferDescs [0u], nullptr),
        Vulkan::WriteDescriptorSet(FrameState.DescriptorSets [FrameState.CurrentFrameStateIndex << 1u], VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1u, 1u, &UniformBufferDescs [1u], nullptr),
        Vulkan::WriteDescriptorSet(FrameState.DescriptorSets [FrameState.CurrentFrameStateIndex << 1u], VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 2u, 5u, nullptr, ImageViewsAndSamplers.data()),
        Vulkan::WriteDescriptorSet(FrameState.DescriptorSets [FrameState.CurrentFrameStateIndex << 1u], VK_DESCRIPTOR_TYPE_SAMPLER, 7u, 2u, nullptr, &ImageViewsAndSamplers [ImageViewsAndSamplers.size() - 2u]),
        Vulkan::WriteDescriptorSet(FrameState.DescriptorSets [(FrameState.CurrentFrameStateIndex << 1u) + 1u], VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 0u, 1u, nullptr, &InputAttachmentDesc),
    };

    vkUpdateDescriptorSets(DeviceState.Device, static_cast<uint32>(kDescriptors.size()), kDescriptors.data(), 0u, nullptr);
}

static void ResetCurrentFrameState()
{
    LinearBufferAllocator::Reset(FrameState.LinearAllocators [FrameState.CurrentFrameStateIndex]);

    //Vulkan::Descriptors::ResetDescriptorAllocator(DeviceState, FrameState.DescriptorAllocators [FrameState.CurrentFrameStateIndex]);

    vkResetFences(DeviceState.Device, 1u, &FrameState.Fences [FrameState.CurrentFrameStateIndex]);

    VERIFY_VKRESULT(vkResetCommandBuffer(FrameState.CommandBuffers [FrameState.CurrentFrameStateIndex], 0u));

    VkCommandBufferBeginInfo const BeginInfo = Vulkan::CommandBufferBegin();
    VERIFY_VKRESULT(vkBeginCommandBuffer(FrameState.CommandBuffers [FrameState.CurrentFrameStateIndex], &BeginInfo));
}

static void ResizeViewport()
{
    VERIFY_VKRESULT(vkDeviceWaitIdle(DeviceState.Device));

    if (Vulkan::Viewport::ResizeViewport(InstanceState, DeviceState, ViewportState))
    {
        for (std::uint32_t CurrentImageIndex = {};
             CurrentImageIndex < kFrameStateCount;
             CurrentImageIndex++)
        {
            Vulkan::Device::DestroyImage(DeviceState, FrameState.SceneColourImages [CurrentImageIndex], VK_NULL_HANDLE);
            Vulkan::Device::DestroyImage(DeviceState, FrameState.DepthStencilImages [CurrentImageIndex], VK_NULL_HANDLE);

            /* TODO: Create new depth stencil images and scene colour images */
        }
    }

    FrameState.CurrentFrameStateIndex = 0u;
}

static bool const PreRender(Scene::SceneData const & Scene)
{
    VkResult const ImageAcquireResult = vkAcquireNextImageKHR(DeviceState.Device, ViewportState.SwapChain, std::numeric_limits<uint64>::max(), FrameState.Semaphores [FrameState.CurrentFrameStateIndex], VK_NULL_HANDLE, &FrameState.CurrentImageIndex);

    if (ImageAcquireResult == VK_ERROR_OUT_OF_DATE_KHR)
    {
        ::ResizeViewport();
        return false;
    }

    /* Run through new assets and create + upload the GPU data */

    VkImageView SceneColourImageView = {};
    Vulkan::Resource::GetImageView(FrameState.SceneColourImageViews [FrameState.CurrentFrameStateIndex], SceneColourImageView);

    VkImageView DepthStencilImageView = {};
    Vulkan::Resource::GetImageView(FrameState.DepthStencilImageViews [FrameState.CurrentFrameStateIndex], DepthStencilImageView);

    std::vector<VkImageView> FrameBufferAttachments =
    {
        SceneColourImageView,
        DepthStencilImageView,
        ViewportState.SwapChainImageViews [FrameState.CurrentImageIndex],
    };

    if (FrameState.FrameBuffers [FrameState.CurrentFrameStateIndex] != 0u)
    {
        Vulkan::Device::DestroyFrameBuffer(DeviceState, FrameState.FrameBuffers [FrameState.CurrentFrameStateIndex], VK_NULL_HANDLE);
    }

    Vulkan::Device::CreateFrameBuffer(DeviceState, ViewportState.ImageExtents.width, ViewportState.ImageExtents.height, MainRenderPass, FrameBufferAttachments, FrameState.FrameBuffers [FrameState.CurrentFrameStateIndex]);

    ::ResetCurrentFrameState();

    VkCommandBuffer & CommandBuffer = FrameState.CommandBuffers [FrameState.CurrentFrameStateIndex];


    std::vector<LinearBufferAllocator::Allocation> UniformBufferAllocations = {};
    ::CreateAndFillUniformBuffers(Scene, UniformBufferAllocations);
    ::UpdateFrameUniformBufferDescriptor(UniformBufferAllocations);
    Assets::StaticMesh::InitialiseGPUResources(CommandBuffer, DeviceState, FrameState.Fences [FrameState.CurrentFrameStateIndex]);
    Assets::Texture::InitialiseGPUResources(CommandBuffer, DeviceState, FrameState.Fences [FrameState.CurrentFrameStateIndex]);

    return true;
}

static void PostRender()
{
    FrameState.CurrentFrameStateIndex = (++FrameState.CurrentFrameStateIndex) % kFrameStateCount;

    vkWaitForFences(DeviceState.Device, 1u, &FrameState.Fences [FrameState.CurrentFrameStateIndex], VK_TRUE, std::numeric_limits<uint64>::max());

    Vulkan::Device::DestroyUnusedResources(DeviceState);
}

bool const ForwardRenderer::Initialise(VkApplicationInfo const & ApplicationInfo)
{
    /* These are loaded and compiled async, doing them here should mean they will be ready by the time we create the pipeline state */
    ShaderLibrary::LoadShader("ProjectOnScreen.vert", ProjectVerticesVSHandle);
    ShaderLibrary::LoadShader("TorranceSparrow.frag", TorranceSparrowFSHandle);

    ShaderLibrary::LoadShader("FullScreenTriangle.vert", FullScreenTriangleVSHandle);
    ShaderLibrary::LoadShader("PostProcess.frag", PostProcessFSHandle);

    Assets::Texture::FindTexture("Boat Diffuse", TextureHandles [0u]);
    Assets::Texture::FindTexture("Boat AO", TextureHandles [1u]);
    Assets::Texture::FindTexture("Boat Specular", TextureHandles [2u]);
    Assets::Texture::FindTexture("Boat Gloss", TextureHandles [3u]);

    bool bResult = false;

    if (Vulkan::Instance::CreateInstance(ApplicationInfo, InstanceState) 
        && Vulkan::Device::CreateDevice(InstanceState, DeviceState) 
        && Vulkan::Viewport::CreateViewport(InstanceState, DeviceState, ViewportState))
    {
        bResult = ::CreateMainRenderPass();

        ::CreateDescriptorSetLayout();

        bResult = ::CreateTorranceSparrowPipeline() 
            && ::CreateTorranceSparrowPipeline()
            && ::CreateOutputPipeline();

        if (bResult)
        {
            ::CreateFrameState();
        }
        else
        {
            Logging::Log(Logging::LogTypes::Error, PBR_TEXT("Failed to create graphics pipeline state. Exiting."));
        }

        VkSamplerCreateInfo SamplerCreateInfo = 
        {
            VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
            nullptr,
            0u,
            VK_FILTER_LINEAR, VK_FILTER_LINEAR,
            VK_SAMPLER_MIPMAP_MODE_NEAREST,
            VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
            0.0f, 
            VK_TRUE, 8.0f, 
            VK_FALSE, VK_COMPARE_OP_NEVER,
            0.0f, 0.0f,
            VK_BORDER_COLOR_INT_OPAQUE_BLACK,
            VK_FALSE,
        };

        VERIFY_VKRESULT(vkCreateSampler(DeviceState.Device, &SamplerCreateInfo, nullptr, &AnisotropicImageSampler));

        SamplerCreateInfo.anisotropyEnable = VK_FALSE;

        VERIFY_VKRESULT(vkCreateSampler(DeviceState.Device, &SamplerCreateInfo, nullptr, &LinearImageSampler));
    }

    return bResult;
}

bool const ForwardRenderer::Shutdown()
{
    vkDeviceWaitIdle(DeviceState.Device);

    ::DestroyFrameState();

    DeviceMemoryAllocator::FreeAllDeviceMemory(DeviceState);

    if (RenderPipeline)
    {
        vkDestroyPipeline(DeviceState.Device, RenderPipeline, nullptr);
        RenderPipeline = VK_NULL_HANDLE;
    }

    if (RenderPipelineLayout)
    {
        vkDestroyPipelineLayout(DeviceState.Device, RenderPipelineLayout, nullptr);
        RenderPipelineLayout = VK_NULL_HANDLE;
    }

    ShaderLibrary::DestroyShaderModules(DeviceState);

    if (MainRenderPass)
    {
        vkDestroyRenderPass(DeviceState.Device, MainRenderPass, nullptr);
        MainRenderPass = VK_NULL_HANDLE;
    }

    Vulkan::Viewport::DestroyViewport(DeviceState, ViewportState);

    Vulkan::Device::DestroyDevice(DeviceState);

    Vulkan::Instance::DestroyInstance(InstanceState);

    return true;
}

void ForwardRenderer::Render(Scene::SceneData const & Scene)
{
    if (!::PreRender(Scene))
    {
        return;
    }

    VkCommandBuffer & CommandBuffer = FrameState.CommandBuffers [FrameState.CurrentFrameStateIndex];

    {
        std::array<VkClearValue, 3u> const kAttachmentClearValues =
        {
            VkClearValue { 0.0f, 0.4f, 0.7f, 1.0f },
            VkClearValue { 0.0f, 0u },
            VkClearValue { 0.0f, 0.0f, 0.0f, 1.0f },
        };

        VkFramebuffer CurrentFrameBuffer = {};
        Vulkan::Resource::GetFrameBuffer(FrameState.FrameBuffers [FrameState.CurrentFrameStateIndex], CurrentFrameBuffer);

        VkRenderPassBeginInfo const BeginInfo = Vulkan::RenderPassBegin(MainRenderPass, CurrentFrameBuffer, VkRect2D { VkOffset2D { 0u, 0u }, ViewportState.ImageExtents }, static_cast<uint32>(kAttachmentClearValues.size()), kAttachmentClearValues.data());

        vkCmdBeginRenderPass(CommandBuffer, &BeginInfo, VK_SUBPASS_CONTENTS_INLINE);
    }

    vkCmdSetViewport(CommandBuffer, 0u, 1u, &ViewportState.DynamicViewport);
    vkCmdSetScissor(CommandBuffer, 0u, 1u, &ViewportState.DynamicScissorRect);

    vkCmdBindPipeline(CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, RenderPipeline);

    vkCmdBindDescriptorSets(CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, RenderPipelineLayout, 0u, 1u, &FrameState.DescriptorSets [FrameState.CurrentFrameStateIndex << 1u], 0u, nullptr);

    for (uint32 CurrentActorHandle = { 1u };
         CurrentActorHandle <= Scene.ActorCount;
         CurrentActorHandle++)
    {
        if (Scene::DoesActorHaveComponents(Scene, CurrentActorHandle, static_cast<uint32>(Scene::ComponentMasks::StaticMesh) | static_cast<uint32>(Scene::ComponentMasks::Material)))
        {
            /* TODO: Render with material */
            /* TODO: Per draw descriptor set */

            Components::StaticMesh::ComponentData MeshComponentData = {};
            bool bHasData = Components::StaticMesh::GetComponentData(Scene, CurrentActorHandle, MeshComponentData);

            Assets::StaticMesh::AssetData MeshData = {};
            bHasData &= Assets::StaticMesh::GetAssetData(MeshComponentData.AssetHandle, MeshData);

            /*Components::Material::ComponentData MaterialData = {};
            bHasResources &= Components::Material::GetComponentData(Scene, CurrentActorHandle, MaterialData);*/

            if (bHasData)
            {
                std::array<Vulkan::Resource::Buffer, 4u> MeshBuffers = {};

                Vulkan::Resource::GetBuffer(MeshData.VertexBufferHandle, MeshBuffers [0u]);
                Vulkan::Resource::GetBuffer(MeshData.NormalBufferHandle, MeshBuffers [1u]);
                Vulkan::Resource::GetBuffer(MeshData.UVBufferHandle, MeshBuffers [2u]);
                Vulkan::Resource::GetBuffer(MeshData.IndexBufferHandle, MeshBuffers [3u]);

                vkCmdBindIndexBuffer(CommandBuffer, MeshBuffers [3u].Resource, 0u, VK_INDEX_TYPE_UINT32);

                {
                    std::array<VkBuffer, 3u> const VertexBuffers =
                    {
                        MeshBuffers [0u].Resource,
                        MeshBuffers [1u].Resource,
                        MeshBuffers [2u].Resource,
                    };

                    std::array const BufferOffsets = std::array<VkDeviceSize, VertexBuffers.size()>();

                    vkCmdBindVertexBuffers(CommandBuffer, 0u, static_cast<uint32>(VertexBuffers.size()), VertexBuffers.data(), BufferOffsets.data());
                }

                vkCmdDrawIndexed(CommandBuffer, MeshData.IndexCount, 1u, 0u, 0u, 0u);
            }
        }
    }

    vkCmdNextSubpass(CommandBuffer, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, OutputPipelineState);
    vkCmdBindDescriptorSets(CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, OutputPipelineLayout, 0u, 1u, &FrameState.DescriptorSets [(FrameState.CurrentFrameStateIndex << 1u) + 1u], 0u, nullptr);

    vkCmdDraw(CommandBuffer, 3u, 1u, 0u, 0u);

    vkCmdEndRenderPass(CommandBuffer);

    VERIFY_VKRESULT(vkEndCommandBuffer(CommandBuffer));

    {
        VkPipelineStageFlags const PipelineWaitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

        VkSubmitInfo const SubmitInfo = Vulkan::SubmitInfo(1u, &CommandBuffer,
                                                           1u, &FrameState.Semaphores [kFrameStateCount + FrameState.CurrentFrameStateIndex], // Signal
                                                           1u, &FrameState.Semaphores [FrameState.CurrentFrameStateIndex], // Wait
                                                           &PipelineWaitStage);

        VERIFY_VKRESULT(vkQueueSubmit(DeviceState.GraphicsQueue, 1u, &SubmitInfo, FrameState.Fences [FrameState.CurrentFrameStateIndex]));
    }

    {
        VkPresentInfoKHR PresentInfo = {};
        PresentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        PresentInfo.swapchainCount = 1u;
        PresentInfo.pSwapchains = &ViewportState.SwapChain;
        PresentInfo.waitSemaphoreCount = 1u;
        PresentInfo.pWaitSemaphores = &FrameState.Semaphores [kFrameStateCount + FrameState.CurrentFrameStateIndex];
        PresentInfo.pImageIndices = &FrameState.CurrentImageIndex;

        VkResult PresentResult = vkQueuePresentKHR(DeviceState.GraphicsQueue, &PresentInfo);

        if (PresentResult == VK_SUBOPTIMAL_KHR
            || PresentResult == VK_ERROR_OUT_OF_DATE_KHR)
        {
            ::ResizeViewport();
            return;
        }
    }

    ::PostRender();
}