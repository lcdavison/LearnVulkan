#include "Graphics/Device.hpp"

#include "Graphics/Instance.hpp"
#include "Graphics/Memory.hpp"
#include "Utilities/Containers.hpp"

#include <vector>
#include <string>
#include <functional>

// implement a resource caching system, so we can reuse old vulkan resources

namespace Vulkan::Device::Private
{
    template<class TResourceType>
    struct ResourceManager
    {
        std::unordered_map<VkFence, std::vector<uint32>> FenceToPendingDeletionQueueMap = {};

        // the index returned from Add can be used as the handle
        SparseVector<TResourceType> Resources = {};

        bool DestroyResource(Vulkan::Device::DeviceState const & kDeviceState, uint32 const kResourceHandle);

        bool DestroyPendingResources(Vulkan::Device::DeviceState const & kDeviceState)
        {
            for (auto & [Fence, Handles] : FenceToPendingDeletionQueueMap)
            {
                VkResult const kFenceStatus = vkGetFenceStatus(kDeviceState.Device, Fence);

                if (kFenceStatus == VK_SUCCESS)
                {
                    for (uint32 const Handle : Handles)
                    {
                        DestroyResource(kDeviceState, Handle);
                    }

                    Handles.clear();
                }
            }

            return true;
        }
    };

    using BufferManager = ResourceManager<Vulkan::Resource::Buffer>;
    using ImageManager = ResourceManager<Vulkan::Resource::Image>;
    using ImageViewManager = ResourceManager<VkImageView>;
    using FrameBufferManager = ResourceManager<VkFramebuffer>;

    bool ResourceManager<Vulkan::Resource::Buffer>::DestroyResource(Vulkan::Device::DeviceState const & kDeviceState, uint32 const kResourceHandle)
    {
        Logging::Log(Logging::LogTypes::Info, String::Format(PBR_TEXT("DestroyBuffer: Destroying buffer [ID: %u]"), kResourceHandle));

        uint32 const kBufferIndex = { kResourceHandle - 1u };

        Vulkan::Resource::Buffer const & kBuffer = Resources [kBufferIndex];

        if (kBuffer.Resource != VK_NULL_HANDLE)
        {
            vkDestroyBuffer(kDeviceState.Device, kBuffer.Resource, nullptr);
        }

        Vulkan::Memory::Free(kBuffer.MemoryAllocationHandle);

        Resources.Remove(kBufferIndex);

        return true;
    }

    bool ResourceManager<Vulkan::Resource::Image>::DestroyResource(Vulkan::Device::DeviceState const & kDeviceState, uint32 const kResourceHandle)
    {
        Logging::Log(Logging::LogTypes::Info, String::Format(PBR_TEXT("DestroyImage: Destroying image [ID: %u]"), kResourceHandle));

        uint32 const kImageIndex = { kResourceHandle - 1u };

        Vulkan::Resource::Image const & kImage = Resources [kImageIndex];

        if (kImage.Resource != VK_NULL_HANDLE)
        {
            vkDestroyImage(kDeviceState.Device, kImage.Resource, nullptr);
        }

        Vulkan::Memory::Free(kImage.MemoryAllocationHandle);

        Resources.Remove(kImageIndex);

        return true;
    }

    bool ResourceManager<VkImageView>::DestroyResource(Vulkan::Device::DeviceState const & kDeviceState, uint32 const kResourceHandle)
    {
        Logging::Log(Logging::LogTypes::Info, String::Format(PBR_TEXT("DestroyImageView: Destroying image view [ID: %u]"), kResourceHandle));

        uint32 const kImageViewIndex = { kResourceHandle - 1u };

        VkImageView const kImageView = Resources [kImageViewIndex];

        if (kImageView != VK_NULL_HANDLE)
        {
            vkDestroyImageView(kDeviceState.Device, kImageView, nullptr);
        }

        Resources.Remove(kImageViewIndex);

        return true;
    }

    bool ResourceManager<VkFramebuffer>::DestroyResource(Vulkan::Device::DeviceState const & kDeviceState, uint32 const kResourceHandle)
    {
        //Logging::Log(Logging::LogTypes::Info, String::Format(PBR_TEXT("DestroyFrameBuffer: Destroying frame buffer [ID: %u]"), kResourceHandle));

        uint32 const kFrameBufferIndex = { kResourceHandle - 1u };

        VkFramebuffer const kFrameBuffer = Resources [kFrameBufferIndex];

        if (kFrameBuffer != VK_NULL_HANDLE)
        {
            vkDestroyFramebuffer(kDeviceState.Device, kFrameBuffer, nullptr);
        }

        Resources.Remove(kFrameBufferIndex);

        return true;
    }
}

static Vulkan::Device::Private::BufferManager BufferManager = {};
static Vulkan::Device::Private::ImageManager ImageManager = {};
static Vulkan::Device::Private::ImageViewManager ImageViewManager = {};
static Vulkan::Device::Private::FrameBufferManager FrameBufferManager = {};

static std::vector<char const *> const kRequiredExtensionNames =
{
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
};

/* TODO: Do layer checks? */
//static bool const HasRequiredLayers(VkPhysicalDevice Device)
//{
//    uint32 LayerCount = {};
//    VERIFY_VKRESULT(vkEnumerateDeviceLayerProperties(Device, &LayerCount, nullptr));
//
//    std::vector AvailableLayers = std::vector<VkLayerProperties>(LayerCount);
//
//    VERIFY_VKRESULT(vkEnumerateDeviceLayerProperties(Device, &LayerCount, AvailableLayers.data()));
//
//
//
//    return true;
//}

static bool const HasRequiredExtensions(VkPhysicalDevice const kPhysicalDevice)
{
    uint32 ExtensionCount = {};
    VERIFY_VKRESULT(vkEnumerateDeviceExtensionProperties(kPhysicalDevice, nullptr, &ExtensionCount, nullptr));

    std::vector AvailableExtensions = std::vector<VkExtensionProperties>(ExtensionCount);

    VERIFY_VKRESULT(vkEnumerateDeviceExtensionProperties(kPhysicalDevice, nullptr, &ExtensionCount, AvailableExtensions.data()));

#if defined(_DEBUG)
    {
        char const OutputPrefix [] = "Device Extension: ";

        std::string ExtensionName = std::string();
        ExtensionName.reserve(sizeof(OutputPrefix) + VK_MAX_EXTENSION_NAME_SIZE);

        for (VkExtensionProperties const & Extension : AvailableExtensions)
        {
            ExtensionName.clear();

            ExtensionName += OutputPrefix;
            ExtensionName += Extension.extensionName;
            ExtensionName += "\n";

            Platform::Windows::OutputDebugString(ExtensionName.c_str());
        }
    }
#endif

    uint16 MatchedExtensionCount = { 0u };

    for (char const * ExtensionName : kRequiredExtensionNames)
    {
        for (VkExtensionProperties const & Extension : AvailableExtensions)
        {
            if (std::strcmp(ExtensionName, Extension.extensionName) == 0l)
            {
                MatchedExtensionCount++;
                break;
            }
        }
    }

    return MatchedExtensionCount == kRequiredExtensionNames.size();
}

static bool const GetPhysicalDeviceList(VkInstance const kInstance, std::vector<VkPhysicalDevice> & OutputPhysicalDevices)
{
    bool bResult = false;

    uint32 DeviceCount = {};
    VERIFY_VKRESULT(vkEnumeratePhysicalDevices(kInstance, &DeviceCount, nullptr));

    if (DeviceCount > 0u)
    {
        OutputPhysicalDevices.resize(DeviceCount);

        VERIFY_VKRESULT(vkEnumeratePhysicalDevices(kInstance, &DeviceCount, OutputPhysicalDevices.data()));

        bResult = true;
    }
    else
    {
        Logging::FatalError(PBR_TEXT("Failed to find a device with Vulkan support."));
    }

    return bResult;
}

static bool const GetPhysicalDevice(VkInstance const kInstance, std::function<bool(VkPhysicalDeviceFeatures const &, VkPhysicalDeviceProperties const &)> const & kPhysicalDeviceRequirementPredicate, VkPhysicalDevice & OutputPhysicalDevice)
{
    bool bResult = false;

    std::vector PhysicalDevices = std::vector<VkPhysicalDevice>();
    if (::GetPhysicalDeviceList(kInstance, PhysicalDevices))
    {
        for (VkPhysicalDevice CurrentPhysicalDevice : PhysicalDevices)
        {
            VkPhysicalDeviceProperties DeviceProperties = {};
            vkGetPhysicalDeviceProperties(CurrentPhysicalDevice, &DeviceProperties);

            VkPhysicalDeviceFeatures DeviceFeatures = {};
            vkGetPhysicalDeviceFeatures(CurrentPhysicalDevice, &DeviceFeatures);
            
            if (kPhysicalDeviceRequirementPredicate(DeviceFeatures, DeviceProperties))
            {
#if defined(_DEBUG)
                std::basic_string Output = "Selected Device: ";
                Output += DeviceProperties.deviceName;
                Output += "\n";

                Platform::Windows::OutputDebugString(Output.c_str());
#endif

                OutputPhysicalDevice = CurrentPhysicalDevice;
                bResult = true;
                break;
            }
        }
    }

    return bResult;
}

static bool const GetQueueFamilyIndex(VkPhysicalDevice const kPhysicalDevice, VkQueueFlags const kRequiredFlags, bool const kbSupportPresentation, VkSurfaceKHR const kSurface, uint32 & OutputQueueFamilyIndex)
{
    bool bResult = false;

    uint32 PropertyCount = {};
    vkGetPhysicalDeviceQueueFamilyProperties(kPhysicalDevice, &PropertyCount, nullptr);

    std::vector Properties = std::vector<VkQueueFamilyProperties>(PropertyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(kPhysicalDevice, &PropertyCount, Properties.data());

    {
        uint32 CurrentFamilyIndex = {};
        for (VkQueueFamilyProperties const & FamilyProperties : Properties)
        {
            if (FamilyProperties.queueFlags & kRequiredFlags)
            {
                bool bSelectQueueFamily = true;

                if (kbSupportPresentation)
                {
                    VkBool32 SupportsPresentation = {};
                    VERIFY_VKRESULT(vkGetPhysicalDeviceSurfaceSupportKHR(kPhysicalDevice, CurrentFamilyIndex, kSurface, &SupportsPresentation));

                    bSelectQueueFamily = bSelectQueueFamily && SupportsPresentation == VK_TRUE;
                }

                if (bSelectQueueFamily)
                {
                    OutputQueueFamilyIndex = CurrentFamilyIndex;
                    bResult = true;
                    break;
                }
            }

            CurrentFamilyIndex++;
        }
    }

    return bResult;
}

bool const Vulkan::Device::CreateDevice(Vulkan::Instance::InstanceState const & InstanceState, Vulkan::Device::DeviceState & OutputState)
{
    /* Use this for atomicity, we don't want to partially fill OutputState */
    DeviceState IntermediateState = {};

    IntermediateState.PhysicalDeviceFeatures.samplerAnisotropy = VK_TRUE;

    bool bResult = ::GetPhysicalDevice(InstanceState.Instance,
                                       [](VkPhysicalDeviceFeatures const & Features, VkPhysicalDeviceProperties const & Properties)
                                       {
                                           return Properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU && Features.samplerAnisotropy == VK_TRUE;
                                       },
                                       IntermediateState.PhysicalDevice);

    if (bResult)
    {
        if (::HasRequiredExtensions(IntermediateState.PhysicalDevice) &&
            ::GetQueueFamilyIndex(IntermediateState.PhysicalDevice, VK_QUEUE_GRAPHICS_BIT, true, InstanceState.Surface, IntermediateState.GraphicsQueueFamilyIndex))
        {
            float const QueuePriority = 1.0f;

            VkDeviceQueueCreateInfo const kQueueCreateInfo = Vulkan::DeviceQueue(IntermediateState.GraphicsQueueFamilyIndex, 
                                                                                 1u, 
                                                                                 &QueuePriority);

            {
                VkDeviceCreateInfo const kCreateInfo = Vulkan::DeviceInfo(1u, &kQueueCreateInfo,
                                                                          &IntermediateState.PhysicalDeviceFeatures,
                                                                          static_cast<uint32>(kRequiredExtensionNames.size()), kRequiredExtensionNames.data());

                VERIFY_VKRESULT(vkCreateDevice(IntermediateState.PhysicalDevice, &kCreateInfo, nullptr, &IntermediateState.Device));
            }

            if (::LoadDeviceFunctions(IntermediateState.Device) &&
                ::LoadDeviceExtensionFunctions(IntermediateState.Device, static_cast<uint32>(kRequiredExtensionNames.size()), kRequiredExtensionNames.data()))
            {
                vkGetDeviceQueue(IntermediateState.Device, IntermediateState.GraphicsQueueFamilyIndex, 0u, &IntermediateState.GraphicsQueue);

                {
                    VkCommandPoolCreateInfo const kCreateInfo = Vulkan::CommandPoolInfo(IntermediateState.GraphicsQueueFamilyIndex, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

                    VERIFY_VKRESULT(vkCreateCommandPool(IntermediateState.Device, &kCreateInfo, nullptr, &IntermediateState.CommandPool));
                }

                OutputState = IntermediateState;
                bResult = true;
            }
        }
    }

    return bResult;
}

void Vulkan::Device::DestroyDevice(Vulkan::Device::DeviceState & DeviceState)
{
    VERIFY_VKRESULT(vkDeviceWaitIdle(DeviceState.Device));

    if (DeviceState.CommandPool)
    {
        vkDestroyCommandPool(DeviceState.Device, DeviceState.CommandPool, nullptr);
        DeviceState.CommandPool = VK_NULL_HANDLE;
    }

    if (DeviceState.Device)
    {
        vkDestroyDevice(DeviceState.Device, nullptr);
        DeviceState.Device = VK_NULL_HANDLE;
    }
}

void Vulkan::Device::CreateCommandBuffers(Vulkan::Device::DeviceState const & kDeviceState, VkCommandBufferLevel const kCommandBufferLevel, uint32 const kCommandBufferCount, std::vector<VkCommandBuffer> & OutputCommandBuffers)
{
    std::vector CommandBuffers = std::vector<VkCommandBuffer>(kCommandBufferCount);

    VkCommandBufferAllocateInfo const kAllocateInfo = Vulkan::CommandBufferAllocateInfo(kDeviceState.CommandPool, kCommandBufferCount, kCommandBufferLevel);

    VERIFY_VKRESULT(vkAllocateCommandBuffers(kDeviceState.Device, &kAllocateInfo, CommandBuffers.data()));

    OutputCommandBuffers = std::move(CommandBuffers);
}

void Vulkan::Device::DestroyCommandBuffers(Vulkan::Device::DeviceState const & kDeviceState, std::vector<VkCommandBuffer> & CommandBuffers)
{
    vkFreeCommandBuffers(kDeviceState.Device, kDeviceState.CommandPool, static_cast<uint32>(CommandBuffers.size()), CommandBuffers.data());

    for (VkCommandBuffer CommandBuffer : CommandBuffers)
    {
        CommandBuffer = VK_NULL_HANDLE;
    }
}

void Vulkan::Device::CreateSemaphore(Vulkan::Device::DeviceState const & kDeviceState, VkSemaphoreCreateFlags const kFlags, VkSemaphore & OutputSemaphore)
{
    VkSemaphoreCreateInfo const kCreateInfo = Vulkan::SemaphoreInfo(kFlags);

    VERIFY_VKRESULT(vkCreateSemaphore(kDeviceState.Device, &kCreateInfo, nullptr, &OutputSemaphore));
}

void Vulkan::Device::DestroySemaphore(Vulkan::Device::DeviceState const & kDeviceState, VkSemaphore & Semaphore)
{
    if (Semaphore != VK_NULL_HANDLE)
    {
        vkDestroySemaphore(kDeviceState.Device, Semaphore, nullptr);
        Semaphore = VK_NULL_HANDLE;
    }
}

void Vulkan::Device::CreateFence(Vulkan::Device::DeviceState const & kDeviceState, VkFenceCreateFlags const kFlags, VkFence & OutputFence)
{
    VkFenceCreateInfo const kCreateInfo = Vulkan::FenceInfo(kFlags);

    VERIFY_VKRESULT(vkCreateFence(kDeviceState.Device, &kCreateInfo, nullptr, &OutputFence));
}

void Vulkan::Device::DestroyFence(Vulkan::Device::DeviceState const & kDeviceState, VkFence & Fence)
{
    if (Fence != VK_NULL_HANDLE)
    {
        vkDestroyFence(kDeviceState.Device, Fence, nullptr);
        Fence = VK_NULL_HANDLE;
    }
}

void Vulkan::Device::CreateEvent(Vulkan::Device::DeviceState const & kDeviceState, VkEventCreateFlags const kFlags, VkEvent & OutputEvent)
{
    VkEventCreateInfo CreateInfo = {};
    CreateInfo.sType = VK_STRUCTURE_TYPE_EVENT_CREATE_INFO;
    CreateInfo.flags = kFlags;

    VERIFY_VKRESULT(vkCreateEvent(kDeviceState.Device, &CreateInfo, nullptr, &OutputEvent));
}

void Vulkan::Device::DestroyEvent(Vulkan::Device::DeviceState const & kDeviceState, VkEvent & Event)
{
    if (Event != VK_NULL_HANDLE)
    {
        vkDestroyEvent(kDeviceState.Device, Event, nullptr);
        Event = VK_NULL_HANDLE;
    }
}

void Vulkan::Device::CreateFrameBuffer(Vulkan::Device::DeviceState const & kDeviceState, Vulkan::Resource::FrameBufferDescriptor const & kDescriptor, uint32 & OutputFrameBufferHandle)
{
    VkExtent3D const kFrameBufferExtents = VkExtent3D { kDescriptor.Width, kDescriptor.Height, 1u };
    VkFramebufferCreateInfo const kCreateInfo = Vulkan::FrameBuffer(kFrameBufferExtents, kDescriptor.RenderPass, kDescriptor.AttachmentCount, kDescriptor.Attachments);

    VkFramebuffer NewFrameBuffer = {};
    VERIFY_VKRESULT(vkCreateFramebuffer(kDeviceState.Device, &kCreateInfo, nullptr, &NewFrameBuffer));

    uint32 const kNewFrameBufferHandle = { FrameBufferManager.Resources.Add(NewFrameBuffer) + 1u };

    OutputFrameBufferHandle = kNewFrameBufferHandle;
}

void Vulkan::Device::DestroyFrameBuffer(Vulkan::Device::DeviceState const & kDeviceState, uint32 const kFrameBufferHandle, VkFence const kFenceToWaitFor)
{
    if (kFrameBufferHandle == 0u)
    {
        Logging::Log(Logging::LogTypes::Error, PBR_TEXT("DestroyFrameBuffer: Invalid frame buffer handle. Cannot destroy NULL frame buffer"));
        return;
    }

    if (kFenceToWaitFor == VK_NULL_HANDLE)
    {
        FrameBufferManager.DestroyResource(kDeviceState, kFrameBufferHandle);
    }
    else
    {
        Logging::Log(Logging::LogTypes::Info, String::Format(PBR_TEXT("DestroyFrameBuffer: Queueing frame buffer for deletion [ID: %u]"), kFrameBufferHandle));

        std::vector<uint32> & DeletionQueue = FrameBufferManager.FenceToPendingDeletionQueueMap [kFenceToWaitFor];
        DeletionQueue.push_back(kFrameBufferHandle);
    }
}

/* keep this around until I can replace it with the new version throughout the code */
void Vulkan::Device::CreateBuffer(Vulkan::Device::DeviceState const & kDeviceState, uint64 const kSizeInBytes, VkBufferUsageFlags const kUsageFlags, VkMemoryPropertyFlags const kMemoryFlags, uint32 & OutputBufferHandle)
{
    Vulkan::Resource::Buffer NewBuffer = { kSizeInBytes, 0u, VK_NULL_HANDLE };

    VkBufferCreateInfo const CreateInfo = Vulkan::Buffer(kSizeInBytes, kUsageFlags);

    VERIFY_VKRESULT(vkCreateBuffer(kDeviceState.Device, &CreateInfo, nullptr, &NewBuffer.Resource));

    VkMemoryRequirements MemoryRequirements = {};
    vkGetBufferMemoryRequirements(kDeviceState.Device, NewBuffer.Resource, &MemoryRequirements);

    Vulkan::Memory::Allocate(kDeviceState, MemoryRequirements, kMemoryFlags, NewBuffer.MemoryAllocationHandle);

    Vulkan::Memory::AllocationInfo AllocationInfo = {};
    Vulkan::Memory::GetAllocationInfo(NewBuffer.MemoryAllocationHandle, AllocationInfo);

    VERIFY_VKRESULT(vkBindBufferMemory(kDeviceState.Device, NewBuffer.Resource, AllocationInfo.DeviceMemory, AllocationInfo.OffsetInBytes));

    uint32 const kNewBufferHandle = { BufferManager.Resources.Add(NewBuffer) + 1u };

    OutputBufferHandle = kNewBufferHandle;
}

void Vulkan::Device::CreateBuffer(Vulkan::Device::DeviceState const & kDeviceState, Vulkan::Resource::BufferDescriptor const & kDescriptor, uint32 & OutputBufferHandle)
{
    Vulkan::Resource::Buffer NewBuffer = Vulkan::Resource::Buffer { kDescriptor.SizeInBytes, 0u, VK_NULL_HANDLE };

    VkBufferCreateInfo const kCreateInfo = Vulkan::Buffer(kDescriptor.SizeInBytes, kDescriptor.UsageFlags, VK_SHARING_MODE_EXCLUSIVE, 0u, nullptr, kDescriptor.CreateFlags);
    
    VERIFY_VKRESULT(vkCreateBuffer(kDeviceState.Device, &kCreateInfo, nullptr, &NewBuffer.Resource));

    VkMemoryRequirements MemoryRequirements = {};
    vkGetBufferMemoryRequirements(kDeviceState.Device, NewBuffer.Resource, &MemoryRequirements);
    
    Vulkan::Memory::Allocate(kDeviceState, MemoryRequirements, kDescriptor.MemoryFlags, NewBuffer.MemoryAllocationHandle);

    Vulkan::Memory::AllocationInfo AllocationInfo = {};
    Vulkan::Memory::GetAllocationInfo(NewBuffer.MemoryAllocationHandle, AllocationInfo);

    VERIFY_VKRESULT(vkBindBufferMemory(kDeviceState.Device, NewBuffer.Resource, AllocationInfo.DeviceMemory, AllocationInfo.OffsetInBytes));

    uint32 const kNewBufferHandle = { BufferManager.Resources.Add(NewBuffer) + 1u };

    OutputBufferHandle = kNewBufferHandle;
}

void Vulkan::Device::DestroyBuffer(Vulkan::Device::DeviceState const & kDeviceState, uint32 const kBufferHandle, VkFence const kFenceToWaitFor)
{
    if (kBufferHandle == 0u)
    {
        Logging::Log(Logging::LogTypes::Error, PBR_TEXT("DestroyBuffer: Invalid buffer handle. Cannot destroy NULL buffer."));
        return;
    }

    if (kFenceToWaitFor == VK_NULL_HANDLE)
    {
        BufferManager.DestroyResource(kDeviceState, kBufferHandle);
    }
    else
    {
        Logging::Log(Logging::LogTypes::Info, String::Format(PBR_TEXT("DestroyBuffer: Queueing buffer for deletion [ID: %u]"), kBufferHandle));

        std::vector<uint32> & DeletionQueue = BufferManager.FenceToPendingDeletionQueueMap [kFenceToWaitFor];
        DeletionQueue.push_back(kBufferHandle);
    }
}

void Vulkan::Device::CreateImage(Vulkan::Device::DeviceState const & kDeviceState, Vulkan::ImageDescriptor const & kDescriptor, VkMemoryPropertyFlags const kMemoryFlags, uint32 & OutputImageHandle)
{
    Vulkan::Resource::Image NewImage = { 0u, nullptr, kDescriptor.Width, kDescriptor.Height };

    VkExtent3D const ImageExtents = VkExtent3D { kDescriptor.Width, kDescriptor.Height, kDescriptor.Depth };

    VkImageCreateInfo const CreateInfo = Vulkan::Image(kDescriptor.ImageType, kDescriptor.Format,
                                                       kDescriptor.UsageFlags, ImageExtents,
                                                       kDescriptor.bIsCPUWritable ? VK_IMAGE_LAYOUT_PREINITIALIZED : VK_IMAGE_LAYOUT_UNDEFINED,
                                                       kDescriptor.MipLevelCount, kDescriptor.ArrayLayerCount, kDescriptor.SampleCount,
                                                       kDescriptor.Tiling, VK_SHARING_MODE_EXCLUSIVE, 0u, nullptr, kDescriptor.Flags);

    VERIFY_VKRESULT(vkCreateImage(kDeviceState.Device, &CreateInfo, nullptr, &NewImage.Resource));

    VkMemoryRequirements MemoryRequirements = {};
    vkGetImageMemoryRequirements(kDeviceState.Device, NewImage.Resource, &MemoryRequirements);

    Vulkan::Memory::Allocate(kDeviceState, MemoryRequirements, kMemoryFlags, NewImage.MemoryAllocationHandle);

    Vulkan::Memory::AllocationInfo AllocationInfo = {};
    Vulkan::Memory::GetAllocationInfo(NewImage.MemoryAllocationHandle, AllocationInfo);

    VERIFY_VKRESULT(vkBindImageMemory(kDeviceState.Device, NewImage.Resource, AllocationInfo.DeviceMemory, AllocationInfo.OffsetInBytes));

    uint32 const kNewImageHandle = { ImageManager.Resources.Add(NewImage) + 1u };

    OutputImageHandle = kNewImageHandle;
}

void Vulkan::Device::DestroyImage(Vulkan::Device::DeviceState const & kDeviceState, uint32 const kImageHandle, VkFence const kFenceToWaitFor)
{
    if (kImageHandle == 0u)
    {
        Logging::Log(Logging::LogTypes::Error, PBR_TEXT("DestroyImage: Invalid image handle. Cannot destroy NULL image."));
        return;
    }

    if (kFenceToWaitFor == VK_NULL_HANDLE)
    {
        ImageManager.DestroyResource(kDeviceState, kImageHandle);
    }
    else
    {
        Logging::Log(Logging::LogTypes::Info, String::Format(PBR_TEXT("DestroyImage: Queueing image for deletion [ID: %u]"), kImageHandle));

        std::vector<uint32> & DeletionQueue = ImageManager.FenceToPendingDeletionQueueMap [kFenceToWaitFor];
        DeletionQueue.push_back(kImageHandle);
    }
}

void Vulkan::Device::CreateImageView(Vulkan::Device::DeviceState const & kDeviceState, uint32 const kImageHandle, Vulkan::ImageViewDescriptor const & kDescriptor, uint32 & OutputImageViewHandle)
{
    Vulkan::Resource::Image Image = {};
    Vulkan::Resource::GetImage(kImageHandle, Image);

    VkComponentMapping const ComponentMapping = kDescriptor.bReverseComponents 
        ? Vulkan::ComponentMapping(VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_R) 
        : Vulkan::ComponentMapping();

    VkImageSubresourceRange const SubResourceRange = Vulkan::ImageSubResourceRange(kDescriptor.AspectFlags, kDescriptor.MipLevelCount, kDescriptor.ArrayLayerCount);

    VkImageViewCreateInfo const CreateInfo = Vulkan::ImageView(Image.Resource, kDescriptor.ViewType, kDescriptor.Format, ComponentMapping, SubResourceRange);

    VkImageView NewImageView = {};  
    VERIFY_VKRESULT(vkCreateImageView(kDeviceState.Device, &CreateInfo, nullptr, &NewImageView));

    uint32 const kNewImageViewHandle = { ImageViewManager.Resources.Add(NewImageView) + 1u };

    OutputImageViewHandle = kNewImageViewHandle;
}

void Vulkan::Device::DestroyImageView(Vulkan::Device::DeviceState const & kDeviceState, uint32 const kImageViewHandle, VkFence const kFenceToWaitFor)
{
    if (kImageViewHandle == 0u)
    {
        Logging::Log(Logging::LogTypes::Info, PBR_TEXT("DestroyImageView: Invalid image view handle. Cannot destroy NULL image view."));
        return;
    }

    if (kFenceToWaitFor == VK_NULL_HANDLE)
    {
        ImageViewManager.DestroyResource(kDeviceState, kImageViewHandle);
    }
    else
    {
        Logging::Log(Logging::LogTypes::Info, String::Format(PBR_TEXT("DestroyImageView: Queueing image view for deletion [ID: %u]"), kImageViewHandle));

        ImageViewManager.FenceToPendingDeletionQueueMap [kFenceToWaitFor].push_back(kImageViewHandle);
    }
}

void Vulkan::Device::DestroyUnusedResources(Vulkan::Device::DeviceState const & kDeviceState)
{
    BufferManager.DestroyPendingResources(kDeviceState);
    FrameBufferManager.DestroyPendingResources(kDeviceState);
    ImageViewManager.DestroyPendingResources(kDeviceState);
    ImageManager.DestroyPendingResources(kDeviceState);
}

bool const Vulkan::Resource::GetBuffer(uint32 const kBufferHandle, Vulkan::Resource::Buffer & OutputBuffer)
{
    if (kBufferHandle == 0u)
    {
        Logging::Log(Logging::LogTypes::Error, PBR_TEXT("GetBuffer: Invalid buffer handle. Cannot get buffer for NULL handle."));
        return false;
    }

    //PBR_ASSERT(kBufferHandle <= BufferManager.Resources.size());

    uint32 const kBufferIndex = { kBufferHandle - 1u };
    OutputBuffer = BufferManager.Resources [kBufferIndex];

    return true;
}

bool const Vulkan::Resource::GetImage(uint32 const kImageHandle, Vulkan::Resource::Image & OutputImage)
{
    if (kImageHandle == 0u)
    {
        Logging::Log(Logging::LogTypes::Error, PBR_TEXT("GetImage: Invalid image handle. Cannot get image for NULL handle."));
        return false;
    }

    //PBR_ASSERT(kImageHandle <= ImageManager.Resources.size());

    uint32 const kImageIndex = { kImageHandle - 1u };

    OutputImage = ImageManager.Resources [kImageIndex];

    return true;
}

bool const Vulkan::Resource::GetImageView(uint32 const kImageViewHandle, VkImageView & OutputImageView)
{
    if (kImageViewHandle == 0u)
    {
        Logging::Log(Logging::LogTypes::Error, PBR_TEXT("GetImageView: Invalid image view handle. Cannot get image view for NULL handle."));
        return false;
    }

    //PBR_ASSERT(kImageViewHandle <= ImageViewManager.Resources.size());

    uint32 const kImageViewIndex = { kImageViewHandle - 1u };

    OutputImageView = ImageViewManager.Resources [kImageViewIndex];

    return true;
}

bool const Vulkan::Resource::GetFrameBuffer(uint32 const kFrameBufferHandle, VkFramebuffer & OutputFrameBuffer)
{
    if (kFrameBufferHandle == 0u)
    {
        Logging::Log(Logging::LogTypes::Error, PBR_TEXT("GetFrameBuffer: Invalid frame buffer handle. Cannot get frame buffer for NULL handle."));
        return false;
    }

    //PBR_ASSERT(kFrameBufferHandle <= FrameBufferManager.Resources.size());

    uint32 const kFrameBufferIndex = { kFrameBufferHandle - 1u };

    OutputFrameBuffer = FrameBufferManager.Resources [kFrameBufferIndex];

    return true;
}