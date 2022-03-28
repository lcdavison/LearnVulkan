#include "ForwardRenderer.hpp"

#include "Graphics/Instance.hpp"
#include "Graphics/Device.hpp"
#include "Graphics/Viewport.hpp"

static Vulkan::Instance::InstanceState InstanceState = {};
static Vulkan::Device::DeviceState DeviceState = {};
static Vulkan::Viewport::ViewportState ViewportState = {};

static VkCommandBuffer CommandBuffer = {};

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

    return bResult;
}

bool const ForwardRenderer::Shutdown()
{
    Vulkan::Viewport::DestroyViewport(DeviceState, ViewportState);

    Vulkan::Device::DestroyDevice(DeviceState);

    Vulkan::Instance::DestroyInstance(InstanceState);

    return true;
}

void ForwardRenderer::Render()
{
    uint32 CurrentImageIndex = {};
    VkResult ImageAcquireResult = vkAcquireNextImageKHR(DeviceState.Device, ViewportState.SwapChain, 0, DeviceState.PresentSemaphore, VK_NULL_HANDLE, &CurrentImageIndex);

    VERIFY_VKRESULT(vkResetCommandPool(DeviceState.Device, DeviceState.CommandPool, 0u));

    VkCommandBufferBeginInfo BeginInfo = {};
    BeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    BeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    VERIFY_VKRESULT(vkBeginCommandBuffer(CommandBuffer, &BeginInfo));

    static VkClearColorValue ClearValue = { 0.0f, 0.4f, 0.7f, 1.0f };

    VkImageSubresourceRange ImageRange = {};
    ImageRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    ImageRange.levelCount = 1u;
    ImageRange.layerCount = 1u;

    {
        VkImageMemoryBarrier ImageBarrier = {};
        ImageBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        ImageBarrier.image = ViewportState.SwapChainImages [CurrentImageIndex];
        ImageBarrier.oldLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        ImageBarrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
        ImageBarrier.srcAccessMask = VK_ACCESS_NONE;
        ImageBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        ImageBarrier.subresourceRange = ImageRange;

        vkCmdPipelineBarrier(CommandBuffer,
                             VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                             0u,
                             0u, nullptr,
                             0u, nullptr,
                             1u, &ImageBarrier);
    }

    /* Currently this gives a validation error since the swapchain images weren't created with a required flag */
    /* But we are still able to clear the image so :) */
    vkCmdClearColorImage(CommandBuffer, ViewportState.SwapChainImages [CurrentImageIndex], VK_IMAGE_LAYOUT_GENERAL, &ClearValue, 1u, &ImageRange);

    {
        VkImageMemoryBarrier ImageBarrier = {};
        ImageBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        ImageBarrier.image = ViewportState.SwapChainImages [CurrentImageIndex];
        ImageBarrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
        ImageBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        ImageBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        ImageBarrier.dstAccessMask = VK_ACCESS_NONE;
        ImageBarrier.subresourceRange = ImageRange;

        vkCmdPipelineBarrier(CommandBuffer,
                             VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                             0u,
                             0u, nullptr,
                             0u, nullptr,
                             1u, &ImageBarrier);
    }

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

    VkResult PresentResult = vkQueuePresentKHR(DeviceState.GraphicsQueue, &PresentInfo);

    /* Bad at the moment, but wait for the device to finish work before starting the next frame */
    VERIFY_VKRESULT(vkDeviceWaitIdle(DeviceState.Device));
}
