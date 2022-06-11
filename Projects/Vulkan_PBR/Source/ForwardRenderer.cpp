#include "ForwardRenderer.hpp"

#include "Graphics/VulkanModule.hpp"

#include "Graphics/Instance.hpp"
#include "Graphics/Device.hpp"
#include "Graphics/Viewport.hpp"
#include "Graphics/ShaderLibrary.hpp"

#include "Primitives/StaticMesh.hpp"

#include "Memory.hpp"
#include "Allocation.hpp"

#include <Math/Matrix.hpp>
#include <Math/Transform.hpp>
#include <Math/Utilities.hpp>

#include "VulkanPBR.hpp"

#include "AssetManager.hpp"
#include "GPUResourceManager.hpp"
#include "Scene.hpp"

#include <array>

struct PerFrameUniformBufferData
{
    Math::Matrix4x4 PerspectiveMatrix;
    float Brightness;
};

struct FrameStateCollection
{
    std::vector<VkCommandBuffer> CommandBuffers;
    std::vector<VkSemaphore> Semaphores;
    std::vector<VkFence> Fences;
    std::vector<VkFramebuffer> FrameBuffers;

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

static AssetManager::MeshAsset const * CubeMesh = {};
static Primitives::StaticMesh::StaticMeshData CubeStaticMesh = {};

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
        for (VkFramebuffer & FrameBuffer : FrameState.FrameBuffers)
        {
            std::vector<VkImageView> FrameBufferAttachments =
            {
                ViewportState.SwapChainImageViews [CurrentImageViewIndex],
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

    for (VkFramebuffer & FrameBuffer : FrameState.FrameBuffers)
    {
        Vulkan::Device::DestroyFrameBuffer(DeviceState, FrameBuffer);
    }

    Vulkan::Device::DestroyCommandBuffers(DeviceState, FrameState.CommandBuffers);
}

static bool const CreateMainRenderPass()
{
    bool bResult = true;

    VkAttachmentDescription ColourAttachment = {};
    ColourAttachment.format = ViewportState.SurfaceFormat.format;
    ColourAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    ColourAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    ColourAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    ColourAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    ColourAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    ColourAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    ColourAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

    VkAttachmentReference ColourReference = {};
    ColourReference.attachment = 0u;
    ColourReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription Subpass = {};
    Subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    Subpass.colorAttachmentCount = 1u;
    Subpass.pColorAttachments = &ColourReference;

    {
        VkRenderPassCreateInfo CreateInfo = {};
        CreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        CreateInfo.attachmentCount = 1u;
        CreateInfo.pAttachments = &ColourAttachment;
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

    std::array<VkPipelineShaderStageCreateInfo, 2u> ShaderStages = {};

    {
        VkPipelineShaderStageCreateInfo & VertexStage = ShaderStages [0u];
        VertexStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        VertexStage.stage = VK_SHADER_STAGE_VERTEX_BIT;
        VertexStage.module = VertexShaderModule;
        VertexStage.pName = kDefaultShaderEntryPointName.c_str();

        VkPipelineShaderStageCreateInfo & FragmentStage = ShaderStages [1u];
        FragmentStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        FragmentStage.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        FragmentStage.module = FragmentShaderModule;
        FragmentStage.pName = kDefaultShaderEntryPointName.c_str();
    }

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

        VkPipelineVertexInputStateCreateInfo VertexInputState = Vulkan::VertexInputState(static_cast<uint32>(VertexInputBindings.size()), VertexInputBindings.data(),
                                                                                         static_cast<uint32>(VertexAttributes.size()), VertexAttributes.data());

        VkPipelineInputAssemblyStateCreateInfo InputAssemblerState = Vulkan::InputAssemblyState();

        VkPipelineRasterizationStateCreateInfo RasterizationState = {};
        RasterizationState.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        RasterizationState.cullMode = VK_CULL_MODE_BACK_BIT;
        RasterizationState.polygonMode = VK_POLYGON_MODE_FILL;
        RasterizationState.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        RasterizationState.lineWidth = 1.0f;
        RasterizationState.depthClampEnable = VK_FALSE;
        RasterizationState.depthBiasEnable = VK_FALSE;

        VkPipelineColorBlendAttachmentState ColourAttachmentState = {};
        ColourAttachmentState.blendEnable = VK_FALSE;
        ColourAttachmentState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

        VkPipelineColorBlendStateCreateInfo ColourBlendState = {};
        ColourBlendState.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        ColourBlendState.attachmentCount = 1u;
        ColourBlendState.pAttachments = &ColourAttachmentState;
        ColourBlendState.logicOpEnable = VK_FALSE;

        VkPipelineMultisampleStateCreateInfo MultiSampleState = {};
        MultiSampleState.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        MultiSampleState.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        MultiSampleState.alphaToCoverageEnable = VK_FALSE;
        MultiSampleState.sampleShadingEnable = VK_FALSE;

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

        VkGraphicsPipelineCreateInfo CreateInfo = {};
        CreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        CreateInfo.basePipelineHandle = VK_NULL_HANDLE;
        CreateInfo.basePipelineIndex = -1l;
        CreateInfo.renderPass = MainRenderPass;
        CreateInfo.subpass = 0u;
        CreateInfo.stageCount = static_cast<uint32>(ShaderStages.size());
        CreateInfo.pStages = ShaderStages.data();
        CreateInfo.layout = TrianglePipelineLayout;
        CreateInfo.pVertexInputState = &VertexInputState;
        CreateInfo.pInputAssemblyState = &InputAssemblerState;
        CreateInfo.pRasterizationState = &RasterizationState;
        CreateInfo.pColorBlendState = &ColourBlendState;
        CreateInfo.pMultisampleState = &MultiSampleState;
        CreateInfo.pViewportState = &PipelineViewportState;
        CreateInfo.pDynamicState = &DynamicState;

        VERIFY_VKRESULT(vkCreateGraphicsPipelines(DeviceState.Device, VK_NULL_HANDLE, 1u, &CreateInfo, nullptr, &TrianglePipelineState));
    }

    return true;
}

static bool const CreateFrameUniformBuffer(LinearBufferAllocator::Allocation & OutputAllocation)
{
    LinearBufferAllocator::Allocation IntermediateAllocation = {};
    bool bResult = LinearBufferAllocator::Allocate(FrameState.LinearAllocators [FrameState.CurrentFrameStateIndex], sizeof(PerFrameUniformBufferData), IntermediateAllocation);

    if (bResult)
    {
        PerFrameUniformBufferData PerFrameData = {};

        float const AspectRatio = static_cast<float>(ViewportState.ImageExtents.width) / static_cast<float>(ViewportState.ImageExtents.height);
        PerFrameData.PerspectiveMatrix = Math::PerspectiveMatrix(Math::ConvertDegreesToRadians(90.0f), AspectRatio, 0.001f, 1000.0f);

        PerFrameData.Brightness = -0.25f;

        ::memcpy_s(IntermediateAllocation.MappedAddress, IntermediateAllocation.SizeInBytes, &PerFrameData, sizeof(PerFrameData));

        OutputAllocation = IntermediateAllocation;
    }

    return bResult;
}

static void UpdateUniformBufferDescriptors(LinearBufferAllocator::Allocation const & UniformBufferAllocation)
{
    GPUResource::Buffer UniformBuffer = {};
    GPUResourceManager::GetResource(UniformBufferAllocation.Buffer, UniformBuffer);

    VkDescriptorBufferInfo UniformBufferInfo = {};
    UniformBufferInfo.buffer = UniformBuffer.VkResource;
    UniformBufferInfo.offset = UniformBufferAllocation.OffsetInBytes;
    UniformBufferInfo.range = UniformBufferAllocation.SizeInBytes;

    VkWriteDescriptorSet WriteDescriptors = {};
    WriteDescriptors.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    WriteDescriptors.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    WriteDescriptors.descriptorCount = 1u;
    WriteDescriptors.dstSet = FrameState.DescriptorSets [FrameState.CurrentFrameStateIndex];
    WriteDescriptors.dstBinding = 0u;
    WriteDescriptors.pBufferInfo = &UniformBufferInfo;

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

    Vulkan::Viewport::ResizeViewport(InstanceState, DeviceState, ViewportState);

    {
        uint8 CurrentImageIndex = { 0u };
        for (VkFramebuffer & FrameBuffer : FrameState.FrameBuffers)
        {
            vkDestroyFramebuffer(DeviceState.Device, FrameBuffer, nullptr);

            std::vector<VkImageView> FrameBufferAttachments =
            {
                ViewportState.SwapChainImageViews [CurrentImageIndex],
            };

            Vulkan::Device::CreateFrameBuffer(DeviceState, Application::State.CurrentWindowWidth, Application::State.CurrentWindowHeight, MainRenderPass, FrameBufferAttachments, FrameBuffer);

            CurrentImageIndex++;
        }
    }

    FrameState.CurrentFrameStateIndex = 0u;
}

bool const ForwardRenderer::Initialise(VkApplicationInfo const & ApplicationInfo)
{
    /* These are loaded and compiled async, doing them here should mean they will be ready by the time we create the pipeline state */
    ShaderLibrary::LoadShader("AmbientLighting.vert", VertexShaderHandle);
    ShaderLibrary::LoadShader("AmbientLighting.frag", FragmentShaderHandle);

    bool bResult = false;

    if (Vulkan::Instance::CreateInstance(ApplicationInfo, InstanceState) &&
        Vulkan::Device::CreateDevice(InstanceState, DeviceState) &&
        Vulkan::Viewport::CreateViewport(InstanceState, DeviceState, ViewportState))
    {
        bResult = ::CreateMainRenderPass();

        ::CreateDescriptorSetLayout();

        bResult = ::CreateGraphicsPipeline();

        ::CreateFrameState();
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

void ForwardRenderer::PreRender(Scene::SceneData const & Scene)
{
    VkResult ImageAcquireResult = vkAcquireNextImageKHR(DeviceState.Device, ViewportState.SwapChain, 0ull, FrameState.Semaphores [FrameState.CurrentFrameStateIndex], VK_NULL_HANDLE, &FrameState.CurrentImageIndex);

    if (ImageAcquireResult == VK_SUBOPTIMAL_KHR ||
        ImageAcquireResult == VK_ERROR_OUT_OF_DATE_KHR)
    {
        ::ResizeViewport();

        VERIFY_VKRESULT(vkAcquireNextImageKHR(DeviceState.Device, ViewportState.SwapChain, 0ull, FrameState.Semaphores [FrameState.CurrentFrameStateIndex], VK_NULL_HANDLE, &FrameState.CurrentImageIndex));
    }

    ::ResetCurrentFrameState();

    if (Scene.NewActorIndices.size() > 0u)
    {
        VkCommandBufferBeginInfo BeginInfo = Vulkan::CommandBufferBegin();
        VkCommandBuffer & CurrentCommandBuffer = FrameState.CommandBuffers [kFrameStateCount + FrameState.CurrentFrameStateIndex];

        vkBeginCommandBuffer(CurrentCommandBuffer, &BeginInfo);

        for (uint32 NewActorIndex : Scene.NewActorIndices)
        {
            Primitives::StaticMesh::CreateGPUResources(CurrentCommandBuffer, DeviceState, Scene.ActorMeshHandles [NewActorIndex - 1u], NewActorIndex, FrameState.Fences [FrameState.CurrentFrameStateIndex]);
            Primitives::StaticMesh::GetStaticMeshResources(NewActorIndex, CubeStaticMesh);
        }

        vkEndCommandBuffer(CurrentCommandBuffer);

        VkSubmitInfo const SubmitInfo = Vulkan::SubmitInfo(1u, &CurrentCommandBuffer);
        VERIFY_VKRESULT(vkQueueSubmit(DeviceState.GraphicsQueue, 1u, &SubmitInfo, VK_NULL_HANDLE));
    }

    LinearBufferAllocator::Allocation UniformBufferAllocation = {};
    ::CreateFrameUniformBuffer(UniformBufferAllocation);
    ::UpdateUniformBufferDescriptors(UniformBufferAllocation);
}

void ForwardRenderer::Render(Scene::SceneData const & /*Scene*/)
{
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

        GPUResource::FrameBuffer CurrentFrameBuffer = {};
        GPUResourceManager::GetResource(FrameState.FrameBuffers [FrameState.CurrentFrameStateIndex], CurrentFrameBuffer);

        VkRenderPassBeginInfo BeginInfo = Vulkan::RenderPassBegin(MainRenderPass, CurrentFrameBuffer.VkResource, VkRect2D { VkOffset2D { 0u, 0u }, ViewportState.ImageExtents }, static_cast<uint32>(AttachmentClearValues.size()), AttachmentClearValues.data());
        
        vkCmdBeginRenderPass(CurrentCommandBuffer, &BeginInfo, VK_SUBPASS_CONTENTS_INLINE);
    }

    vkCmdSetViewport(CurrentCommandBuffer, 0u, 1u, &ViewportState.DynamicViewport);
    vkCmdSetScissor(CurrentCommandBuffer, 0u, 1u, &ViewportState.DynamicScissorRect);

    vkCmdBindPipeline(CurrentCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, TrianglePipelineState);

    vkCmdBindDescriptorSets(CurrentCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, TrianglePipelineLayout, 0u, 1u, &FrameState.DescriptorSets [FrameState.CurrentFrameStateIndex], 0u, nullptr);

    GPUResource::Buffer VertexBuffer = {};
    GPUResource::Buffer NormalBuffer = {};
    GPUResource::Buffer IndexBuffer = {};

    GPUResourceManager::GetResource(CubeStaticMesh.VertexBuffer, VertexBuffer);
    GPUResourceManager::GetResource(CubeStaticMesh.NormalBuffer, NormalBuffer);
    GPUResourceManager::GetResource(CubeStaticMesh.IndexBuffer, IndexBuffer);

    vkCmdBindIndexBuffer(CurrentCommandBuffer, IndexBuffer.VkResource, 0u, VK_INDEX_TYPE_UINT32);

    {
        std::array<VkBuffer, 2u> VertexBuffers =
        {
            VertexBuffer.VkResource,
            NormalBuffer.VkResource,
        };

        std::array BufferOffsets = std::array<VkDeviceSize, VertexBuffers.size()>();

        vkCmdBindVertexBuffers(CurrentCommandBuffer, 0u, static_cast<uint32>(VertexBuffers.size()), VertexBuffers.data(), BufferOffsets.data());
    }

    vkCmdDrawIndexed(CurrentCommandBuffer, 36u, 1u, 0u, 0l, 0u);

    vkCmdEndRenderPass(CurrentCommandBuffer);

    vkEndCommandBuffer(CurrentCommandBuffer);

    {
        VkPipelineStageFlags const PipelineWaitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

        VkSubmitInfo SubmitInfo = Vulkan::SubmitInfo(1u, &CurrentCommandBuffer,
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
}

void ForwardRenderer::PostRender()
{
    FrameState.CurrentFrameStateIndex = (++FrameState.CurrentFrameStateIndex) % kFrameStateCount;

    vkWaitForFences(DeviceState.Device, 1u, &FrameState.Fences [FrameState.CurrentFrameStateIndex], VK_TRUE, std::numeric_limits<uint64>::max());

    GPUResourceManager::DestroyUnusedResources(DeviceState);
}