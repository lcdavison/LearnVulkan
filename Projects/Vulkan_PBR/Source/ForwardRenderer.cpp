#include "ForwardRenderer.hpp"

#include "Graphics/VulkanModule.hpp"

#include "Graphics/Instance.hpp"
#include "Graphics/Device.hpp"
#include "Graphics/Viewport.hpp"

#include "VulkanPBR.hpp"

static Vulkan::Instance::InstanceState InstanceState = {};
static Vulkan::Device::DeviceState DeviceState = {};
static Vulkan::Viewport::ViewportState ViewportState = {};

static VkCommandBuffer CommandBuffer = {};
static VkRenderPass MainRenderPass = {};

static bool const CreateMainRenderPass()
{
    bool bResult = true;

    VkAttachmentDescription ColourAttachment = {};
    ColourAttachment.format = ViewportState.SurfaceFormat.format;
    ColourAttachment.initialLayout = VK_IMAGE_LAYOUT_GENERAL;
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

bool const ForwardRenderer::Initialise(VkApplicationInfo const & ApplicationInfo)
{
    bool bResult = Vulkan::Instance::CreateInstance(ApplicationInfo, InstanceState) &&
        Vulkan::Device::CreateDevice(InstanceState, DeviceState) &&
        Vulkan::Viewport::CreateViewport(InstanceState, DeviceState, ViewportState);

    Vulkan::Device::CreateCommandPool(DeviceState, DeviceState.CommandPool);

    Vulkan::Device::CreateCommandBuffer(DeviceState, VK_COMMAND_BUFFER_LEVEL_PRIMARY, CommandBuffer);

    VkSemaphoreCreateInfo CreateInfo = {};
    CreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VERIFY_VKRESULT(vkCreateSemaphore(DeviceState.Device, &CreateInfo, nullptr, &DeviceState.PresentSemaphore));
    VERIFY_VKRESULT(vkCreateSemaphore(DeviceState.Device, &CreateInfo, nullptr, &DeviceState.RenderSemaphore));

    bResult = CreateMainRenderPass();

    return bResult;
}

bool const ForwardRenderer::Shutdown()
{
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
        VkImageMemoryBarrier ImageBarrier = {};
        ImageBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        ImageBarrier.image = ViewportState.SwapChainImages [CurrentImageIndex];
        ImageBarrier.oldLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        ImageBarrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
        ImageBarrier.srcAccessMask = VK_ACCESS_NONE;
        ImageBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        ImageBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        ImageBarrier.subresourceRange.levelCount = 1u;
        ImageBarrier.subresourceRange.layerCount = 1u;

        vkCmdPipelineBarrier(CommandBuffer,
                             VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                             0u,
                             0u, nullptr,
                             0u, nullptr,
                             1u, &ImageBarrier);
    }

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