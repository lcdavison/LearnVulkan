#include "ForwardRenderer.hpp"

#include "Graphics/VulkanModule.hpp"

#include "Graphics/Instance.hpp"
#include "Graphics/Device.hpp"
#include "Graphics/Viewport.hpp"
#include "Graphics/ShaderLibrary.hpp"

#include "Components/StaticMeshComponent.hpp"

#include "Graphics/Memory.hpp"
#include "Allocation.hpp"

#include <Math/Matrix.hpp>
#include <Math/Transform.hpp>
#include <Math/Utilities.hpp>

#include "VulkanPBR.hpp"

#include "AssetManager.hpp"
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

/* TODO: Separate out the frame state */
struct FrameStateCollection
{
    std::vector<VkCommandBuffer> CommandBuffers;
    std::vector<VkSemaphore> Semaphores;
    std::vector<VkFence> Fences;
    std::vector<uint32> FrameBuffers;

    std::vector<VkDescriptorSet> DescriptorSets;

    std::vector<LinearBufferAllocator::AllocatorState> LinearAllocators;

    uint32 CurrentImageIndex;

    uint8 CurrentFrameStateIndex;
};

static std::string const kDefaultShaderEntryPointName = "main";
static uint8 const kFrameStateCount = { 2u };

static Vulkan::Instance::InstanceState InstanceState = {};
static Vulkan::Device::DeviceState DeviceState = {};
static Vulkan::Viewport::ViewportState ViewportState = {};
static FrameStateCollection FrameState = {};

static VkRenderPass MainRenderPass = {};

static VkPipelineLayout TrianglePipelineLayout = {};
static VkPipeline TrianglePipelineState = {};

static uint16 VertexShaderHandle = {};
static uint16 FragmentShaderHandle = {};

static VkShaderModule VertexShaderModule = {};
static VkShaderModule FragmentShaderModule = {};

static VkDescriptorSetLayout DescriptorSetLayout = {};

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

    uint8 constexpr kCommandBufferCount = { kFrameStateCount * 2u };

    FrameState.CommandBuffers.resize(kCommandBufferCount); // Pre-Render Command Buffer and Render Command Buffer
    FrameState.FrameBuffers.resize(kFrameStateCount);
    FrameState.Semaphores.resize(kFrameStateCount * 2u);    // Each frame uses 2 semaphores
    FrameState.Fences.resize(kFrameStateCount);
    FrameState.DescriptorSets.resize(kFrameStateCount);
    FrameState.LinearAllocators.resize(kFrameStateCount);

    Vulkan::Device::CreateCommandBuffers(DeviceState, VK_COMMAND_BUFFER_LEVEL_PRIMARY, kCommandBufferCount, FrameState.CommandBuffers);

    {
        uint8 CurrentImageViewIndex = { 0u };
        for (uint32 & FrameBuffer : FrameState.FrameBuffers)
        {
            VkImageView DepthStencilImageView = {};
            Vulkan::Resource::GetImageView(ViewportState.DepthStencilImageView, DepthStencilImageView);

            std::vector<VkImageView> FrameBufferAttachments =
            {
                ViewportState.SwapChainImageViews [CurrentImageViewIndex],
                DepthStencilImageView,
            };

            Vulkan::Device::CreateFrameBuffer(DeviceState, ViewportState.ImageExtents.width, ViewportState.ImageExtents.height, MainRenderPass, FrameBufferAttachments, FrameBuffer);

            CurrentImageViewIndex++;
        }
    }

    for (VkSemaphore & Semaphore : FrameState.Semaphores)
    {
        Vulkan::Device::CreateSemaphore(DeviceState, 0u, Semaphore);
    }

    for (VkFence & Fence : FrameState.Fences)
    {
        Vulkan::Device::CreateFence(DeviceState, VK_FENCE_CREATE_SIGNALED_BIT, Fence);
    }

    {
        std::vector DescriptorSetLayouts = std::vector<VkDescriptorSetLayout>(2u, DescriptorSetLayout);

        VkDescriptorSetAllocateInfo AllocateInfo = {};
        AllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        AllocateInfo.descriptorPool = DeviceState.DescriptorPool;
        AllocateInfo.descriptorSetCount = static_cast<uint32>(FrameState.DescriptorSets.size());
        AllocateInfo.pSetLayouts = DescriptorSetLayouts.data();

        vkAllocateDescriptorSets(DeviceState.Device, &AllocateInfo, FrameState.DescriptorSets.data());
    }

    for (LinearBufferAllocator::AllocatorState & AllocatorState : FrameState.LinearAllocators)
    {
        LinearBufferAllocator::CreateAllocator(DeviceState, 1024u, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, AllocatorState);
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

    std::array<VkAttachmentDescription, 2u> Attachments =
    {
        VkAttachmentDescription { 0u, VK_FORMAT_B8G8R8A8_UNORM, VK_SAMPLE_COUNT_1_BIT, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE, VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_DONT_CARE, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR },
        VkAttachmentDescription { 0u, VK_FORMAT_D24_UNORM_S8_UINT, VK_SAMPLE_COUNT_1_BIT, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE, VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_DONT_CARE, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL },
    };

    std::array<VkAttachmentReference, 2u> AttachmentReferences =
    {
        VkAttachmentReference { 0u, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL },
        VkAttachmentReference { 1u, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL },
    };

    VkSubpassDescription Subpass = {};
    Subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    Subpass.colorAttachmentCount = 1u;
    Subpass.pColorAttachments = &AttachmentReferences [0u];
    Subpass.pDepthStencilAttachment = &AttachmentReferences [1u];

    {
        VkRenderPassCreateInfo CreateInfo = {};
        CreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        CreateInfo.attachmentCount = static_cast<uint32>(Attachments.size());
        CreateInfo.pAttachments = Attachments.data();
        CreateInfo.subpassCount = 1u;
        CreateInfo.pSubpasses = &Subpass;

        VERIFY_VKRESULT(vkCreateRenderPass(DeviceState.Device, &CreateInfo, nullptr, &MainRenderPass));
    }

    return bResult;
}

static bool const CreateDescriptorSetLayout()
{
    std::vector DescriptorBindings = std::vector<VkDescriptorSetLayoutBinding>(1u);
    DescriptorBindings [0u].binding = 0u;
    DescriptorBindings [0u].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    DescriptorBindings [0u].descriptorCount = 1u;
    DescriptorBindings [0u].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    VkDescriptorSetLayoutCreateInfo CreateInfo = {};
    CreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    CreateInfo.bindingCount = static_cast<uint32>(DescriptorBindings.size());
    CreateInfo.pBindings = DescriptorBindings.data();

    VERIFY_VKRESULT(vkCreateDescriptorSetLayout(DeviceState.Device, &CreateInfo, nullptr, &DescriptorSetLayout));

    return true;
}

static bool const CreateGraphicsPipeline()
{
    /* TODO: Do following from the shader libary */
    /* Loading required shaders */
    /* Setup pipeline shader slot bindings */
    /* Setup pipeline state */
    /* Create pipeline state */

    {
        VkPipelineLayoutCreateInfo CreateInfo = {};
        CreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        CreateInfo.setLayoutCount = 1u;
        CreateInfo.pSetLayouts = &DescriptorSetLayout;

        VERIFY_VKRESULT(vkCreatePipelineLayout(DeviceState.Device, &CreateInfo, nullptr, &TrianglePipelineLayout));
    }

    ShaderLibrary::CreateShaderModule(DeviceState, VertexShaderHandle, VertexShaderModule);
    ShaderLibrary::CreateShaderModule(DeviceState, FragmentShaderHandle, FragmentShaderModule);

    if (VertexShaderModule == VK_NULL_HANDLE || FragmentShaderModule == VK_NULL_HANDLE)
    {
        return false;
    }

    std::array<VkPipelineShaderStageCreateInfo, 2u> const ShaderStages =
    {
        Vulkan::PipelineShaderStage(VK_SHADER_STAGE_VERTEX_BIT, VertexShaderModule, kDefaultShaderEntryPointName.c_str()),
        Vulkan::PipelineShaderStage(VK_SHADER_STAGE_FRAGMENT_BIT, FragmentShaderModule, kDefaultShaderEntryPointName.c_str()),
    };

    {
        std::array<VkVertexInputBindingDescription, 2u> VertexInputBindings =
        {
            Vulkan::VertexInputBinding(0u, sizeof(Math::Vector3)),
            Vulkan::VertexInputBinding(1u, sizeof(Math::Vector3)),
        };

        std::array<VkVertexInputAttributeDescription, 2u> VertexAttributes =
        {
            Vulkan::VertexInputAttribute(0u, 0u, 0u),
            Vulkan::VertexInputAttribute(1u, 1u, 0u),
        };

        VkPipelineVertexInputStateCreateInfo const VertexInputState = Vulkan::VertexInputState(static_cast<uint32>(VertexInputBindings.size()), VertexInputBindings.data(),
                                                                                               static_cast<uint32>(VertexAttributes.size()), VertexAttributes.data());

        VkPipelineInputAssemblyStateCreateInfo const InputAssemblerState = Vulkan::InputAssemblyState();

        VkPipelineRasterizationStateCreateInfo const RasterizationState = Vulkan::RasterizationState(VK_CULL_MODE_BACK_BIT);

        VkPipelineColorBlendAttachmentState const ColourAttachmentState = Vulkan::ColourAttachmentBlendState();
        VkPipelineColorBlendStateCreateInfo const ColourBlendState = Vulkan::ColourBlendState(1u, &ColourAttachmentState);


        VkPipelineDepthStencilStateCreateInfo const DepthStencilState = Vulkan::DepthStencilState(VK_TRUE, VK_TRUE, VK_COMPARE_OP_LESS);
        VkPipelineMultisampleStateCreateInfo const MultiSampleState = Vulkan::MultiSampleState();

        VkPipelineViewportStateCreateInfo PipelineViewportState = {};
        PipelineViewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        PipelineViewportState.viewportCount = 1u;
        PipelineViewportState.scissorCount = 1u;

        std::array<VkDynamicState, 2u> DynamicStates =
        {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR,
        };

        VkPipelineDynamicStateCreateInfo DynamicState = {};
        DynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        DynamicState.dynamicStateCount = static_cast<uint32>(DynamicStates.size());
        DynamicState.pDynamicStates = DynamicStates.data();

        VkGraphicsPipelineCreateInfo const CreateInfo = Vulkan::GraphicsPipelineState(TrianglePipelineLayout, MainRenderPass, 0u,
                                                                                      static_cast<uint32>(ShaderStages.size()), ShaderStages.data(),
                                                                                      &VertexInputState, &InputAssemblerState, &PipelineViewportState,
                                                                                      &RasterizationState, &ColourBlendState, &DepthStencilState, &DynamicState, &MultiSampleState);

        VERIFY_VKRESULT(vkCreateGraphicsPipelines(DeviceState.Device, VK_NULL_HANDLE, 1u, &CreateInfo, nullptr, &TrianglePipelineState));
    }

    return true;
}

static bool const CreateFrameUniformBuffer(LinearBufferAllocator::Allocation & OutputAllocation, Camera::CameraState const & Camera)
{
    LinearBufferAllocator::Allocation IntermediateAllocation = {};
    bool bResult = LinearBufferAllocator::Allocate(FrameState.LinearAllocators [FrameState.CurrentFrameStateIndex], sizeof(PerFrameUniformBufferData), IntermediateAllocation);

    if (bResult)
    {
        float const AspectRatio = static_cast<float>(ViewportState.ImageExtents.width) / static_cast<float>(ViewportState.ImageExtents.height);

        PerFrameUniformBufferData PerFrameData =
        {
            Math::Matrix4x4::Identity(),
            Math::PerspectiveMatrix(Math::ConvertDegreesToRadians(90.0f), AspectRatio, 0.1f, 1000.0f),
        };

        Camera::GetViewMatrix(Camera, PerFrameData.WorldToViewMatrix);

        PerFrameData.WorldToViewMatrix = Math::Matrix4x4::Inverse(PerFrameData.WorldToViewMatrix);

        ::memcpy_s(IntermediateAllocation.MappedAddress, IntermediateAllocation.SizeInBytes, &PerFrameData, sizeof(PerFrameData));

        OutputAllocation = IntermediateAllocation;
    }

    return bResult;
}

static void UpdateUniformBufferDescriptors(LinearBufferAllocator::Allocation const & UniformBufferAllocation)
{
    Vulkan::Resource::Buffer UniformBuffer = {};
    Vulkan::Resource::GetBuffer(UniformBufferAllocation.Buffer, UniformBuffer);

    std::array<VkDescriptorBufferInfo, 2u> UniformBufferDescs =
    {
        Vulkan::DescriptorBufferInfo(UniformBuffer.Resource, UniformBufferAllocation.OffsetInBytes, UniformBufferAllocation.SizeInBytes),
    };

    VkWriteDescriptorSet const WriteDescriptors = Vulkan::WriteDescriptorSet(FrameState.DescriptorSets [FrameState.CurrentFrameStateIndex], VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0u, 1u, UniformBufferDescs.data(), nullptr);

    vkUpdateDescriptorSets(DeviceState.Device, 1u, &WriteDescriptors, 0u, nullptr);
}

static void ResetCurrentFrameState()
{
    LinearBufferAllocator::Reset(FrameState.LinearAllocators [FrameState.CurrentFrameStateIndex]);

    vkResetFences(DeviceState.Device, 1u, &FrameState.Fences [FrameState.CurrentFrameStateIndex]);
}

static void ResizeViewport()
{
    VERIFY_VKRESULT(vkDeviceWaitIdle(DeviceState.Device));

    if (Vulkan::Viewport::ResizeViewport(InstanceState, DeviceState, ViewportState))
    {
        uint8 CurrentImageIndex = { 0u };
        for (uint32 & FrameBuffer : FrameState.FrameBuffers)
        {
            Vulkan::Device::DestroyFrameBuffer(DeviceState, FrameBuffer, VK_NULL_HANDLE);

            VkImageView DepthStencilImageView = {};
            Vulkan::Resource::GetImageView(ViewportState.DepthStencilImageView, DepthStencilImageView);

            std::vector<VkImageView> FrameBufferAttachments =
            {
                ViewportState.SwapChainImageViews [CurrentImageIndex],
                DepthStencilImageView,
            };

            Vulkan::Device::CreateFrameBuffer(DeviceState, Application::State.CurrentWindowWidth, Application::State.CurrentWindowHeight, MainRenderPass, FrameBufferAttachments, FrameBuffer);

            CurrentImageIndex++;
        }
    }

    FrameState.CurrentFrameStateIndex = 0u;
}

static bool const PreRender(Scene::SceneData const & Scene)
{
    VkResult ImageAcquireResult = vkAcquireNextImageKHR(DeviceState.Device, ViewportState.SwapChain, 0ull, FrameState.Semaphores [FrameState.CurrentFrameStateIndex], VK_NULL_HANDLE, &FrameState.CurrentImageIndex);

    if (ImageAcquireResult == VK_SUBOPTIMAL_KHR ||
        ImageAcquireResult == VK_ERROR_OUT_OF_DATE_KHR)
    {
        ::ResizeViewport();
        return false;
    }

    ::ResetCurrentFrameState();

    if (Scene.NewActorHandles.size() > 0u)
    {
        VkCommandBufferBeginInfo BeginInfo = Vulkan::CommandBufferBegin();
        VkCommandBuffer & CurrentCommandBuffer = FrameState.CommandBuffers [kFrameStateCount + FrameState.CurrentFrameStateIndex];

        vkBeginCommandBuffer(CurrentCommandBuffer, &BeginInfo);

        for (uint32 const ActorHandle : Scene.NewActorHandles)
        {
            if (Scene::DoesActorHaveComponent(Scene, ActorHandle, Scene::ComponentMasks::StaticMesh))
            {
                Components::StaticMesh::TransferToGPU(ActorHandle, CurrentCommandBuffer, DeviceState, FrameState.Fences [FrameState.CurrentFrameStateIndex]);
            }
        }

        vkEndCommandBuffer(CurrentCommandBuffer);

        VkSubmitInfo const SubmitInfo = Vulkan::SubmitInfo(1u, &CurrentCommandBuffer);
        VERIFY_VKRESULT(vkQueueSubmit(DeviceState.GraphicsQueue, 1u, &SubmitInfo, VK_NULL_HANDLE));
    }

    LinearBufferAllocator::Allocation UniformBufferAllocation = {};
    ::CreateFrameUniformBuffer(UniformBufferAllocation, Scene.MainCamera);
    ::UpdateUniformBufferDescriptors(UniformBufferAllocation);

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
    ShaderLibrary::LoadShader("AmbientLighting.vert", VertexShaderHandle);
    ShaderLibrary::LoadShader("AmbientLighting.frag", FragmentShaderHandle);

    bool bResult = false;

    if (Vulkan::Instance::CreateInstance(ApplicationInfo, InstanceState) 
        && Vulkan::Device::CreateDevice(InstanceState, DeviceState) 
        && Vulkan::Viewport::CreateViewport(InstanceState, DeviceState, ViewportState))
    {
        bResult = ::CreateMainRenderPass();

        ::CreateDescriptorSetLayout();

        bResult = ::CreateGraphicsPipeline();

        if (bResult)
        {
            ::CreateFrameState();
        }
        else
        {
            Logging::Log(Logging::LogTypes::Error, PBR_TEXT("Failed to create graphics pipeline state. Exiting."));
        }
    }

    return bResult;
}

bool const ForwardRenderer::Shutdown()
{
    vkDeviceWaitIdle(DeviceState.Device);

    ::DestroyFrameState();

    DeviceMemoryAllocator::FreeAllDeviceMemory(DeviceState);

    if (TrianglePipelineState)
    {
        vkDestroyPipeline(DeviceState.Device, TrianglePipelineState, nullptr);
        TrianglePipelineState = VK_NULL_HANDLE;
    }

    if (TrianglePipelineLayout)
    {
        vkDestroyPipelineLayout(DeviceState.Device, TrianglePipelineLayout, nullptr);
        TrianglePipelineLayout = VK_NULL_HANDLE;
    }

    if (DescriptorSetLayout)
    {
        vkDestroyDescriptorSetLayout(DeviceState.Device, DescriptorSetLayout, nullptr);
        DescriptorSetLayout = VK_NULL_HANDLE;
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

void ForwardRenderer::CreateNewActorResources(Scene::SceneData const & Scene)
{
    for (uint32 const ActorHandle : Scene.NewActorHandles)
    {
        if (Scene::DoesActorHaveComponent(Scene, ActorHandle, Scene::ComponentMasks::StaticMesh))
        {
            Components::StaticMesh::CreateGPUResources(ActorHandle, DeviceState);
        }
    }
}

void ForwardRenderer::Render(Scene::SceneData const & Scene)
{
    if (!::PreRender(Scene))
    {
        return;
    }

    VkCommandBuffer & CurrentCommandBuffer = FrameState.CommandBuffers [FrameState.CurrentFrameStateIndex];

    VERIFY_VKRESULT(vkResetCommandBuffer(CurrentCommandBuffer, 0u));

    {
        VkCommandBufferBeginInfo BeginInfo = Vulkan::CommandBufferBegin();
        VERIFY_VKRESULT(vkBeginCommandBuffer(CurrentCommandBuffer, &BeginInfo));
    }

    {
        std::array<VkClearValue, 2u> AttachmentClearValues =
        {
            VkClearValue { 0.0f, 0.4f, 0.7f, 1.0f },
            VkClearValue { 1.0f, 0u },
        };

        VkFramebuffer CurrentFrameBuffer = {};
        Vulkan::Resource::GetFrameBuffer(FrameState.FrameBuffers [FrameState.CurrentFrameStateIndex], CurrentFrameBuffer);

        VkRenderPassBeginInfo BeginInfo = Vulkan::RenderPassBegin(MainRenderPass, CurrentFrameBuffer, VkRect2D { VkOffset2D { 0u, 0u }, ViewportState.ImageExtents }, static_cast<uint32>(AttachmentClearValues.size()), AttachmentClearValues.data());

        vkCmdBeginRenderPass(CurrentCommandBuffer, &BeginInfo, VK_SUBPASS_CONTENTS_INLINE);
    }

    vkCmdSetViewport(CurrentCommandBuffer, 0u, 1u, &ViewportState.DynamicViewport);
    vkCmdSetScissor(CurrentCommandBuffer, 0u, 1u, &ViewportState.DynamicScissorRect);

    vkCmdBindPipeline(CurrentCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, TrianglePipelineState);

    vkCmdBindDescriptorSets(CurrentCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, TrianglePipelineLayout, 0u, 1u, &FrameState.DescriptorSets [FrameState.CurrentFrameStateIndex], 0u, nullptr);

    for (uint32 CurrentActorHandle = { 1u };
         CurrentActorHandle <= Scene.ActorCount;
         CurrentActorHandle++)
    {
        if (Scene::DoesActorHaveComponent(Scene, CurrentActorHandle, Scene::ComponentMasks::StaticMesh))
        {
            Components::StaticMesh::StaticMeshData CurrentMeshData = {};
            bool const bHasResources = Components::StaticMesh::GetStaticMeshResources(CurrentActorHandle, CurrentMeshData);

            if (bHasResources)
            {
                Vulkan::Resource::Buffer VertexBuffer = {};
                Vulkan::Resource::Buffer NormalBuffer = {};
                Vulkan::Resource::Buffer IndexBuffer = {};

                Vulkan::Resource::GetBuffer(CurrentMeshData.VertexBuffer, VertexBuffer);
                Vulkan::Resource::GetBuffer(CurrentMeshData.NormalBuffer, NormalBuffer);
                Vulkan::Resource::GetBuffer(CurrentMeshData.IndexBuffer, IndexBuffer);

                vkCmdBindIndexBuffer(CurrentCommandBuffer, IndexBuffer.Resource, 0u, VK_INDEX_TYPE_UINT32);

                {
                    std::array<VkBuffer, 2u> const VertexBuffers =
                    {
                        VertexBuffer.Resource,
                        NormalBuffer.Resource,
                    };

                    std::array const BufferOffsets = std::array<VkDeviceSize, VertexBuffers.size()>();

                    vkCmdBindVertexBuffers(CurrentCommandBuffer, 0u, static_cast<uint32>(VertexBuffers.size()), VertexBuffers.data(), BufferOffsets.data());
                }

                vkCmdDrawIndexed(CurrentCommandBuffer, CurrentMeshData.IndexCount, 1u, 0u, 0u, 0u);
            }
        }
    }

    vkCmdEndRenderPass(CurrentCommandBuffer);

    vkEndCommandBuffer(CurrentCommandBuffer);

    {
        VkPipelineStageFlags const PipelineWaitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

        VkSubmitInfo const SubmitInfo = Vulkan::SubmitInfo(1u, &CurrentCommandBuffer,
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

        vkQueuePresentKHR(DeviceState.GraphicsQueue, &PresentInfo);
    }

    ::PostRender();
}