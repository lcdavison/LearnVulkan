#pragma once

#include <vulkan/vulkan_core.h>
#include <cstdint>

#ifdef VULKAN_WRAPPER_EXPORT
    #define VULKAN_WRAPPER_API __declspec(dllexport)
#else
    #define VULKAN_WRAPPER_API __declspec(dllimport)
#endif

namespace Vulkan
{
    inline VkDescriptorBufferInfo const DescriptorBufferInfo(VkBuffer const Buffer, VkDeviceSize const OffsetInBytes, VkDeviceSize const RangeInBytes)
    {
        return VkDescriptorBufferInfo { Buffer, OffsetInBytes, RangeInBytes };
    }

    inline VkWriteDescriptorSet WriteDescriptorSet(VkDescriptorSet const DescriptorSet, VkDescriptorType const DescriptorType, 
                                                   std::uint32_t const BindingIndex, std::uint32_t const DescriptorCount, 
                                                   VkDescriptorBufferInfo const * const BufferDescriptors, VkDescriptorImageInfo const * const ImageDescriptors, 
                                                   std::uint32_t const ArrayElementIndex = 0u, VkBufferView const * const TexelBufferViews = nullptr)
    {
        return VkWriteDescriptorSet { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr, DescriptorSet, BindingIndex, ArrayElementIndex, DescriptorCount, DescriptorType, ImageDescriptors, BufferDescriptors, TexelBufferViews };
    }

    inline constexpr VkComponentMapping ComponentMapping(VkComponentSwizzle const Red = VK_COMPONENT_SWIZZLE_IDENTITY, VkComponentSwizzle const Green = VK_COMPONENT_SWIZZLE_IDENTITY, 
                                                         VkComponentSwizzle const Blue = VK_COMPONENT_SWIZZLE_IDENTITY, VkComponentSwizzle const Alpha = VK_COMPONENT_SWIZZLE_IDENTITY)
    {
        return VkComponentMapping { Red, Green, Blue, Alpha };
    }

    inline constexpr VkImageSubresourceRange ImageSubResourceRange(VkImageAspectFlags const AspectFlags,
                                                                   std::uint32_t const MipMapLevelCount, std::uint32_t const ArrayLayerCount, 
                                                                   std::uint32_t const BaseMipMapLevelIndex = 0u, std::uint32_t const BaseArrayLayerIndex = 0u)
    {
        return VkImageSubresourceRange { AspectFlags, BaseMipMapLevelIndex, MipMapLevelCount, BaseArrayLayerIndex, ArrayLayerCount };
    }

    inline constexpr VkBufferCreateInfo Buffer(VkDeviceSize const SizeInBytes, VkBufferUsageFlags const UsageFlags, VkSharingMode const SharingMode = VK_SHARING_MODE_EXCLUSIVE, std::uint32_t const QueueFamilyIndexCount = 0u, std::uint32_t const * const QueueFamilyIndices = nullptr, VkBufferCreateFlags const Flags = 0u)
    {
        return VkBufferCreateInfo { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO, nullptr, Flags, SizeInBytes, UsageFlags, SharingMode, QueueFamilyIndexCount, QueueFamilyIndices };
    }

    inline constexpr VkImageCreateInfo Image(VkImageType const Type, VkFormat const Format,
                                             VkImageUsageFlags const UsageFlags, VkExtent3D const & Extent,
                                             VkImageLayout const InitialImageLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                                             std::uint32_t const MipLevelCount = 1u, std::uint32_t const ArrayLayerCount = 1u,
                                             VkSampleCountFlags const SampleCount = VK_SAMPLE_COUNT_1_BIT, VkImageTiling const Tiling = VK_IMAGE_TILING_OPTIMAL,
                                             VkSharingMode SharingMode = VK_SHARING_MODE_EXCLUSIVE,
                                             std::uint32_t const QueueFamilyIndexCount = 0u, std::uint32_t const * const QueueFamilyIndices = nullptr,
                                             VkImageCreateFlags const Flags = 0u)
    {
        return VkImageCreateInfo { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO, nullptr, Flags, Type, Format, Extent, MipLevelCount, ArrayLayerCount, static_cast<VkSampleCountFlagBits>(SampleCount), Tiling, UsageFlags, SharingMode, QueueFamilyIndexCount, QueueFamilyIndices, InitialImageLayout };
    }

    inline constexpr VkImageViewCreateInfo ImageView(VkImage const Image, VkImageViewType const ViewType, VkFormat const Format, VkComponentMapping const & ComponentMapping, VkImageSubresourceRange const & SubResourceRange, VkImageViewCreateFlags const Flags = 0u)
    {
        return VkImageViewCreateInfo { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO, nullptr, Flags, Image, ViewType, Format, ComponentMapping, SubResourceRange };
    }

    inline VkFramebufferCreateInfo FrameBuffer(VkExtent3D const & Extents, VkRenderPass const RenderPass, std::uint32_t const AttachmentCount, VkImageView const * const Attachments, VkFramebufferCreateFlags const Flags = 0u)
    {
        return VkFramebufferCreateInfo { VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO, nullptr, Flags, RenderPass, AttachmentCount, Attachments, Extents.width, Extents.height, Extents.depth };
    }

    inline constexpr VkDescriptorSetLayoutBinding DescriptorSetBinding(VkDescriptorType const DescriptorType, std::uint32_t const DescriptorCount, std::uint32_t const BindingIndex, VkShaderStageFlags const ShaderStageFlags, VkSampler const * const ImmutableSamplers = nullptr)
    {
        return VkDescriptorSetLayoutBinding { BindingIndex, DescriptorType, DescriptorCount, ShaderStageFlags, ImmutableSamplers };
    }

    inline VkDescriptorSetLayoutCreateInfo const DescriptorSetLayout(std::uint32_t const BindingCount, VkDescriptorSetLayoutBinding const * const Bindings, VkDescriptorSetLayoutCreateFlags const Flags = 0u)
    {
        return VkDescriptorSetLayoutCreateInfo { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO, nullptr, Flags, BindingCount, Bindings };
    }

    inline VkPipelineLayoutCreateInfo const PipelineLayout(std::uint32_t const DescriptorSetLayoutCount, VkDescriptorSetLayout const * const DescriptorSetLayouts, std::uint32_t const PushConstantCount = 0u, VkPushConstantRange const * const PushConstants = nullptr, VkPipelineLayoutCreateFlags const Flags = 0u)
    {
        return VkPipelineLayoutCreateInfo { VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO, nullptr, Flags, DescriptorSetLayoutCount, DescriptorSetLayouts, PushConstantCount, PushConstants };
    }

    inline constexpr VkCommandBufferBeginInfo CommandBufferBegin(VkCommandBufferUsageFlags const UsageFlags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT, VkCommandBufferInheritanceInfo const * const InheritanceInfo = nullptr)
    {
        return VkCommandBufferBeginInfo { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO, nullptr, UsageFlags, InheritanceInfo };
    }

    inline VkSubmitInfo const SubmitInfo(std::uint32_t const CommandBufferCount, VkCommandBuffer const * const CommandBuffers,
                                         std::uint32_t const SignalSemaphoreCount = 0u, VkSemaphore const * const SignalSemaphores = nullptr,
                                         std::uint32_t const WaitSemaphoreCount = 0u, VkSemaphore const * const WaitSemaphores = nullptr,
                                         VkPipelineStageFlags const * const WaitPipelineStageMask = nullptr)
    {
        return VkSubmitInfo { VK_STRUCTURE_TYPE_SUBMIT_INFO, nullptr, WaitSemaphoreCount, WaitSemaphores, WaitPipelineStageMask, CommandBufferCount, CommandBuffers, SignalSemaphoreCount, SignalSemaphores };
    }

    inline VkRenderPassCreateInfo RenderPass(std::uint32_t const AttachmentCount, VkAttachmentDescription const * const Attachments,
                                             std::uint32_t const SubpassCount, VkSubpassDescription const * const Subpasses,
                                             std::uint32_t const DependencyCount = 0u, VkSubpassDependency const * const Dependencies = nullptr,
                                             VkRenderPassCreateFlags const Flags = 0u)
    {
        return VkRenderPassCreateInfo { VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO, nullptr, Flags, AttachmentCount, Attachments, SubpassCount, Subpasses, DependencyCount, Dependencies };
    }

    inline constexpr VkAttachmentDescription Attachment(VkFormat const Format,
                                                        VkImageLayout const InitialLayout, VkImageLayout const FinalLayout,
                                                        VkAttachmentLoadOp const LoadOp, VkAttachmentStoreOp const StoreOp,
                                                        VkSampleCountFlags const SampleCount = VK_SAMPLE_COUNT_1_BIT,
                                                        VkAttachmentLoadOp const StencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE, VkAttachmentStoreOp const StencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
                                                        VkAttachmentDescriptionFlags const Flags = 0u)
    {
        return VkAttachmentDescription { Flags, Format, static_cast<VkSampleCountFlagBits>(SampleCount), LoadOp, StoreOp, StencilLoadOp, StencilStoreOp, InitialLayout, FinalLayout };
    }

    inline VkSubpassDescription const Subpass(VkPipelineBindPoint const PipelineBindPoint,
                                              std::uint32_t const ColourAttachmentCount, VkAttachmentReference const * const ColourAttachments,
                                              VkAttachmentReference const * const DepthStencilAttachment = nullptr,
                                              VkAttachmentReference const * const ResolveAttachments = nullptr,
                                              std::uint32_t const InputAttachmentCount = 0u, VkAttachmentReference const * const InputAttachments = nullptr,
                                              std::uint32_t const PreserveAttachmentCount = 0u, std::uint32_t const * const PreserveAttachments = nullptr,
                                              VkSubpassDescriptionFlags const Flags = 0u)
    {
        return VkSubpassDescription { Flags, PipelineBindPoint, InputAttachmentCount, InputAttachments, ColourAttachmentCount, ColourAttachments, ResolveAttachments, DepthStencilAttachment, PreserveAttachmentCount, PreserveAttachments };
    }

    inline VkRenderPassBeginInfo RenderPassBegin(VkRenderPass const RenderPass, VkFramebuffer const FrameBuffer, VkRect2D const RenderArea, std::uint32_t const ClearValueCount = 0u, VkClearValue const * const ClearValues = nullptr)
    {
        return VkRenderPassBeginInfo { VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO, nullptr, RenderPass, FrameBuffer, RenderArea, ClearValueCount, ClearValues };
    }

    inline VkBufferMemoryBarrier BufferMemoryBarrier(VkBuffer Buffer, VkAccessFlags SourceAccessFlags, VkAccessFlags DestinationAccessFlags, VkDeviceSize const SizeInBytes = VK_WHOLE_SIZE, VkDeviceSize const OffsetInBytes = 0u)
    {
        return VkBufferMemoryBarrier { VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER, nullptr, SourceAccessFlags, DestinationAccessFlags, 0u, 0u, Buffer, OffsetInBytes, SizeInBytes };
    }

    inline constexpr VkVertexInputBindingDescription VertexInputBinding(std::uint32_t const BindingSlotIndex, std::uint32_t const StrideInBytes, VkVertexInputRate InputRate = VK_VERTEX_INPUT_RATE_VERTEX)
    {
        return VkVertexInputBindingDescription { BindingSlotIndex, StrideInBytes, InputRate };
    }

    inline constexpr VkVertexInputAttributeDescription VertexInputAttribute(std::uint32_t const Location, std::uint32_t const BindingSlotIndex, std::uint32_t const OffsetInBytes, VkFormat const Format = VK_FORMAT_R32G32B32_SFLOAT)
    {
        return VkVertexInputAttributeDescription { Location, BindingSlotIndex, Format, OffsetInBytes };
    }

    inline VkPipelineShaderStageCreateInfo const PipelineShaderStage(VkShaderStageFlags const ShaderStage, VkShaderModule const ShaderModule, char const * EntryPointName, VkSpecializationInfo const * const SpecializationInfo = nullptr, VkPipelineShaderStageCreateFlags const Flags = 0u)
    {
        return VkPipelineShaderStageCreateInfo { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, nullptr, Flags, static_cast<VkShaderStageFlagBits>(ShaderStage), ShaderModule, EntryPointName, SpecializationInfo };
    }

    inline constexpr VkPipelineVertexInputStateCreateInfo const VertexInputState(std::uint32_t const VertexBindingCount, VkVertexInputBindingDescription * const VertexBindings,
                                                                                 std::uint32_t const VertexAttributeCount, VkVertexInputAttributeDescription * const VertexAttributes,
                                                                                 VkPipelineVertexInputStateCreateFlags const Flags = 0u)
    {
        return VkPipelineVertexInputStateCreateInfo { VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO, nullptr, Flags, VertexBindingCount, VertexBindings, VertexAttributeCount, VertexAttributes };
    }

    inline constexpr VkPipelineInputAssemblyStateCreateInfo const InputAssemblyState(VkPrimitiveTopology const PrimitiveTopology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, VkBool32 const PrimitiveRestartEnabled = VK_FALSE, VkPipelineInputAssemblyStateCreateFlags const Flags = 0u)
    {
        return VkPipelineInputAssemblyStateCreateInfo { VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO, nullptr, Flags, PrimitiveTopology, PrimitiveRestartEnabled };
    }

    inline constexpr VkPipelineRasterizationStateCreateInfo const RasterizationState(VkCullModeFlags const CullMode, VkPolygonMode const PolygonMode = VK_POLYGON_MODE_FILL,
                                                                                     VkFrontFace const FrontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
                                                                                     VkBool32 const bEnableDepthBias = VK_FALSE, VkBool32 const bEnableDepthClamp = VK_FALSE,
                                                                                     float const DepthBiasFactor = 0.0f, float const DepthBiasSlopeFactor = 0.0f, float const DepthBiasClamp = 0.0f,
                                                                                     float const LineWidth = 1.0f, VkBool32 const bEnableRasterizerDiscard = VK_FALSE, VkPipelineRasterizationStateCreateFlags const Flags = 0u)
    {
        return VkPipelineRasterizationStateCreateInfo { VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO, nullptr, Flags, bEnableDepthClamp, bEnableRasterizerDiscard, PolygonMode, CullMode, FrontFace, bEnableDepthBias, DepthBiasFactor, DepthBiasClamp, DepthBiasSlopeFactor, LineWidth };
    }

    inline constexpr VkPipelineColorBlendAttachmentState const ColourAttachmentBlendState(VkBool32 const bEnableBlending = VK_FALSE,
                                                                                          VkBlendFactor const SourceColourBlendFactor = VK_BLEND_FACTOR_ONE, VkBlendFactor const DestinationColourBlendFactor = VK_BLEND_FACTOR_ZERO,
                                                                                          VkBlendOp const ColourBlendOperation = VK_BLEND_OP_ADD,
                                                                                          VkBlendFactor const SourceAlphaBlendFactor = VK_BLEND_FACTOR_ONE, VkBlendFactor const DestinationAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
                                                                                          VkBlendOp const AlphaBlendOperation = VK_BLEND_OP_ADD,
                                                                                          VkColorComponentFlags const ColourMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT)
    {
        return VkPipelineColorBlendAttachmentState { bEnableBlending, SourceColourBlendFactor, DestinationColourBlendFactor, ColourBlendOperation, SourceAlphaBlendFactor, DestinationAlphaBlendFactor, AlphaBlendOperation, ColourMask };
    }

    inline VkPipelineColorBlendStateCreateInfo const ColourBlendState(std::uint32_t const AttachmentCount, VkPipelineColorBlendAttachmentState const * const Attachments, VkBool32 const bEnableLogicOperation = VK_FALSE, VkLogicOp const LogicOperation = VK_LOGIC_OP_CLEAR, float const BlendConstants [4u] = nullptr, VkPipelineColorBlendStateCreateFlags const Flags = 0u)
    {
        return VkPipelineColorBlendStateCreateInfo { VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO, nullptr, Flags, bEnableLogicOperation, LogicOperation, AttachmentCount, Attachments,  { BlendConstants ? BlendConstants [0u] : 0.0f, BlendConstants ? BlendConstants [1u] : 0.0f, BlendConstants ? BlendConstants [2u] : 0.0f, BlendConstants ? BlendConstants [3u] : 0.0f } };
    }

    inline constexpr VkStencilOpState const StencilOpState()
    {
        return VkStencilOpState {};
    }

    inline constexpr VkPipelineDepthStencilStateCreateInfo const DepthStencilState(VkBool32 const bEnableDepthTest, VkBool32 const bEnableDepthWrite, VkCompareOp const DepthComparison,
                                                                                   VkBool32 const bEnableStencilTest = VK_FALSE, VkStencilOpState const FrontFaceStencilOperation = {}, VkStencilOpState const BackFaceStencilOperation = {},
                                                                                   VkBool32 const bEnableDepthBoundsTest = VK_FALSE, float const MinDepthBound = 0.0f, float const MaxDepthBound = 1.0f,
                                                                                   VkPipelineDepthStencilStateCreateFlags const Flags = 0u)
    {
        return VkPipelineDepthStencilStateCreateInfo { VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO, nullptr, Flags, bEnableDepthTest, bEnableDepthWrite, DepthComparison, bEnableDepthBoundsTest, bEnableStencilTest, FrontFaceStencilOperation, BackFaceStencilOperation, MinDepthBound, MaxDepthBound };
    }

    inline VkPipelineMultisampleStateCreateInfo const MultiSampleState(VkSampleCountFlags const SampleCount = VK_SAMPLE_COUNT_1_BIT, VkSampleMask const * const SampleMasks = nullptr, VkBool32 const bEnableAlphaToCoverage = VK_FALSE, VkBool32 const bEnableAlphaToOne = VK_FALSE, VkBool32 const bEnableSampleShading = VK_FALSE, float const MinSampleShadingFraction = 1.0f, VkPipelineMultisampleStateCreateFlags const Flags = 0u)
    {
        return VkPipelineMultisampleStateCreateInfo { VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO, nullptr, Flags, static_cast<VkSampleCountFlagBits>(SampleCount), bEnableSampleShading, MinSampleShadingFraction, SampleMasks, bEnableAlphaToCoverage, bEnableAlphaToOne };
    }

    inline VkPipelineViewportStateCreateInfo const ViewportState(std::uint32_t const ViewportCount, VkViewport const * const Viewports, std::uint32_t const ScissorRectangleCount, VkRect2D const * const ScissorRectangles, VkPipelineViewportStateCreateFlags const Flags = 0u)
    {
        return VkPipelineViewportStateCreateInfo { VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO, nullptr, Flags, ViewportCount, Viewports, ScissorRectangleCount, ScissorRectangles };
    }

    inline VkPipelineDynamicStateCreateInfo const DynamicState(std::uint32_t const DynamicStateCount = 0u, VkDynamicState const * const DynamicStates = nullptr, VkPipelineDynamicStateCreateFlags const Flags = 0u)
    {
        return VkPipelineDynamicStateCreateInfo { VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO, nullptr, Flags, DynamicStateCount, DynamicStates };
    }

    inline VkGraphicsPipelineCreateInfo const GraphicsPipelineState(VkPipelineLayout const PipelineLayout, VkRenderPass const RenderPass, std::uint32_t const SubPassIndex,
                                                                    std::uint32_t const ShaderStageCount, VkPipelineShaderStageCreateInfo const * const ShaderStages,
                                                                    VkPipelineVertexInputStateCreateInfo const * const VertexInputState, VkPipelineInputAssemblyStateCreateInfo const * const InputAssemblyState,
                                                                    VkPipelineViewportStateCreateInfo const * const ViewportState, VkPipelineRasterizationStateCreateInfo const * const RasterizationState,
                                                                    VkPipelineColorBlendStateCreateInfo const * const BlendState, VkPipelineDepthStencilStateCreateInfo const * const DepthStencilState,
                                                                    VkPipelineDynamicStateCreateInfo const * const DynamicState, VkPipelineMultisampleStateCreateInfo const * const MultiSampleState,
                                                                    VkPipelineCreateFlags const Flags = 0u)
    {
        return VkGraphicsPipelineCreateInfo { VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO, nullptr, Flags, ShaderStageCount, ShaderStages, VertexInputState, InputAssemblyState, nullptr, ViewportState, RasterizationState, MultiSampleState, DepthStencilState, BlendState, DynamicState, PipelineLayout, RenderPass, SubPassIndex, VK_NULL_HANDLE, -1l };
    }
}

VULKAN_WRAPPER_API bool const InitialiseVulkanWrapper();
VULKAN_WRAPPER_API bool const ShutdownVulkanWrapper();

VULKAN_WRAPPER_API bool const LoadExportedFunctions();
VULKAN_WRAPPER_API bool const LoadGlobalFunctions();
VULKAN_WRAPPER_API bool const LoadInstanceFunctions(VkInstance Instance);
VULKAN_WRAPPER_API bool const LoadDeviceFunctions(VkDevice Device);
VULKAN_WRAPPER_API bool const LoadInstanceExtensionFunctions(VkInstance Instance, std::uint32_t const ExtensionNameCount, char const * const * ExtensionNames);
VULKAN_WRAPPER_API bool const LoadDeviceExtensionFunctions(VkDevice Device, std::uint32_t const ExtensionNameCount, char const * const * ExtensionNames);

VULKAN_WRAPPER_API VkResult vkEnumerateInstanceExtensionProperties(char const * pLayerName, std::uint32_t * pPropertyCount, VkExtensionProperties * pProperties);
VULKAN_WRAPPER_API VkResult vkEnumerateInstanceLayerProperties(std::uint32_t * pPropertyCount, VkLayerProperties * pProperties);
VULKAN_WRAPPER_API VkResult vkEnumeratePhysicalDevices(VkInstance instance, std::uint32_t * pPhysicalDeviceCount, VkPhysicalDevice * pPhysicalDevice);
VULKAN_WRAPPER_API VkResult vkEnumerateDeviceExtensionProperties(VkPhysicalDevice device, char const * pLayerName, std::uint32_t * pPropertyCount, VkExtensionProperties * pProperties);
VULKAN_WRAPPER_API VkResult vkEnumerateDeviceLayerProperties(VkPhysicalDevice device, std::uint32_t * pPropertyCount, VkLayerProperties * pProperties);

VULKAN_WRAPPER_API void vkGetPhysicalDeviceProperties(VkPhysicalDevice physicalDevice, VkPhysicalDeviceProperties * pProperties);
VULKAN_WRAPPER_API void vkGetPhysicalDeviceFeatures(VkPhysicalDevice physicalDevice, VkPhysicalDeviceFeatures * pFeatures);
VULKAN_WRAPPER_API void vkGetPhysicalDeviceMemoryProperties(VkPhysicalDevice physicalDevice, VkPhysicalDeviceMemoryProperties * pMemoryProperties);
VULKAN_WRAPPER_API void vkGetPhysicalDeviceQueueFamilyProperties(VkPhysicalDevice physicalDevice, std::uint32_t * pQueueFamilyPropertyCount, VkQueueFamilyProperties * pQueueFamilyProperties);
VULKAN_WRAPPER_API VkResult vkGetPhysicalDeviceSurfaceSupportKHR(VkPhysicalDevice physicalDevice, std::uint32_t queueFamilyIndex, VkSurfaceKHR surface, VkBool32 * pSupported);
VULKAN_WRAPPER_API VkResult vkGetPhysicalDeviceSurfaceFormatsKHR(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, std::uint32_t * pFormatCount, VkSurfaceFormatKHR * pSurfaceFormat);
VULKAN_WRAPPER_API VkResult vkGetPhysicalDeviceSurfaceCapabilitiesKHR(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, VkSurfaceCapabilitiesKHR * pSurfaceCapabilities);
VULKAN_WRAPPER_API void vkGetDeviceQueue(VkDevice device, std::uint32_t queueFamilyIndex, std::uint32_t queueIndex, VkQueue * pQueue);
VULKAN_WRAPPER_API void vkGetBufferMemoryRequirements(VkDevice device, VkBuffer buffer, VkMemoryRequirements * pMemoryRequirements);
VULKAN_WRAPPER_API void vkGetImageMemoryRequirements(VkDevice device, VkImage image, VkMemoryRequirements * pMemoryRequirements);
VULKAN_WRAPPER_API VkResult vkGetFenceStatus(VkDevice device, VkFence fence);
VULKAN_WRAPPER_API VkResult vkGetEventStatus(VkDevice device, VkEvent event);
VULKAN_WRAPPER_API VkResult vkGetSwapchainImagesKHR(VkDevice device, VkSwapchainKHR swapchain, std::uint32_t * pSwapchainImageCount, VkImage * pSwapchainImages);

VULKAN_WRAPPER_API VkResult vkCreateInstance(VkInstanceCreateInfo const * pCreateInfo, VkAllocationCallbacks const * pAllocator, VkInstance * pInstance);
VULKAN_WRAPPER_API VkResult vkCreateDevice(VkPhysicalDevice physicalDevice, VkDeviceCreateInfo const * pCreateInfo, VkAllocationCallbacks const * pAllocator, VkDevice * pDevice);
VULKAN_WRAPPER_API VkResult vkCreateCommandPool(VkDevice device, VkCommandPoolCreateInfo const * pCreateInfo, VkAllocationCallbacks const * pAllocator, VkCommandPool * pCommandPool);
VULKAN_WRAPPER_API VkResult vkCreateSemaphore(VkDevice device, VkSemaphoreCreateInfo const * pCreateInfo, VkAllocationCallbacks const * pAllocator, VkSemaphore * pSemaphore);
VULKAN_WRAPPER_API VkResult vkCreateFence(VkDevice device, VkFenceCreateInfo const * pCreateInfo, VkAllocationCallbacks const * pAllocator, VkFence * pFence);
VULKAN_WRAPPER_API VkResult vkCreateEvent(VkDevice device, VkEventCreateInfo const * pCreateInfo, VkAllocationCallbacks const * pAllocator, VkEvent * pEvent);
VULKAN_WRAPPER_API VkResult vkCreateRenderPass(VkDevice device, VkRenderPassCreateInfo const * pCreateInfo, VkAllocationCallbacks const * pAllocator, VkRenderPass * pRenderPass);
VULKAN_WRAPPER_API VkResult vkCreateFramebuffer(VkDevice device, VkFramebufferCreateInfo const * pCreateInfo, VkAllocationCallbacks const * pAllocator, VkFramebuffer * pFramebuffer);
VULKAN_WRAPPER_API VkResult vkCreateShaderModule(VkDevice device, VkShaderModuleCreateInfo const * pCreateInfo, VkAllocationCallbacks const * pAllocator, VkShaderModule * pShaderModule);
VULKAN_WRAPPER_API VkResult vkCreateDescriptorPool(VkDevice device, VkDescriptorPoolCreateInfo const * pCreateInfo, VkAllocationCallbacks const * pAllocator, VkDescriptorPool * pDescriptorPool);
VULKAN_WRAPPER_API VkResult vkCreateDescriptorSetLayout(VkDevice device, VkDescriptorSetLayoutCreateInfo const * pCreateInfo, VkAllocationCallbacks const * pAllocator, VkDescriptorSetLayout * pSetLayout);
VULKAN_WRAPPER_API VkResult vkCreatePipelineLayout(VkDevice device, VkPipelineLayoutCreateInfo const * pCreateInfo, VkAllocationCallbacks const * pAllocator, VkPipelineLayout * pPipelineLayout);
VULKAN_WRAPPER_API VkResult vkCreateGraphicsPipelines(VkDevice device, VkPipelineCache pipelineCache, std::uint32_t createInfoCount, VkGraphicsPipelineCreateInfo const * pCreateInfos, VkAllocationCallbacks const * pAllocator, VkPipeline * pPipelines);
VULKAN_WRAPPER_API VkResult vkCreateBuffer(VkDevice device, VkBufferCreateInfo const * pCreateInfo, VkAllocationCallbacks const * pAllocator, VkBuffer * pBuffer);
VULKAN_WRAPPER_API VkResult vkCreateImage(VkDevice device, VkImageCreateInfo const * pCreateInfo, VkAllocationCallbacks const * pAllocator, VkImage * pImage);
VULKAN_WRAPPER_API VkResult vkCreateBufferView(VkDevice device, VkBufferViewCreateInfo const * pCreateInfo, VkAllocationCallbacks const * pAllocator, VkBufferView * pView);
VULKAN_WRAPPER_API VkResult vkCreateImageView(VkDevice device, VkImageViewCreateInfo const * pCreateInfo, VkAllocationCallbacks const * pAllocator, VkImageView * pImageView);
VULKAN_WRAPPER_API VkResult vkCreateSwapchainKHR(VkDevice device, VkSwapchainCreateInfoKHR const * pCreateInfo, VkAllocationCallbacks const * pAllocator, VkSwapchainKHR * pSwapchain);

VULKAN_WRAPPER_API void vkDestroyInstance(VkInstance instance, VkAllocationCallbacks const * pAllocator);
VULKAN_WRAPPER_API void vkDestroyDevice(VkDevice device, VkAllocationCallbacks const * pAllocator);
VULKAN_WRAPPER_API void vkDestroyCommandPool(VkDevice device, VkCommandPool commandPool, VkAllocationCallbacks const * pAllocator);
VULKAN_WRAPPER_API void vkDestroySemaphore(VkDevice device, VkSemaphore semaphore, VkAllocationCallbacks const * pAllocator);
VULKAN_WRAPPER_API void vkDestroyFence(VkDevice device, VkFence fence, VkAllocationCallbacks const * pAllocator);
VULKAN_WRAPPER_API void vkDestroyEvent(VkDevice device, VkEvent event, VkAllocationCallbacks const * pAllocator);
VULKAN_WRAPPER_API void vkDestroyRenderPass(VkDevice device, VkRenderPass renderPass, VkAllocationCallbacks const * pAllocator);
VULKAN_WRAPPER_API void vkDestroyFramebuffer(VkDevice device, VkFramebuffer framebuffer, VkAllocationCallbacks const * pAllocator);
VULKAN_WRAPPER_API void vkDestroyShaderModule(VkDevice device, VkShaderModule shaderModule, VkAllocationCallbacks const * pAllocator);
VULKAN_WRAPPER_API void vkDestroyDescriptorPool(VkDevice device, VkDescriptorPool descriptorPool, VkAllocationCallbacks const * pAllocator);
VULKAN_WRAPPER_API void vkDestroyDescriptorSetLayout(VkDevice device, VkDescriptorSetLayout descriptorSetLayout, VkAllocationCallbacks const * pAllocator);
VULKAN_WRAPPER_API void vkDestroyPipelineLayout(VkDevice device, VkPipelineLayout pipelineLayout, VkAllocationCallbacks const * pAllocator);
VULKAN_WRAPPER_API void vkDestroyPipeline(VkDevice device, VkPipeline pipeline, VkAllocationCallbacks const * pAllocator);
VULKAN_WRAPPER_API void vkDestroyBuffer(VkDevice device, VkBuffer buffer, VkAllocationCallbacks const * pAllocator);
VULKAN_WRAPPER_API void vkDestroyImage(VkDevice device, VkImage image, VkAllocationCallbacks const * pAllocator);
VULKAN_WRAPPER_API void vkDestroyBufferView(VkDevice device, VkBufferView bufferView, VkAllocationCallbacks const * pAllocator);
VULKAN_WRAPPER_API void vkDestroyImageView(VkDevice device, VkImageView imageView, VkAllocationCallbacks const * pAllocator);
VULKAN_WRAPPER_API void vkDestroySurfaceKHR(VkInstance instance, VkSurfaceKHR surface, VkAllocationCallbacks const * pAllocator);
VULKAN_WRAPPER_API void vkDestroySwapchainKHR(VkDevice device, VkSwapchainKHR swapchain, VkAllocationCallbacks const * pAllocator);

VULKAN_WRAPPER_API VkResult vkBindBufferMemory(VkDevice device, VkBuffer buffer, VkDeviceMemory memory, VkDeviceSize memoryOffset);
VULKAN_WRAPPER_API VkResult vkBindImageMemory(VkDevice device, VkImage image, VkDeviceMemory memory, VkDeviceSize memoryOffset);

VULKAN_WRAPPER_API VkResult vkMapMemory(VkDevice device, VkDeviceMemory memory, VkDeviceSize offset, VkDeviceSize size, VkMemoryMapFlags flags, void ** ppData);
VULKAN_WRAPPER_API void vkUnmapMemory(VkDevice device, VkDeviceMemory memory);

VULKAN_WRAPPER_API VkResult vkWaitForFences(VkDevice device, std::uint32_t fenceCount, VkFence const * pFences, VkBool32 waitAll, std::uint64_t timeout);

VULKAN_WRAPPER_API VkResult vkResetFences(VkDevice device, std::uint32_t fenceCount, VkFence const * pFences);
VULKAN_WRAPPER_API VkResult vkResetCommandPool(VkDevice device, VkCommandPool commandPool, VkCommandPoolResetFlags flags);
VULKAN_WRAPPER_API VkResult vkResetCommandBuffer(VkCommandBuffer commandBuffer, VkCommandBufferResetFlags flags);

VULKAN_WRAPPER_API VkResult vkAllocateCommandBuffers(VkDevice device, VkCommandBufferAllocateInfo const * pAllocateInfo, VkCommandBuffer * pCommandBuffers);
VULKAN_WRAPPER_API VkResult vkAllocateDescriptorSets(VkDevice device, VkDescriptorSetAllocateInfo const * pAllocateInfo, VkDescriptorSet * pDescriptorSets);
VULKAN_WRAPPER_API VkResult vkAllocateMemory(VkDevice device, VkMemoryAllocateInfo const * pAllocateInfo, VkAllocationCallbacks const * pAllocator, VkDeviceMemory * pMemory);

VULKAN_WRAPPER_API void vkFreeCommandBuffers(VkDevice device, VkCommandPool commandPool, std::uint32_t commandBufferCount, VkCommandBuffer * pCommandBuffers);
VULKAN_WRAPPER_API void vkFreeDescriptorSets(VkDevice device, VkDescriptorPool descriptorPool, std::uint32_t descriptorSetCount, VkDescriptorSet * pDescriptorSets);
VULKAN_WRAPPER_API void vkFreeMemory(VkDevice device, VkDeviceMemory memory, VkAllocationCallbacks const * pAllocator);

VULKAN_WRAPPER_API VkResult vkBeginCommandBuffer(VkCommandBuffer commandBuffer, VkCommandBufferBeginInfo const * pBeginInfo);

VULKAN_WRAPPER_API VkResult vkEndCommandBuffer(VkCommandBuffer commandBuffer);

VULKAN_WRAPPER_API VkResult vkDeviceWaitIdle(VkDevice device);

VULKAN_WRAPPER_API void vkUpdateDescriptorSets(VkDevice device,
                                               std::uint32_t descriptorWriteCount, VkWriteDescriptorSet const * pDescriptorWrites,
                                               std::uint32_t descriptorCopyCount, VkCopyDescriptorSet const * pDescriptorCopies);

VULKAN_WRAPPER_API VkResult vkQueueSubmit(VkQueue queue, std::uint32_t submitCount, VkSubmitInfo const * pSubmits, VkFence fence);
VULKAN_WRAPPER_API VkResult vkQueuePresentKHR(VkQueue queue, VkPresentInfoKHR const * pPresentInfo);

VULKAN_WRAPPER_API VkResult vkAcquireNextImageKHR(VkDevice device, VkSwapchainKHR swapchain, std::uint64_t timeout, VkSemaphore semaphore, VkFence fence, std::uint32_t * pImageIndex);

VULKAN_WRAPPER_API void vkCmdClearColorImage(VkCommandBuffer commandBuffer, VkImage image, VkImageLayout imageLayout, VkClearColorValue const * pColor, std::uint32_t rangeCount, VkImageSubresourceRange const * pRanges);

VULKAN_WRAPPER_API void vkCmdPipelineBarrier(VkCommandBuffer commandBuffer,
                                             VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask,
                                             VkDependencyFlags dependencyFlags,
                                             std::uint32_t memoryBarrierCount, VkMemoryBarrier const * pMemoryBarriers,
                                             std::uint32_t bufferMemoryBarrierCount, VkBufferMemoryBarrier const * pBufferMemoryBarriers,
                                             std::uint32_t imageMemoryBarrierCount, VkImageMemoryBarrier const * pImageMemoryBarriers);

VULKAN_WRAPPER_API void vkCmdBeginRenderPass(VkCommandBuffer commandBuffer, VkRenderPassBeginInfo const * pRenderPassBegin, VkSubpassContents contents);
VULKAN_WRAPPER_API void vkCmdEndRenderPass(VkCommandBuffer commandBuffer);

VULKAN_WRAPPER_API void vkCmdBindPipeline(VkCommandBuffer commandBuffer, VkPipelineBindPoint pipelineBindPoint, VkPipeline pipeline);
VULKAN_WRAPPER_API void vkCmdBindDescriptorSets(VkCommandBuffer commandBuffer,
                                                VkPipelineBindPoint pipelineBindPoint, VkPipelineLayout layout,
                                                std::uint32_t firstSet,
                                                std::uint32_t descriptorSetCount, VkDescriptorSet const * pDescriptorSets,
                                                std::uint32_t dynamicOffsetCount, std::uint32_t const * pDynamicOffsets);
VULKAN_WRAPPER_API void vkCmdBindVertexBuffers(VkCommandBuffer commandBuffer, std::uint32_t firstBinding, std::uint32_t bindingCount, VkBuffer const * pBuffers, VkDeviceSize const * pOffsets);
VULKAN_WRAPPER_API void vkCmdBindIndexBuffer(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset, VkIndexType indexType);

VULKAN_WRAPPER_API void vkCmdDraw(VkCommandBuffer commandBuffer, std::uint32_t vertexCount, std::uint32_t instanceCount, std::uint32_t firstVertex, std::uint32_t firstInstance);
VULKAN_WRAPPER_API void vkCmdDrawIndexed(VkCommandBuffer commandBuffer, std::uint32_t indexCount, std::uint32_t instanceCount, std::uint32_t firstIndex, std::int32_t vertexOffset, std::uint32_t firstInstance);

VULKAN_WRAPPER_API void vkCmdSetViewport(VkCommandBuffer commandBuffer, std::uint32_t firstViewport, std::uint32_t viewportCount, VkViewport * pViewports);
VULKAN_WRAPPER_API void vkCmdSetScissor(VkCommandBuffer commandBuffer, std::uint32_t firstScissor, std::uint32_t scissorCount, VkRect2D * pScissors);
VULKAN_WRAPPER_API void vkCmdSetEvent(VkCommandBuffer commandBuffer, VkEvent event, VkPipelineStageFlags stageMask);

VULKAN_WRAPPER_API void vkCmdCopyBuffer(VkCommandBuffer commandBuffer, VkBuffer srcBuffer, VkBuffer dstBuffer, std::uint32_t regionCount, VkBufferCopy const * pRegions);