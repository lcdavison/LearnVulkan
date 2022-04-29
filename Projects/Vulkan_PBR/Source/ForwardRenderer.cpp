#include "ForwardRenderer.hpp"

#include "Graphics/VulkanModule.hpp"

#include "Graphics/Instance.hpp"
#include "Graphics/Device.hpp"
#include "Graphics/Viewport.hpp"
#include "Graphics/ShaderLibrary.hpp"

#include <Math/Matrix.hpp>
#include <Math/Transform.hpp>
#include <Math/Utilities.hpp>

#include "VulkanPBR.hpp"

#include <array>

#undef max

struct PerFrameUniformBufferData
{
    Math::Matrix4x4 PerspectiveMatrix;
    float Brightness;
};

/* TODO: Create a linear allocator that we can use for the uniform buffers */

struct FrameStateCollection
{
    std::vector<VkCommandBuffer> CommandBuffers;
    std::vector<VkSemaphore> Semaphores;
    std::vector<VkFence> Fences;
    std::vector<VkFramebuffer> FrameBuffers;

    //std::vector<VkBuffer> PerFrameUniformBuffers;
    //std::vector<VkBufferView> PerFrameUniformBufferViews;

    uint8 CurrentFrameStateIndex;
};

struct PerFrameUniformBufferData
{
    Math::Matrix4x4 ProjectionMatrix;
};

static std::string const kDefaultShaderEntryPointName = "main";
static uint8 const kFrameStateCount = { 2u };

static Vulkan::Instance::InstanceState InstanceState = {};
static Vulkan::Device::DeviceState DeviceState = {};
static Vulkan::Viewport::ViewportState ViewportState = {};
static FrameStateCollection FrameState = {};

static VkRenderPass MainRenderPass = {};
static VkShaderModule ShaderModule = {};

static VkDescriptorSetLayout TestLayout = {};

static VkPipelineLayout TrianglePipelineLayout = {};
static VkPipeline TrianglePipelineState = {};

static uint16 VertexShaderHandle = {};
static uint16 FragmentShaderHandle = {};

static VkShaderModule VertexShaderModule = {};
static VkShaderModule FragmentShaderModule = {};

static VkBuffer TestUniformBuffer = {};
static VkDeviceMemory UniformBufferMemory = {};

static VkDescriptorPool DescriptorPool = {};
static VkDescriptorSetLayout DescriptorSetLayout = {};
static VkDescriptorSet ShaderDescriptorSet = {};

static void CreateFrameState()
{
    FrameState.CurrentFrameStateIndex = 0u;

    FrameState.CommandBuffers.resize(kFrameStateCount);
    FrameState.FrameBuffers.resize(kFrameStateCount);
    FrameState.Semaphores.resize(kFrameStateCount * 2u);    // Each frame uses 2 semaphores
    FrameState.Fences.resize(kFrameStateCount);

    for (VkCommandBuffer & CommandBuffer : FrameState.CommandBuffers)
    {
        Vulkan::Device::CreateCommandBuffer(DeviceState, VK_COMMAND_BUFFER_LEVEL_PRIMARY, CommandBuffer);
    }

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

    {
        VkSemaphoreCreateInfo CreateInfo = {};
        CreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        for (VkSemaphore & Semaphore : FrameState.Semaphores)
        {
            VERIFY_VKRESULT(vkCreateSemaphore(DeviceState.Device, &CreateInfo, nullptr, &Semaphore));
        }
    }

    {
        VkFenceCreateInfo CreateInfo = {};
        CreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        CreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        for (VkFence & Fence : FrameState.Fences)
        {
            VERIFY_VKRESULT(vkCreateFence(DeviceState.Device, &CreateInfo, nullptr, &Fence));
        }
    }
}

static void DestroyFrameState()
{
    for (VkFence & Fence : FrameState.Fences)
    {
        if (Fence)
        {
            vkDestroyFence(DeviceState.Device, Fence, nullptr);
            Fence = VK_NULL_HANDLE;
        }
    }

    for (VkSemaphore & Semaphore : FrameState.Semaphores)
    {
        if (Semaphore)
        {
            vkDestroySemaphore(DeviceState.Device, Semaphore, nullptr);
            Semaphore = VK_NULL_HANDLE;
        }
    }

    for (VkFramebuffer & FrameBuffer : FrameState.FrameBuffers)
    {
        if (FrameBuffer)
        {
            vkDestroyFramebuffer(DeviceState.Device, FrameBuffer, nullptr);
            FrameBuffer = VK_NULL_HANDLE;
        }
    }

    vkFreeCommandBuffers(DeviceState.Device, DeviceState.CommandPool, static_cast<uint32>(FrameState.CommandBuffers.size()), FrameState.CommandBuffers.data());
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

    VertexShaderModule = ShaderLibrary::CreateShaderModule(DeviceState, VertexShaderHandle);
    FragmentShaderModule = ShaderLibrary::CreateShaderModule(DeviceState, FragmentShaderHandle);

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
        VkPipelineVertexInputStateCreateInfo VertexInputState = {};
        VertexInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

        VkPipelineInputAssemblyStateCreateInfo InputAssemblerState = {};
        InputAssemblerState.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        InputAssemblerState.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        InputAssemblerState.primitiveRestartEnable = VK_FALSE;

        VkPipelineRasterizationStateCreateInfo RasterizationState = {};
        RasterizationState.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        RasterizationState.cullMode = VK_CULL_MODE_NONE;
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

static bool const CreateDescriptorPool()
{
    std::vector PoolSizes = std::vector<VkDescriptorPoolSize>(1u);
    PoolSizes [0u].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    PoolSizes [0u].descriptorCount = 4u;

    VkDescriptorPoolCreateInfo CreateInfo = {};
    CreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    CreateInfo.poolSizeCount = static_cast<uint32>(PoolSizes.size());
    CreateInfo.pPoolSizes = PoolSizes.data();
    CreateInfo.maxSets = 8u;

    VERIFY_VKRESULT(vkCreateDescriptorPool(DeviceState.Device, &CreateInfo, nullptr, &DescriptorPool));

    return true;
}

static bool const CreateUniformBuffer()
{
    Vulkan::Device::CreateBuffer(DeviceState, sizeof(PerFrameUniformBufferData), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, TestUniformBuffer, UniformBufferMemory);

    PerFrameUniformBufferData PerFrameData = {};
    float const AspectRatio = static_cast<float>(ViewportState.ImageExtents.width) / static_cast<float>(ViewportState.ImageExtents.height);
    PerFrameData.PerspectiveMatrix = Math::PerspectiveMatrix(Math::ConvertDegreesToRadians(90.0f), AspectRatio, 0.0001f, 1000.0f);

    PerFrameData.Brightness = -0.25f;

    /* Persistent mapping would probably be better */
    {
        void * MappedData = {};
        VERIFY_VKRESULT(vkMapMemory(DeviceState.Device, UniformBufferMemory, 0u, sizeof(PerFrameData), 0u, &MappedData));

        ::memcpy_s(MappedData, sizeof(PerFrameData), &PerFrameData, sizeof(PerFrameData));

        vkUnmapMemory(DeviceState.Device, UniformBufferMemory);
    }

    {
        VkDescriptorSetAllocateInfo AllocateInfo = {};
        AllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        AllocateInfo.descriptorPool = DescriptorPool;
        AllocateInfo.descriptorSetCount = 1u;
        AllocateInfo.pSetLayouts = &DescriptorSetLayout;

        VERIFY_VKRESULT(vkAllocateDescriptorSets(DeviceState.Device, &AllocateInfo, &ShaderDescriptorSet));
    }

    return true;
}

static bool const UpdateDescriptors()
{
    VkDescriptorBufferInfo BufferInfo = {};
    BufferInfo.buffer = TestUniformBuffer;
    BufferInfo.offset = 0u;
    BufferInfo.range = VK_WHOLE_SIZE;

    VkWriteDescriptorSet DescriptorWrites = {};
    DescriptorWrites.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    DescriptorWrites.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    DescriptorWrites.descriptorCount = 1u;
    DescriptorWrites.dstSet = ShaderDescriptorSet;
    DescriptorWrites.dstBinding = 0u;
    DescriptorWrites.pBufferInfo = &BufferInfo;

    vkUpdateDescriptorSets(DeviceState.Device, 1u, &DescriptorWrites, 0u, nullptr);

    return true;
}

bool const ForwardRenderer::Initialise(VkApplicationInfo const & ApplicationInfo)
{
    /* TODO: This should be moved into the Shader Library itself */
    std::filesystem::path const WorkingDirectory = std::filesystem::current_path();

    VertexShaderHandle = ShaderLibrary::LoadShader(WorkingDirectory / std::filesystem::path("Shaders/Triangle.vert"));
    FragmentShaderHandle = ShaderLibrary::LoadShader(WorkingDirectory / std::filesystem::path("Shaders/Triangle.frag"));

    bool bResult = false;

    if (Vulkan::Instance::CreateInstance(ApplicationInfo, InstanceState) &&
        Vulkan::Device::CreateDevice(InstanceState, DeviceState) &&
        Vulkan::Viewport::CreateViewport(InstanceState, DeviceState, ViewportState))
    {
        Vulkan::Device::CreateCommandPool(DeviceState, DeviceState.CommandPool);

        bResult = ::CreateMainRenderPass();

        ::CreateDescriptorSetLayout();

        bResult = ::CreateGraphicsPipeline();

        ::CreateFrameState();

        ::CreateDescriptorPool();

        ::CreateUniformBuffer();

        ::UpdateDescriptors();
    }

    return bResult;
}

bool const ForwardRenderer::Shutdown()
{
    vkDeviceWaitIdle(DeviceState.Device);

    Vulkan::Device::DestroyBuffer(DeviceState, TestUniformBuffer);

    ::DestroyFrameState();

    if (DescriptorPool)
    {
        vkDestroyDescriptorPool(DeviceState.Device, DescriptorPool, nullptr);
        DescriptorPool = VK_NULL_HANDLE;
    }

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
}

void ForwardRenderer::Render()
{
    vkWaitForFences(DeviceState.Device, 1u, &FrameState.Fences [FrameState.CurrentFrameStateIndex], VK_TRUE, std::numeric_limits<uint64>::max());

    /* TODO: Update the descriptors here */

    uint32 CurrentImageIndex = {};
    VkResult ImageAcquireResult = vkAcquireNextImageKHR(DeviceState.Device, ViewportState.SwapChain, 0ull, FrameState.Semaphores [FrameState.CurrentFrameStateIndex], VK_NULL_HANDLE, &CurrentImageIndex);

    if (ImageAcquireResult == VK_SUBOPTIMAL_KHR ||
        ImageAcquireResult == VK_ERROR_OUT_OF_DATE_KHR)
    {
        ::ResizeViewport();

        VERIFY_VKRESULT(vkAcquireNextImageKHR(DeviceState.Device, ViewportState.SwapChain, 0ull, FrameState.Semaphores [FrameState.CurrentFrameStateIndex], VK_NULL_HANDLE, &CurrentImageIndex));
    }

    vkResetFences(DeviceState.Device, 1u, &FrameState.Fences [FrameState.CurrentFrameStateIndex]);

    VkCommandBuffer & CurrentCommandBuffer = FrameState.CommandBuffers [FrameState.CurrentFrameStateIndex];

    VERIFY_VKRESULT(vkResetCommandBuffer(CurrentCommandBuffer, 0u));

    {
        VkCommandBufferBeginInfo BeginInfo = {};
        BeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        BeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        VERIFY_VKRESULT(vkBeginCommandBuffer(CurrentCommandBuffer, &BeginInfo));
    }

    {
        VkClearValue ClearValue = {};
        ClearValue.color = VkClearColorValue { 0.0f, 0.4f, 0.7f, 1.0f };

        VkRenderPassBeginInfo BeginInfo = {};
        BeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        BeginInfo.renderPass = MainRenderPass;
        BeginInfo.framebuffer = FrameState.FrameBuffers [FrameState.CurrentFrameStateIndex];
        BeginInfo.renderArea.extent = ViewportState.ImageExtents;
        BeginInfo.renderArea.offset = VkOffset2D { 0u, 0u };
        BeginInfo.clearValueCount = 1u;
        BeginInfo.pClearValues = &ClearValue;

        vkCmdBeginRenderPass(CurrentCommandBuffer, &BeginInfo, VK_SUBPASS_CONTENTS_INLINE);
    }

    vkCmdSetViewport(CurrentCommandBuffer, 0u, 1u, &ViewportState.DynamicViewport);
    vkCmdSetScissor(CurrentCommandBuffer, 0u, 1u, &ViewportState.DynamicScissorRect);

    vkCmdBindPipeline(CurrentCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, TrianglePipelineState);
    vkCmdBindDescriptorSets(CurrentCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, TrianglePipelineLayout, 0u, 1u, &ShaderDescriptorSet, 0u, nullptr);

    vkCmdDraw(CurrentCommandBuffer, 3u, 1u, 0u, 0u);

    vkCmdEndRenderPass(CurrentCommandBuffer);

    vkEndCommandBuffer(CurrentCommandBuffer);

    {
        VkPipelineStageFlags const PipelineWaitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

        VkSubmitInfo SubmitInfo = {};
        SubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        SubmitInfo.commandBufferCount = 1u;
        SubmitInfo.pCommandBuffers = &CurrentCommandBuffer;
        SubmitInfo.waitSemaphoreCount = 1u;
        SubmitInfo.pWaitSemaphores = &FrameState.Semaphores [FrameState.CurrentFrameStateIndex];
        SubmitInfo.signalSemaphoreCount = 1u;
        SubmitInfo.pSignalSemaphores = &FrameState.Semaphores [FrameState.CurrentFrameStateIndex + 1u];
        SubmitInfo.pWaitDstStageMask = &PipelineWaitStage;

        VERIFY_VKRESULT(vkQueueSubmit(DeviceState.GraphicsQueue, 1u, &SubmitInfo, FrameState.Fences [FrameState.CurrentFrameStateIndex]));
    }

    {
        VkPresentInfoKHR PresentInfo = {};
        PresentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        PresentInfo.swapchainCount = 1u;
        PresentInfo.pSwapchains = &ViewportState.SwapChain;
        PresentInfo.waitSemaphoreCount = 1u;
        PresentInfo.pWaitSemaphores = &FrameState.Semaphores [FrameState.CurrentFrameStateIndex + 1u];
        PresentInfo.pImageIndices = &CurrentImageIndex;

        vkQueuePresentKHR(DeviceState.GraphicsQueue, &PresentInfo);
    }

    FrameState.CurrentFrameStateIndex = (++FrameState.CurrentFrameStateIndex) % kFrameStateCount;
}