#include "ForwardRenderer.hpp"

#include "Graphics/VulkanModule.hpp"

#include "Graphics/Instance.hpp"
#include "Graphics/Device.hpp"
#include "Graphics/Viewport.hpp"
#include "Graphics/ShaderLibrary.hpp"

#include "VulkanPBR.hpp"

#include <array>

static std::string const kDefaultShaderEntryPointName = "main";

static Vulkan::Instance::InstanceState InstanceState = {};
static Vulkan::Device::DeviceState DeviceState = {};
static Vulkan::Viewport::ViewportState ViewportState = {};

static VkCommandBuffer CommandBuffer = {};
static VkRenderPass MainRenderPass = {};
static VkShaderModule ShaderModule = {};

static VkPipelineLayout TrianglePipelineLayout = {};
static VkPipeline TrianglePipelineState = {};

static uint16 VertexShaderHandle = {};
static uint16 FragmentShaderHandle = {};

static VkShaderModule VertexShaderModule = {};
static VkShaderModule FragmentShaderModule = {};

/* TODO: Double buffer render loop so we don't wait at the end of each frame for the device to finish */

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

    Vulkan::Viewport::CreateFrameBuffers(DeviceState, MainRenderPass, ViewportState);

    return bResult;
}

static bool const CreateGraphicsPipeline()
{
    {
        VkPipelineLayoutCreateInfo CreateInfo = {};
        CreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;

        VERIFY_VKRESULT(vkCreatePipelineLayout(DeviceState.Device, &CreateInfo, nullptr, &TrianglePipelineLayout));
    }

    VertexShaderModule = ShaderLibrary::CreateShaderModule(DeviceState, VertexShaderHandle);
    FragmentShaderModule = ShaderLibrary::CreateShaderModule(DeviceState, FragmentShaderHandle);

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

        VkPipelineViewportStateCreateInfo ViewportState = {};
        ViewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        ViewportState.viewportCount = 1u;
        ViewportState.scissorCount = 1u;

        std::array<VkDynamicState, 2u> DynamicStates = 
        {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR,
        };

        VkPipelineDynamicStateCreateInfo DynamicState = {};
        DynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        DynamicState.dynamicStateCount = DynamicStates.size();
        DynamicState.pDynamicStates = DynamicStates.data();

        VkGraphicsPipelineCreateInfo CreateInfo = {};
        CreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        CreateInfo.basePipelineHandle = VK_NULL_HANDLE;
        CreateInfo.basePipelineIndex = -1l;
        CreateInfo.renderPass = MainRenderPass;
        CreateInfo.subpass = 0u;
        CreateInfo.stageCount = ShaderStages.size();
        CreateInfo.pStages = ShaderStages.data();
        CreateInfo.layout = TrianglePipelineLayout;
        CreateInfo.pVertexInputState = &VertexInputState;
        CreateInfo.pInputAssemblyState = &InputAssemblerState;
        CreateInfo.pRasterizationState = &RasterizationState;
        CreateInfo.pColorBlendState = &ColourBlendState;
        CreateInfo.pMultisampleState = &MultiSampleState;
        CreateInfo.pViewportState = &ViewportState;
        CreateInfo.pDynamicState = &DynamicState;

        VERIFY_VKRESULT(vkCreateGraphicsPipelines(DeviceState.Device, VK_NULL_HANDLE, 1u, &CreateInfo, nullptr, &TrianglePipelineState));
    }

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

        Vulkan::Device::CreateCommandBuffer(DeviceState, VK_COMMAND_BUFFER_LEVEL_PRIMARY, CommandBuffer);

        VkSemaphoreCreateInfo CreateInfo = {};
        CreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VERIFY_VKRESULT(vkCreateSemaphore(DeviceState.Device, &CreateInfo, nullptr, &DeviceState.PresentSemaphore));
        VERIFY_VKRESULT(vkCreateSemaphore(DeviceState.Device, &CreateInfo, nullptr, &DeviceState.RenderSemaphore));

        bResult = CreateMainRenderPass();

        bResult = CreateGraphicsPipeline();
    }

    return bResult;
}

bool const ForwardRenderer::Shutdown()
{
    vkDeviceWaitIdle(DeviceState.Device);

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

void ForwardRenderer::Render()
{
    uint32 CurrentImageIndex = {};
    VkResult ImageAcquireResult = vkAcquireNextImageKHR(DeviceState.Device, ViewportState.SwapChain, 0, DeviceState.PresentSemaphore, VK_NULL_HANDLE, &CurrentImageIndex);

    if (ImageAcquireResult == VK_ERROR_OUT_OF_DATE_KHR ||
        ImageAcquireResult == VK_SUBOPTIMAL_KHR)
    {
        if (!Vulkan::Viewport::ResizeViewport(InstanceState, DeviceState, ViewportState))
        {
            return;
        }

        Vulkan::Viewport::CreateFrameBuffers(DeviceState, MainRenderPass, ViewportState);

        VERIFY_VKRESULT(vkAcquireNextImageKHR(DeviceState.Device, ViewportState.SwapChain, 0, DeviceState.PresentSemaphore, VK_NULL_HANDLE, &CurrentImageIndex));
    }

    VERIFY_VKRESULT(vkResetCommandPool(DeviceState.Device, DeviceState.CommandPool, 0u));

    VkCommandBufferBeginInfo BeginInfo = {};
    BeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    BeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    VERIFY_VKRESULT(vkBeginCommandBuffer(CommandBuffer, &BeginInfo));

    {
        static VkClearValue ColourClearValue = {};
        ColourClearValue.color = { 0.0f, 0.4f, 0.7f, 1.0f };

        VkRenderPassBeginInfo PassBeginInfo = {};
        PassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        PassBeginInfo.renderPass = MainRenderPass;
        PassBeginInfo.framebuffer = ViewportState.FrameBuffers [CurrentImageIndex];
        PassBeginInfo.renderArea.extent = ViewportState.ImageExtents;
        PassBeginInfo.renderArea.offset = VkOffset2D { 0u, 0u };
        PassBeginInfo.clearValueCount = 1u;
        PassBeginInfo.pClearValues = &ColourClearValue;

        vkCmdBeginRenderPass(CommandBuffer, &PassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
    }

    VkViewport Viewport = {};
    Viewport.x = 0.0f;
    Viewport.y = 0.0f;
    Viewport.width = static_cast<float>(Application::State.CurrentWindowWidth);
    Viewport.height = static_cast<float>(Application::State.CurrentWindowHeight);
    Viewport.minDepth = 0.0f;
    Viewport.maxDepth = 1.0f;

    vkCmdSetViewport(CommandBuffer, 0u, 1u, &Viewport);

    VkRect2D ScissorRect = {};
    ScissorRect.extent.width = Application::State.CurrentWindowWidth;
    ScissorRect.extent.height = Application::State.CurrentWindowHeight;
    ScissorRect.offset.x = 0u;
    ScissorRect.offset.y = 0u;

    vkCmdSetScissor(CommandBuffer, 0u, 1u, &ScissorRect);

    vkCmdBindPipeline(CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, TrianglePipelineState);
    vkCmdDraw(CommandBuffer, 3u, 1u, 0u, 0u);

    vkCmdEndRenderPass(CommandBuffer);

    VERIFY_VKRESULT(vkEndCommandBuffer(CommandBuffer));

    VkPipelineStageFlags StageFlags = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

    VkSubmitInfo SubmitInfo = {};
    SubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    SubmitInfo.commandBufferCount = 1u;
    SubmitInfo.pCommandBuffers = &CommandBuffer;
    SubmitInfo.waitSemaphoreCount = 1u;
    SubmitInfo.pWaitSemaphores = &DeviceState.PresentSemaphore;
    SubmitInfo.pWaitDstStageMask = &StageFlags;
    SubmitInfo.signalSemaphoreCount = 1u;
    SubmitInfo.pSignalSemaphores = &DeviceState.RenderSemaphore;

    VERIFY_VKRESULT(vkQueueSubmit(DeviceState.GraphicsQueue, 1u, &SubmitInfo, VK_NULL_HANDLE));

    VkPresentInfoKHR PresentInfo = {};
    PresentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    PresentInfo.waitSemaphoreCount = 1u;
    PresentInfo.pWaitSemaphores = &DeviceState.RenderSemaphore;
    PresentInfo.swapchainCount = 1u;
    PresentInfo.pSwapchains = &ViewportState.SwapChain;
    PresentInfo.pImageIndices = &CurrentImageIndex;

    vkQueuePresentKHR(DeviceState.GraphicsQueue, &PresentInfo);

    /* Bad at the moment, but wait for the device to finish work before starting the next frame */
    VERIFY_VKRESULT(vkDeviceWaitIdle(DeviceState.Device));
}