#include <vulkan/vulkan.h>

namespace VulkanFunctions
{
#define VK_FUNCTION_DECLARATION(Function)\
    extern PFN_##Function Function

#define VK_EXPORTED_FUNCTION(Function)\
    VK_FUNCTION_DECLARATION(Function)

#define VK_GLOBAL_FUNCTION(Function)\
    VK_FUNCTION_DECLARATION(Function)

#define VK_INSTANCE_FUNCTION(Function)\
    VK_FUNCTION_DECLARATION(Function)

#define VK_INSTANCE_FUNCTION_FROM_EXTENSION(Function, Extension)\
    VK_FUNCTION_DECLARATION(Function)

#define VK_DEVICE_FUNCTION(Function)\
    VK_FUNCTION_DECLARATION(Function)

#define VK_DEVICE_FUNCTION_FROM_EXTENSION(Function, Extension)\
    VK_FUNCTION_DECLARATION(Function)

#include "VulkanFunctions.inl"

#undef VK_FUNCTION_DECLARATION
}

#if defined(_DEBUG)
    #include <string>

    #if UNICODE
        #define TO_STRING(Value) std::to_wstring(Value)
    #else
        #define TO_STRING(Value) std::to_string(Value)
    #endif

    #define VERIFY_VKRESULT(Function)\
        {\
            VkResult Result = Function;\
            if (Result != VK_SUCCESS)\
            {\
                std::basic_string<TCHAR> ErrorMessage = TEXT("");\
                ErrorMessage += TEXT("Function [");\
                ErrorMessage += TEXT(#Function);\
                ErrorMessage += TEXT("]\nFailed with Error Code [");\
                ErrorMessage += TO_STRING(Result);\
                ErrorMessage += TEXT("]\nIn File [");\
                ErrorMessage += __FILE__;\
                ErrorMessage += TEXT("]\nLine [");\
                ErrorMessage += TO_STRING(__LINE__);\
                ErrorMessage += TEXT("]");\
                ::MessageBox(nullptr, ErrorMessage.c_str(), TEXT("Vulkan Error"), MB_OK);\
            }\
        }
#else
    #define VERIFY_VKRESULT(Function)
#endif // _DEBUG

namespace VulkanModule
{
    extern bool const Start();
    extern bool const Stop();
}

#include "CommonTypes.hpp"

VkResult vkCreateInstance(VkInstanceCreateInfo const * pCreateInfo, VkAllocationCallbacks const * pAllocator, VkInstance * pInstance);

extern inline VkResult vkCreateSemaphore(VkDevice device, VkSemaphoreCreateInfo const * pCreateInfo, VkAllocationCallbacks const * pAllocator, VkSemaphore * pSemaphore);

extern inline VkResult vkCreateFramebuffer(VkDevice device, VkFramebufferCreateInfo const * pCreateInfo, VkAllocationCallbacks const * pAllocator, VkFramebuffer * pFramebuffer);

extern inline VkResult vkBeginCommandBuffer(VkCommandBuffer commandBuffer, VkCommandBufferBeginInfo const * pBeginInfo);

extern inline VkResult vkEndCommandBuffer(VkCommandBuffer commandBuffer);

extern inline VkResult vkDeviceWaitIdle(VkDevice device);

extern inline VkResult vkResetCommandPool(VkDevice device, VkCommandPool commandPool, VkCommandPoolResetFlags flags);

extern inline VkResult vkQueueSubmit(VkQueue queue, uint32 submitCount, VkSubmitInfo const * pSubmits, VkFence fence);

extern inline VkResult vkQueuePresentKHR(VkQueue queue, VkPresentInfoKHR const * pPresentInfo);

extern inline VkResult vkAcquireNextImageKHR(VkDevice device, VkSwapchainKHR swapchain, uint64 timeout, VkSemaphore semaphore, VkFence fence, uint32 * pImageIndex);

extern inline void vkCmdClearColorImage(VkCommandBuffer commandBuffer, VkImage image, VkImageLayout imageLayout, VkClearColorValue const * pColor, uint32 rangeCount, VkImageSubresourceRange const * pRanges);

extern inline void vkCmdPipelineBarrier(VkCommandBuffer commandBuffer, 
                                        VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, 
                                        VkDependencyFlags dependencyFlags, 
                                        uint32 memoryBarrierCount, VkMemoryBarrier const * pMemoryBarriers, 
                                        uint32 bufferMemoryBarrierCount, VkBufferMemoryBarrier const * pBufferMemoryBarriers, 
                                        uint32 imageMemoryBarrierCount, VkImageMemoryBarrier const * pImageMemoryBarriers);

extern inline void vkCmdBeginRenderPass(VkCommandBuffer commandBuffer, VkRenderPassBeginInfo const * pRenderPassBegin, VkSubpassContents contents);

extern inline void vkCmdEndRenderPass(VkCommandBuffer commandBuffer);