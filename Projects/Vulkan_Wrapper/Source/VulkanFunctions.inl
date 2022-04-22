#ifndef VK_EXPORTED_FUNCTION
#define VK_EXPORTED_FUNCTION(Function)
#endif

VK_EXPORTED_FUNCTION(vkGetInstanceProcAddr);

#undef VK_EXPORTED_FUNCTION

#ifndef VK_GLOBAL_FUNCTION
#define VK_GLOBAL_FUNCTION(Function)
#endif

VK_GLOBAL_FUNCTION(vkEnumerateInstanceExtensionProperties);
VK_GLOBAL_FUNCTION(vkEnumerateInstanceLayerProperties);

VK_GLOBAL_FUNCTION(vkCreateInstance);

#undef VK_GLOBAL_FUNCTION

#ifndef VK_INSTANCE_FUNCTION
#define VK_INSTANCE_FUNCTION(Function)
#endif

VK_INSTANCE_FUNCTION(vkGetDeviceProcAddr);

VK_INSTANCE_FUNCTION(vkEnumeratePhysicalDevices);
VK_INSTANCE_FUNCTION(vkEnumerateDeviceExtensionProperties);
VK_INSTANCE_FUNCTION(vkEnumerateDeviceLayerProperties);

VK_INSTANCE_FUNCTION(vkGetPhysicalDeviceProperties);
VK_INSTANCE_FUNCTION(vkGetPhysicalDeviceFeatures);
VK_INSTANCE_FUNCTION(vkGetPhysicalDeviceQueueFamilyProperties);

VK_INSTANCE_FUNCTION(vkCreateDevice);

VK_INSTANCE_FUNCTION(vkDestroyInstance);


#undef VK_INSTANCE_FUNCTION

#ifndef VK_INSTANCE_FUNCTION_FROM_EXTENSION
#define VK_INSTANCE_FUNCTION_FROM_EXTENSION(Function, Extension)
#endif

VK_INSTANCE_FUNCTION_FROM_EXTENSION(vkGetPhysicalDeviceSurfaceSupportKHR, VK_KHR_SURFACE_EXTENSION_NAME);
VK_INSTANCE_FUNCTION_FROM_EXTENSION(vkGetPhysicalDeviceSurfaceCapabilitiesKHR, VK_KHR_SURFACE_EXTENSION_NAME);
VK_INSTANCE_FUNCTION_FROM_EXTENSION(vkGetPhysicalDeviceSurfaceFormatsKHR, VK_KHR_SURFACE_EXTENSION_NAME);

VK_INSTANCE_FUNCTION_FROM_EXTENSION(vkCreateWin32SurfaceKHR, VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
VK_INSTANCE_FUNCTION_FROM_EXTENSION(vkDestroySurfaceKHR, VK_KHR_SURFACE_EXTENSION_NAME);

#undef VK_INSTANCE_FUNCTION_FROM_EXTENSION

#ifndef VK_DEVICE_FUNCTION
#define VK_DEVICE_FUNCTION(Function)
#endif

VK_DEVICE_FUNCTION(vkGetDeviceQueue);
VK_DEVICE_FUNCTION(vkDeviceWaitIdle);

VK_DEVICE_FUNCTION(vkAllocateCommandBuffers);
VK_DEVICE_FUNCTION(vkAllocateDescriptorSets);

VK_DEVICE_FUNCTION(vkFreeCommandBuffers);
VK_DEVICE_FUNCTION(vkFreeDescriptorSets);

VK_DEVICE_FUNCTION(vkCreateBuffer);
VK_DEVICE_FUNCTION(vkGetBufferMemoryRequirements);

VK_DEVICE_FUNCTION(vkCreateCommandPool);
VK_DEVICE_FUNCTION(vkCreateSemaphore);
VK_DEVICE_FUNCTION(vkCreateFence);
VK_DEVICE_FUNCTION(vkCreateFramebuffer);
VK_DEVICE_FUNCTION(vkCreateRenderPass);
VK_DEVICE_FUNCTION(vkCreateImageView);
VK_DEVICE_FUNCTION(vkCreateShaderModule);
VK_DEVICE_FUNCTION(vkCreatePipelineLayout);
VK_DEVICE_FUNCTION(vkCreateGraphicsPipelines);
VK_DEVICE_FUNCTION(vkCreateDescriptorPool);
VK_DEVICE_FUNCTION(vkCreateDescriptorSetLayout);

VK_DEVICE_FUNCTION(vkDestroyDevice);
VK_DEVICE_FUNCTION(vkDestroyCommandPool);
VK_DEVICE_FUNCTION(vkDestroySemaphore);
VK_DEVICE_FUNCTION(vkDestroyFence);
VK_DEVICE_FUNCTION(vkDestroyRenderPass);
VK_DEVICE_FUNCTION(vkDestroyFramebuffer);
VK_DEVICE_FUNCTION(vkDestroyImageView);
VK_DEVICE_FUNCTION(vkDestroyShaderModule);
VK_DEVICE_FUNCTION(vkDestroyPipelineLayout);
VK_DEVICE_FUNCTION(vkDestroyPipeline);
VK_DEVICE_FUNCTION(vkDestroyDescriptorPool);
VK_DEVICE_FUNCTION(vkDestroyDescriptorSetLayout);

VK_DEVICE_FUNCTION(vkWaitForFences);

VK_DEVICE_FUNCTION(vkResetFences);
VK_DEVICE_FUNCTION(vkResetCommandPool);
VK_DEVICE_FUNCTION(vkResetCommandBuffer);

VK_DEVICE_FUNCTION(vkQueueSubmit);

VK_DEVICE_FUNCTION(vkUpdateDescriptorSets);

VK_DEVICE_FUNCTION(vkBeginCommandBuffer);
VK_DEVICE_FUNCTION(vkEndCommandBuffer);

VK_DEVICE_FUNCTION(vkCmdClearColorImage);

VK_DEVICE_FUNCTION(vkCmdPipelineBarrier);

VK_DEVICE_FUNCTION(vkCmdBindPipeline);
VK_DEVICE_FUNCTION(vkCmdBindDescriptorSets);

VK_DEVICE_FUNCTION(vkCmdDraw);

VK_DEVICE_FUNCTION(vkCmdBeginRenderPass);
VK_DEVICE_FUNCTION(vkCmdEndRenderPass);

VK_DEVICE_FUNCTION(vkCmdSetViewport);
VK_DEVICE_FUNCTION(vkCmdSetScissor);

#undef VK_DEVICE_FUNCTION

#ifndef VK_DEVICE_FUNCTION_FROM_EXTENSION
#define VK_DEVICE_FUNCTION_FROM_EXTENSION(Function, Extension)
#endif

VK_DEVICE_FUNCTION_FROM_EXTENSION(vkCreateSwapchainKHR, VK_KHR_SWAPCHAIN_EXTENSION_NAME);
VK_DEVICE_FUNCTION_FROM_EXTENSION(vkDestroySwapchainKHR, VK_KHR_SWAPCHAIN_EXTENSION_NAME);
VK_DEVICE_FUNCTION_FROM_EXTENSION(vkGetSwapchainImagesKHR, VK_KHR_SWAPCHAIN_EXTENSION_NAME);

VK_DEVICE_FUNCTION_FROM_EXTENSION(vkAcquireNextImageKHR, VK_KHR_SWAPCHAIN_EXTENSION_NAME);

VK_DEVICE_FUNCTION_FROM_EXTENSION(vkQueuePresentKHR, VK_KHR_SWAPCHAIN_EXTENSION_NAME);

#undef VK_DEVICE_FUNCTION_FROM_EXTENSION