#include "ForwardRenderer.hpp"

#include "Assets/Material.hpp"
#include "Assets/StaticMesh.hpp"
#include "Assets/Texture.hpp"
#include "Camera.hpp"
#include "Components/StaticMeshComponent.hpp"
#include "Components/TransformComponent.hpp"
#include "Graphics/VulkanModule.hpp"
#include "Graphics/Instance.hpp"
#include "Graphics/Device.hpp"
#include "Graphics/Viewport.hpp"
#include "Graphics/ShaderLibrary.hpp"
#include "Graphics/Descriptors.hpp"
#include "Graphics/Memory.hpp"
#include "Graphics/Allocators.hpp"
#include "Scene.hpp"
#include "VulkanPBR.hpp"

#include <Math/Matrix.hpp>
#include <Math/Transform.hpp>
#include <Math/Utilities.hpp>

#include <array>

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
    std::vector<uint16> LinearAllocatorHandles = {};

    std::vector<VkCommandBuffer> CommandBuffers = {};

    std::vector<VkSemaphore> Semaphores = {};
    std::vector<VkFence> Fences = {};

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
static uint8 const kFrameStateCount = { 3u };

static Vulkan::Instance::InstanceState InstanceState = {};
static Vulkan::Device::DeviceState DeviceState = {};
static Vulkan::Viewport::ViewportState ViewportState = {};
static FrameStateCollection FrameState = {};

static VkRenderPass MainRenderPass = {};

/*
*   0 = Render Pipeline
*   1 = Output Pipeline
*/
std::array<VkPipelineLayout, 2u> PipelineLayouts = {};
std::array<VkPipeline, 2u> Pipelines = {};

/*
*   0 = Projection VS
*   1 = Full Screen Triangle VS
*/
std::array<uint16, 2u> VertexShaderHandles = {};

/*
*   0 = Torrance Sparrow FS
*   1 = Post Process FS
*/
std::array<uint16, 2u> FragmentShaderHandles = {};

std::array<VkShaderModule, 2u> VertexShaderModules = {};
std::array<VkShaderModule, 2u> FragmentShaderModules = {};

static VkShaderModule ProjectVerticesVSShaderModule = {};
static VkShaderModule TorranceSparrowFSShaderModule = {};

static VkShaderModule FullScreenTriangleVSShaderModule = {};
static VkShaderModule PostProcessFSShaderModule = {};

/*
*   0 = Per Frame Layout
*   1 = Per Draw Layout
*/
static std::array<uint16, 2u> DescriptorSetLayoutHandles = {};

/*
*   0 = Linear Sampler
*   1 = Anisotropic Sampler
*/
static std::array<VkSampler, 2u> ImageSamplers = {};

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
    FrameState.CommandBuffers.resize(kFrameStateCount); // Pre-Render Command Buffer and Render Command Buffer
    FrameState.FrameBuffers.resize(kFrameStateCount);
    FrameState.SceneColourImages.resize(kFrameStateCount);
    FrameState.SceneColourImageViews.resize(kFrameStateCount);
    FrameState.DepthStencilImages.resize(kFrameStateCount);
    FrameState.DepthStencilImageViews.resize(kFrameStateCount);
    FrameState.Semaphores.resize(kFrameStateCount * 2u);    // Each frame uses 2 semaphores
    FrameState.Fences.resize(kFrameStateCount);
    FrameState.LinearAllocatorHandles.resize(kFrameStateCount);
    FrameState.DescriptorAllocators.resize(kFrameStateCount);

    Vulkan::Device::CreateCommandBuffers(DeviceState, VK_COMMAND_BUFFER_LEVEL_PRIMARY, kFrameStateCount, FrameState.CommandBuffers);

    for (uint8 CurrentFrameStateIndex = {};
         CurrentFrameStateIndex < kFrameStateCount;
         CurrentFrameStateIndex++)
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
            false,
        };

        Vulkan::Device::CreateImage(DeviceState, ImageDescriptor, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, FrameState.SceneColourImages [CurrentFrameStateIndex]);

        ImageDescriptor.Format = VK_FORMAT_D24_UNORM_S8_UINT;
        ImageDescriptor.UsageFlags = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

        Vulkan::Device::CreateImage(DeviceState, ImageDescriptor, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, FrameState.DepthStencilImages [CurrentFrameStateIndex]);

        Vulkan::ImageViewDescriptor ImageViewDesc =
        {
            VK_IMAGE_VIEW_TYPE_2D,
            VK_FORMAT_R16G16B16A16_SFLOAT,
            VK_IMAGE_ASPECT_COLOR_BIT,
            0u, 1u,
            0u, 1u,
            false,
        };

        Vulkan::Device::CreateImageView(DeviceState, FrameState.SceneColourImages [CurrentFrameStateIndex], ImageViewDesc, FrameState.SceneColourImageViews [CurrentFrameStateIndex]);

        ImageViewDesc.Format = VK_FORMAT_D24_UNORM_S8_UINT;
        ImageViewDesc.AspectFlags = VK_IMAGE_ASPECT_DEPTH_BIT;

        Vulkan::Device::CreateImageView(DeviceState, FrameState.DepthStencilImages [CurrentFrameStateIndex], ImageViewDesc, FrameState.DepthStencilImageViews [CurrentFrameStateIndex]);

        Vulkan::Device::CreateSemaphore(DeviceState, 0u, FrameState.Semaphores [(CurrentFrameStateIndex << 1u) + 0u]);
        Vulkan::Device::CreateSemaphore(DeviceState, 0u, FrameState.Semaphores [(CurrentFrameStateIndex << 1u) + 1u]);

        Vulkan::Device::CreateFence(DeviceState, VK_FENCE_CREATE_SIGNALED_BIT, FrameState.Fences [CurrentFrameStateIndex]);

        Vulkan::Allocators::LinearBufferAllocator::CreateAllocator(DeviceState,
                                                                   1024u,
                                                                   VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                                                   VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                                                   FrameState.LinearAllocatorHandles [CurrentFrameStateIndex]);
        Vulkan::Descriptors::CreateDescriptorAllocator(DeviceState,
                                                       Vulkan::Descriptors::DescriptorTypes::Uniform | Vulkan::Descriptors::DescriptorTypes::SampledImage | Vulkan::Descriptors::DescriptorTypes::Sampler,
                                                       FrameState.DescriptorAllocators [CurrentFrameStateIndex]);
    }
}

static void DestroyFrameState()
{
    for (uint16 const kAllocatorHandle : FrameState.LinearAllocatorHandles)
    {
        Vulkan::Allocators::LinearBufferAllocator::DestroyAllocator(kAllocatorHandle, DeviceState);
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
    std::array<VkDescriptorSetLayoutBinding, 10u> const kDescriptorBindings =
    {
        // Per Frame
        Vulkan::DescriptorSetBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1u, 0u, VK_SHADER_STAGE_VERTEX_BIT),
        Vulkan::DescriptorSetBinding(VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1u, 1u, VK_SHADER_STAGE_FRAGMENT_BIT),

        // Per Draw
        Vulkan::DescriptorSetBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1u, 0u, VK_SHADER_STAGE_VERTEX_BIT),
        Vulkan::DescriptorSetBinding(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1u, 1u, VK_SHADER_STAGE_FRAGMENT_BIT),
        Vulkan::DescriptorSetBinding(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1u, 2u, VK_SHADER_STAGE_FRAGMENT_BIT),
        Vulkan::DescriptorSetBinding(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1u, 3u, VK_SHADER_STAGE_FRAGMENT_BIT),
        Vulkan::DescriptorSetBinding(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1u, 4u, VK_SHADER_STAGE_FRAGMENT_BIT),
        Vulkan::DescriptorSetBinding(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1u, 5u, VK_SHADER_STAGE_FRAGMENT_BIT),
        Vulkan::DescriptorSetBinding(VK_DESCRIPTOR_TYPE_SAMPLER, 1u, 6u, VK_SHADER_STAGE_FRAGMENT_BIT),
        Vulkan::DescriptorSetBinding(VK_DESCRIPTOR_TYPE_SAMPLER, 1u, 7u, VK_SHADER_STAGE_FRAGMENT_BIT),
    };

    Vulkan::Descriptors::CreateDescriptorSetLayout(DeviceState, 2u, kDescriptorBindings.data(), DescriptorSetLayoutHandles [0u]);
    Vulkan::Descriptors::CreateDescriptorSetLayout(DeviceState, 8u, kDescriptorBindings.data() + 2u, DescriptorSetLayoutHandles [1u]);

    return true;
}

static bool const CreateTorranceSparrowPipeline()
{
    {
        std::array<VkDescriptorSetLayout, 2u> DescriptorSetLayouts = {};
        Vulkan::Descriptors::GetDescriptorSetLayout(DescriptorSetLayoutHandles [0u], DescriptorSetLayouts [0u]);
        Vulkan::Descriptors::GetDescriptorSetLayout(DescriptorSetLayoutHandles [1u], DescriptorSetLayouts [1u]);

        VkPipelineLayoutCreateInfo const CreateInfo = Vulkan::PipelineLayout(static_cast<uint32>(DescriptorSetLayouts.size()), DescriptorSetLayouts.data());
        VERIFY_VKRESULT(vkCreatePipelineLayout(DeviceState.Device, &CreateInfo, nullptr, &PipelineLayouts [0u]));
    }

    ShaderLibrary::CreateShaderModule(DeviceState, VertexShaderHandles [0u], ProjectVerticesVSShaderModule);
    ShaderLibrary::CreateShaderModule(DeviceState, FragmentShaderHandles [0u], TorranceSparrowFSShaderModule);

    if (ProjectVerticesVSShaderModule == VK_NULL_HANDLE || TorranceSparrowFSShaderModule == VK_NULL_HANDLE)
    {
        /* ERROR */
        return false;
    }

    std::array<VkPipelineShaderStageCreateInfo, 2u> const ShaderStages =
    {
        Vulkan::PipelineShaderStage(VK_SHADER_STAGE_VERTEX_BIT, ProjectVerticesVSShaderModule, kDefaultShaderEntryPointName.c_str()),
        Vulkan::PipelineShaderStage(VK_SHADER_STAGE_FRAGMENT_BIT, TorranceSparrowFSShaderModule, kDefaultShaderEntryPointName.c_str()),
    };

    {
        std::array<VkVertexInputBindingDescription, 4u> const kVertexInputBindings =
        {
            Vulkan::VertexInputBinding(0u, sizeof(Math::Vector3)),
            Vulkan::VertexInputBinding(1u, sizeof(Math::Vector3)),
            Vulkan::VertexInputBinding(2u, sizeof(Math::Vector4)),
            Vulkan::VertexInputBinding(3u, sizeof(Math::Vector3)),
        };

        std::array<VkVertexInputAttributeDescription, 4u> const kVertexAttributes =
        {
            Vulkan::VertexInputAttribute(0u, 0u, 0u),
            Vulkan::VertexInputAttribute(1u, 1u, 0u),
            Vulkan::VertexInputAttribute(2u, 2u, 0u),
            Vulkan::VertexInputAttribute(3u, 3u, 0u),
        };

        VkPipelineVertexInputStateCreateInfo const VertexInputState = Vulkan::VertexInputState(static_cast<uint32>(kVertexInputBindings.size()), kVertexInputBindings.data(),
                                                                                               static_cast<uint32>(kVertexAttributes.size()), kVertexAttributes.data());

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

        VkGraphicsPipelineCreateInfo const CreateInfo = Vulkan::GraphicsPipelineState(PipelineLayouts [0u], MainRenderPass, 0u,
                                                                                      static_cast<uint32>(ShaderStages.size()), ShaderStages.data(),
                                                                                      &VertexInputState, &InputAssemblerState, &PipelineViewportState,
                                                                                      &RasterizationState, &ColourBlendState, &DepthStencilState, &DynamicState, &MultiSampleState);

        VERIFY_VKRESULT(vkCreateGraphicsPipelines(DeviceState.Device, VK_NULL_HANDLE, 1u, &CreateInfo, nullptr, &Pipelines [0u]));
    }

    return true;
}

static bool const CreateOutputPipeline()
{
    {
        VkDescriptorSetLayout DescriptorSetLayout = {};
        Vulkan::Descriptors::GetDescriptorSetLayout(DescriptorSetLayoutHandles [0u], DescriptorSetLayout);

        VkPipelineLayoutCreateInfo const PipelineLayout = Vulkan::PipelineLayout(1u, &DescriptorSetLayout);
        VERIFY_VKRESULT(vkCreatePipelineLayout(DeviceState.Device, &PipelineLayout, nullptr, &PipelineLayouts [1u]));
    }

    bool const bLoaded = ShaderLibrary::CreateShaderModule(DeviceState, VertexShaderHandles [1u], FullScreenTriangleVSShaderModule)
        && ShaderLibrary::CreateShaderModule(DeviceState, FragmentShaderHandles [1u], PostProcessFSShaderModule);

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

    VkGraphicsPipelineCreateInfo const PipelineCreateInfo = Vulkan::GraphicsPipelineState(PipelineLayouts [1u], MainRenderPass, 1u,
                                                                                          static_cast<uint32>(ShaderStages.size()), ShaderStages.data(),
                                                                                          &VertexInputState, &InputAssemblerState, &Viewport,
                                                                                          &RasterizationState, &ColourBlendState, &DepthStencilState,
                                                                                          &DynamicState, &MultiSampleState);

    VERIFY_VKRESULT(vkCreateGraphicsPipelines(DeviceState.Device, VK_NULL_HANDLE, 1u, &PipelineCreateInfo, nullptr, &Pipelines [1u]));

    return true;
}

static bool const CreateAndFillUniformBuffers(Scene::SceneData const & Scene, std::vector<uint32> & OutputUniformBufferAllocations)
{
    bool bResult = false;

    {
        uint32 AllocationHandle = {};
        bResult = Vulkan::Allocators::LinearBufferAllocator::Allocate(FrameState.LinearAllocatorHandles [FrameState.CurrentFrameStateIndex], sizeof(PerFrameUniformBufferData), AllocationHandle);

        if (bResult)
        {
            PerFrameUniformBufferData PerFrameData =
            {
                Math::Matrix4x4::Identity(),
                Scene.MainCamera.ProjectionMatrix,
            };

            Camera::GetViewMatrix(Scene.MainCamera, PerFrameData.WorldToViewMatrix);
            PerFrameData.WorldToViewMatrix = Math::Matrix4x4::Inverse(PerFrameData.WorldToViewMatrix);

            void * MappedAddress = {};
            Vulkan::Allocators::LinearBufferAllocator::GetMappedAddress(AllocationHandle, MappedAddress);

            ::memcpy_s(MappedAddress, sizeof(PerFrameUniformBufferData), &PerFrameData, sizeof(PerFrameData));

            OutputUniformBufferAllocations.push_back(AllocationHandle);
        }
    }

    PerDrawUniformBufferData IntermediateUniformBufferData = {};

    uint32 const kActorCount = { static_cast<uint32>(Scene.ComponentMasks.size()) };

    uint32 AllocationHandle = {};
    bResult &= Vulkan::Allocators::LinearBufferAllocator::Allocate(FrameState.LinearAllocatorHandles [FrameState.CurrentFrameStateIndex], sizeof(PerFrameUniformBufferData) * kActorCount, AllocationHandle);

    void * MappedAddress = {};
    Vulkan::Allocators::LinearBufferAllocator::GetMappedAddress(AllocationHandle, MappedAddress);

    uint32 CurrentOutputIndex = {};
    for (Components::StaticMesh::Types::Iterator CurrentMesh = Components::StaticMesh::Types::Iterator::Begin();
         CurrentMesh != Components::StaticMesh::Types::Iterator::End();
         ++CurrentMesh)
    {
        Components::StaticMesh::Types::ComponentData const kComponentData = *CurrentMesh;

        Components::Transform::GetTransformationMatrix(kComponentData.ParentActorHandle, IntermediateUniformBufferData.ModelToWorldMatrix);

        // need to handle the case there isn't a valid transform
        //if (!bHasTransform)
        //{
        //    // just use some default transform, but should also log this
        //}

        PerDrawUniformBufferData * OutputAddress = static_cast<PerDrawUniformBufferData *>(MappedAddress) + CurrentOutputIndex;

        std::memcpy(OutputAddress, &IntermediateUniformBufferData, sizeof(IntermediateUniformBufferData));

        CurrentOutputIndex++;
    }

    OutputUniformBufferAllocations.push_back(AllocationHandle);

    return bResult;
}

static void UpdatePerFrameDescriptorSet(uint16 const kPerFrameDescriptorSetHandle, uint32 const kPerFrameUniformBufferAllocation)
{
    using namespace Vulkan::Allocators;
    using namespace Vulkan::Allocators::Types;

    AllocationInfo Allocation = {};
    LinearBufferAllocator::GetAllocationInfo(kPerFrameUniformBufferAllocation, Allocation);

    Vulkan::Resource::Buffer PerFrameUniformBuffer = {};
    Vulkan::Resource::GetBuffer(Allocation.BufferHandle, PerFrameUniformBuffer);

    VkImageView SceneColourImageView = {};
    Vulkan::Resource::GetImageView(FrameState.SceneColourImageViews [FrameState.CurrentFrameStateIndex], SceneColourImageView);

    VkDescriptorBufferInfo const kPerFrameUniformBufferDesc = Vulkan::DescriptorBufferInfo(PerFrameUniformBuffer.Resource, Allocation.OffsetInBytes, Allocation.SizeInBytes);
    VkDescriptorImageInfo const kSceneColourImageDesc =
    {
        VK_NULL_HANDLE,
        SceneColourImageView,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
    };

    uint16 const kAllocatorHandle = FrameState.DescriptorAllocators [FrameState.CurrentFrameStateIndex];

    Vulkan::Descriptors::BindBufferDescriptors(kAllocatorHandle, kPerFrameDescriptorSetHandle, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0u, 1u, &kPerFrameUniformBufferDesc);
    Vulkan::Descriptors::BindImageDescriptors(kAllocatorHandle, kPerFrameDescriptorSetHandle, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1u, 1u, &kSceneColourImageDesc);
}

static void UpdatePerDrawDescriptorSet(uint16 const kPerDrawDescriptorSetHandle, uint32 const kPerDrawUniformBufferAllocation, Assets::Material::MaterialData const & Material)
{
    using namespace Vulkan::Allocators;
    using namespace Vulkan::Allocators::Types;

    AllocationInfo Allocation = {};
    LinearBufferAllocator::GetAllocationInfo(kPerDrawUniformBufferAllocation, Allocation);

    Vulkan::Resource::Buffer PerDrawUniformBuffer = {};
    Vulkan::Resource::GetBuffer(Allocation.BufferHandle, PerDrawUniformBuffer);

    VkDescriptorBufferInfo const kPerDrawUniformBufferDesc = Vulkan::DescriptorBufferInfo(PerDrawUniformBuffer.Resource, Allocation.OffsetInBytes, Allocation.SizeInBytes);

    std::array<VkDescriptorImageInfo, 7u> ImageViewsAndSamplers = {};

    Assets::Texture::TextureData TextureData = {};

    Assets::Texture::GetTextureData(Material.AlbedoTexture, TextureData);
    Vulkan::Resource::GetImageView(TextureData.ViewHandle, ImageViewsAndSamplers [0u].imageView);
    ImageViewsAndSamplers [0u].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    Assets::Texture::GetTextureData(Material.SpecularTexture, TextureData);
    Vulkan::Resource::GetImageView(TextureData.ViewHandle, ImageViewsAndSamplers [1u].imageView);
    ImageViewsAndSamplers [1u].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    Assets::Texture::GetTextureData(Material.NormalTexture, TextureData);
    Vulkan::Resource::GetImageView(TextureData.ViewHandle, ImageViewsAndSamplers [2u].imageView);
    ImageViewsAndSamplers [2u].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    
    Assets::Texture::GetTextureData(Material.RoughnessTexture, TextureData);
    Vulkan::Resource::GetImageView(TextureData.ViewHandle, ImageViewsAndSamplers [3u].imageView);
    ImageViewsAndSamplers [3u].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    Assets::Texture::GetTextureData(Material.AmbientOcclusionTexture, TextureData);
    Vulkan::Resource::GetImageView(TextureData.ViewHandle, ImageViewsAndSamplers [4u].imageView);
    ImageViewsAndSamplers [4u].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    ImageViewsAndSamplers [ImageViewsAndSamplers.size() - 2u].sampler = ImageSamplers [1u];
    ImageViewsAndSamplers [ImageViewsAndSamplers.size() - 1u].sampler = ImageSamplers [0u];

    uint16 const kAllocatorHandle = FrameState.DescriptorAllocators [FrameState.CurrentFrameStateIndex];

    Vulkan::Descriptors::BindBufferDescriptors(kAllocatorHandle, kPerDrawDescriptorSetHandle, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0u, 1u, &kPerDrawUniformBufferDesc);
    Vulkan::Descriptors::BindImageDescriptors(kAllocatorHandle, kPerDrawDescriptorSetHandle, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1u, static_cast<uint8>(ImageViewsAndSamplers.size() - 2u), ImageViewsAndSamplers.data());
    Vulkan::Descriptors::BindImageDescriptors(kAllocatorHandle, kPerDrawDescriptorSetHandle, VK_DESCRIPTOR_TYPE_SAMPLER, 6u, 2u, &ImageViewsAndSamplers [ImageViewsAndSamplers.size() - 2u]);
}

static void ResetCurrentFrameState()
{
    Vulkan::Allocators::LinearBufferAllocator::Reset(FrameState.LinearAllocatorHandles [FrameState.CurrentFrameStateIndex]);

    Vulkan::Descriptors::ResetDescriptorAllocator(DeviceState, FrameState.DescriptorAllocators [FrameState.CurrentFrameStateIndex]);

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

static bool const PreRender()
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

static void RenderStaticMeshes(VkCommandBuffer const kCommandBuffer, uint16 const kDescriptorSetHandle, VkDescriptorSet const kMeshDescriptorSet, uint32 const kAllocation)
{
    for (Components::StaticMesh::Types::Iterator CurrentMesh = Components::StaticMesh::Types::Iterator::Begin();
         CurrentMesh != Components::StaticMesh::Types::Iterator::End();
         ++CurrentMesh)
    {
        Components::StaticMesh::Types::ComponentData const kComponentData = *CurrentMesh;

        Assets::StaticMesh::AssetData MeshData = {};
        Assets::Material::MaterialData MaterialData = {};

        bool bHasData = Assets::StaticMesh::GetAssetData(kComponentData.MeshHandle, MeshData);
        bHasData &= Assets::Material::GetAssetData(kComponentData.MaterialHandle, MaterialData);

        if (bHasData)
        {
            ::UpdatePerDrawDescriptorSet(kDescriptorSetHandle, kAllocation, MaterialData);

            std::array<Vulkan::Resource::Buffer, 5u> MeshBuffers = {};

            Vulkan::Resource::GetBuffer(MeshData.VertexBufferHandle, MeshBuffers [0u]);
            Vulkan::Resource::GetBuffer(MeshData.NormalBufferHandle, MeshBuffers [1u]);
            Vulkan::Resource::GetBuffer(MeshData.TangentBufferHandle, MeshBuffers [2u]);
            Vulkan::Resource::GetBuffer(MeshData.UVBufferHandle, MeshBuffers [3u]);
            Vulkan::Resource::GetBuffer(MeshData.IndexBufferHandle, MeshBuffers [4u]);

            vkCmdBindIndexBuffer(kCommandBuffer, MeshBuffers [4u].Resource, 0u, VK_INDEX_TYPE_UINT32);

            {
                std::array<VkBuffer, 4u> const VertexBuffers =
                {
                    MeshBuffers [0u].Resource,
                    MeshBuffers [1u].Resource,
                    MeshBuffers [2u].Resource,
                    MeshBuffers [3u].Resource,
                };

                std::array const BufferOffsets = std::array<VkDeviceSize, VertexBuffers.size()>();

                vkCmdBindVertexBuffers(kCommandBuffer, 0u, static_cast<uint32>(VertexBuffers.size()), VertexBuffers.data(), BufferOffsets.data());
            }

            vkCmdBindDescriptorSets(kCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, PipelineLayouts [0u], 1u, 1u, &kMeshDescriptorSet, 0u, nullptr);

            /* Put this here at the moment, ideally we should be flushing after changing the material */
            /* Doesn't do anything if there aren't any descriptors to flush */
            Vulkan::Descriptors::FlushDescriptorWrites(DeviceState);

            vkCmdDrawIndexed(kCommandBuffer, MeshData.IndexCount, 1u, 0u, 0u, 0u);
        }
    }
}

bool const ForwardRenderer::Initialise(VkApplicationInfo const & ApplicationInfo)
{
    /* These are loaded and compiled async, doing them here should mean they will be ready by the time we create the pipeline state */
    ShaderLibrary::LoadShader("ProjectOnScreen.vert", VertexShaderHandles [0u]);
    ShaderLibrary::LoadShader("TorranceSparrow.frag", FragmentShaderHandles [0u]);

    ShaderLibrary::LoadShader("FullScreenTriangle.vert", VertexShaderHandles [1u]);
    ShaderLibrary::LoadShader("PostProcess.frag", FragmentShaderHandles [1u]);

    bool bResult = false;

    if (Vulkan::Instance::CreateInstance(ApplicationInfo, InstanceState))
    {
        bResult = Vulkan::Device::CreateDevice(InstanceState, DeviceState);
        bResult &= Vulkan::Viewport::CreateViewport(InstanceState, DeviceState, ViewportState);

        if (bResult)
        {
            bResult &= ::CreateMainRenderPass();
            bResult &= ::CreateDescriptorSetLayout();
            bResult &= ::CreateTorranceSparrowPipeline();
            bResult &= ::CreateOutputPipeline();

            if (bResult)
            {
                ::CreateFrameState();

                VkSamplerCreateInfo SamplerCreateInfo =
                {
                    VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
                    nullptr,
                    0u,
                    VK_FILTER_LINEAR, VK_FILTER_LINEAR,
                    VK_SAMPLER_MIPMAP_MODE_NEAREST,
                    VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
                    0.0f,
                    VK_FALSE, 8.0f,
                    VK_FALSE, VK_COMPARE_OP_NEVER,
                    0.0f, 0.0f,
                    VK_BORDER_COLOR_INT_OPAQUE_BLACK,
                    VK_FALSE,
                };

                VERIFY_VKRESULT(vkCreateSampler(DeviceState.Device, &SamplerCreateInfo, nullptr, &ImageSamplers [0u]));

                SamplerCreateInfo.anisotropyEnable = VK_TRUE;

                VERIFY_VKRESULT(vkCreateSampler(DeviceState.Device, &SamplerCreateInfo, nullptr, &ImageSamplers [1u]));
            }
            else
            {
                Logging::Log(Logging::LogTypes::Error, PBR_TEXT("Failed to create graphics pipeline state. Exiting."));
            }
        }
    }

    return bResult;
}

bool const ForwardRenderer::Shutdown()
{
    vkDeviceWaitIdle(DeviceState.Device);

    ::DestroyFrameState();

    for (uint8 PipelineIndex = {};
         PipelineIndex < Pipelines.size();
         PipelineIndex++)
    {
        if (Pipelines [PipelineIndex] != VK_NULL_HANDLE)
        {
            vkDestroyPipeline(DeviceState.Device, Pipelines [PipelineIndex], nullptr);
        }
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
    if (!::PreRender())
    {
        return;
    }

    // before rendering the meshes, we will want to find all lights in the scene and organise them into a buffer.
    // due to the nature of the code, we can simply iterate over all the light components

    // setup uniform buffers for all things that need rendering (atm this is just static meshes)

    std::vector<uint32> UniformBufferAllocations = {};
    ::CreateAndFillUniformBuffers(Scene, UniformBufferAllocations);

    /* Only require 2 per frame atm, so allocate here */
    uint16 const kDescriptorAllocatorHandle = { FrameState.DescriptorAllocators [FrameState.CurrentFrameStateIndex] };

    std::array<uint16, 2u> DescriptorSetHandles = {};
    Vulkan::Descriptors::AllocateDescriptorSet(DeviceState, kDescriptorAllocatorHandle, DescriptorSetLayoutHandles [0u], DescriptorSetHandles [0u]);
    Vulkan::Descriptors::AllocateDescriptorSet(DeviceState, kDescriptorAllocatorHandle, DescriptorSetLayoutHandles [1u], DescriptorSetHandles [1u]);

    ::UpdatePerFrameDescriptorSet(DescriptorSetHandles [0u], UniformBufferAllocations [0u]);

    VkCommandBuffer CommandBuffer = FrameState.CommandBuffers [FrameState.CurrentFrameStateIndex];

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

    vkCmdBindPipeline(CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, Pipelines [0u]);

    std::array<VkDescriptorSet, 2u> DescriptorSets = {};
    Vulkan::Descriptors::GetDescriptorSet(kDescriptorAllocatorHandle, DescriptorSetHandles [0u], DescriptorSets[0u]);
    Vulkan::Descriptors::GetDescriptorSet(kDescriptorAllocatorHandle, DescriptorSetHandles [1u], DescriptorSets [1u]);

    /*
        Frequency Based Descriptor Sets
        - Allocate descriptor set per material (Need a way to determine active materials)
        - Check if per mesh data is necessary
        - Need a descriptor set for per-frame data
    */

    /* bind the per-frame descriptor set */
    vkCmdBindDescriptorSets(CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, PipelineLayouts [0u], 0u, 1u, DescriptorSets.data(), 0u, nullptr);

    ::RenderStaticMeshes(CommandBuffer, DescriptorSetHandles [1u], DescriptorSets [1u], UniformBufferAllocations [1u]);
    
    vkCmdNextSubpass(CommandBuffer, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, Pipelines [1u]);
    vkCmdBindDescriptorSets(CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, PipelineLayouts [1u], 0u, 1u, DescriptorSets.data(), 0u, nullptr);

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
        VkPresentInfoKHR const kPresentInfo = Vulkan::PresentInfo(1u, &ViewportState.SwapChain,
                                                                  &FrameState.CurrentImageIndex,
                                                                  1u, &FrameState.Semaphores [kFrameStateCount + FrameState.CurrentFrameStateIndex]);

        VkResult const kPresentResult = vkQueuePresentKHR(DeviceState.GraphicsQueue, &kPresentInfo);

        if (kPresentResult == VK_SUBOPTIMAL_KHR || kPresentResult == VK_ERROR_OUT_OF_DATE_KHR)
        {
            ::ResizeViewport();
            return;
        }
    }

    ::PostRender();
}