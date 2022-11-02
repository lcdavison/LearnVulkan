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

        bool DestroyResource(Vulkan::Device::DeviceState const & DeviceState, uint32 const kResourceHandle);
    };

    using BufferManager = ResourceManager<Vulkan::Resource::Buffer>;
    using ImageManager = ResourceManager<Vulkan::Resource::Image>;
    using ImageViewManager = ResourceManager<VkImageView>;
    using FrameBufferManager = ResourceManager<VkFramebuffer>;

    bool ResourceManager<Vulkan::Resource::Buffer>::DestroyResource(Vulkan::Device::DeviceState const & DeviceState, uint32 const kResourceHandle)
    {
        Logging::Log(Logging::LogTypes::Info, String::Format(PBR_TEXT("DestroyBuffer: Destroying buffer [ID: %u]"), kResourceHandle));

        uint32 const kBufferIndex = kResourceHandle - 1u;

        Vulkan::Resource::Buffer const & kBuffer = Resources [kBufferIndex];

        if (kBuffer.Resource != VK_NULL_HANDLE)
        {
            vkDestroyBuffer(DeviceState.Device, kBuffer.Resource, nullptr);
        }

        Vulkan::Memory::Free(kBuffer.MemoryAllocationHandle);

        Resources.Remove(kBufferIndex);

        return true;
    }

    bool ResourceManager<Vulkan::Resource::Image>::DestroyResource(Vulkan::Device::DeviceState const & kDeviceState, uint32 const kResourceHandle)
    {
        Logging::Log(Logging::LogTypes::Info, String::Format(PBR_TEXT("DestroyImage: Destroying image [ID: %u]"), kResourceHandle));

        uint32 const kImageIndex = kResourceHandle - 1u;

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

static bool const HasRequiredExtensions(VkPhysicalDevice Device)
{
    uint32 ExtensionCount = {};
    VERIFY_VKRESULT(vkEnumerateDeviceExtensionProperties(Device, nullptr, &ExtensionCount, nullptr));

    std::vector AvailableExtensions = std::vector<VkExtensionProperties>(ExtensionCount);

    VERIFY_VKRESULT(vkEnumerateDeviceExtensionProperties(Device, nullptr, &ExtensionCount, AvailableExtensions.data()));

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

static bool const GetPhysicalDeviceList(VkInstance Instance, std::vector<VkPhysicalDevice> & OutputPhysicalDevices)
{
    bool bResult = false;

    uint32 DeviceCount = {};
    VERIFY_VKRESULT(vkEnumeratePhysicalDevices(Instance, &DeviceCount, nullptr));

    if (DeviceCount > 0u)
    {
        OutputPhysicalDevices.resize(DeviceCount);

        VERIFY_VKRESULT(vkEnumeratePhysicalDevices(Instance, &DeviceCount, OutputPhysicalDevices.data()));

        bResult = true;
    }
    else
    {
        Logging::FatalError(PBR_TEXT("Failed to find a device with Vulkan support."));
    }

    return bResult;
}

static bool const GetPhysicalDevice(VkInstance Instance, std::function<bool(VkPhysicalDeviceFeatures const &, VkPhysicalDeviceProperties const &)> PhysicalDeviceRequirementPredicate, VkPhysicalDevice & OutputPhysicalDevice)
{
    bool bResult = false;

    std::vector PhysicalDevices = std::vector<VkPhysicalDevice>();
    if (::GetPhysicalDeviceList(Instance, PhysicalDevices))
    {
        for (VkPhysicalDevice CurrentPhysicalDevice : PhysicalDevices)
        {
            VkPhysicalDeviceProperties DeviceProperties = {};
            vkGetPhysicalDeviceProperties(CurrentPhysicalDevice, &DeviceProperties);

            VkPhysicalDeviceFeatures DeviceFeatures = {};
            vkGetPhysicalDeviceFeatures(CurrentPhysicalDevice, &DeviceFeatures);

            if (PhysicalDeviceRequirementPredicate(DeviceFeatures, DeviceProperties))
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

static bool const GetQueueFamilyIndex(VkPhysicalDevice Device, VkQueueFlags RequiredFlags, bool const bSupportPresentation, VkSurfaceKHR Surface, uint32 & OutputQueueFamilyIndex)
{
    bool bResult = false;

    uint32 PropertyCount = {};
    vkGetPhysicalDeviceQueueFamilyProperties(Device, &PropertyCount, nullptr);

    std::vector Properties = std::vector<VkQueueFamilyProperties>(PropertyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(Device, &PropertyCount, Properties.data());

    {
        uint32 CurrentFamilyIndex = {};
        for (VkQueueFamilyProperties const & FamilyProperties : Properties)
        {
            if (FamilyProperties.queueFlags & RequiredFlags)
            {
                bool bSelectQueueFamily = true;

                if (bSupportPresentation)
                {
                    VkBool32 SupportsPresentation = {};
                    VERIFY_VKRESULT(vkGetPhysicalDeviceSurfaceSupportKHR(Device, CurrentFamilyIndex, Surface, &SupportsPresentation));

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

            VkDeviceQueueCreateInfo QueueCreateInfo = {};
            QueueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            QueueCreateInfo.queueFamilyIndex = IntermediateState.GraphicsQueueFamilyIndex;
            QueueCreateInfo.queueCount = 1u;
            QueueCreateInfo.pQueuePriorities = &QueuePriority;

            {
                VkDeviceCreateInfo CreateInfo = {};
                CreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
                CreateInfo.queueCreateInfoCount = 1u;
                CreateInfo.pQueueCreateInfos = &QueueCreateInfo;
                CreateInfo.enabledExtensionCount = static_cast<uint32>(kRequiredExtensionNames.size());
                CreateInfo.ppEnabledExtensionNames = kRequiredExtensionNames.data();
                CreateInfo.pEnabledFeatures = &IntermediateState.PhysicalDeviceFeatures;

                VERIFY_VKRESULT(vkCreateDevice(IntermediateState.PhysicalDevice, &CreateInfo, nullptr, &IntermediateState.Device));
            }

            if (::LoadDeviceFunctions(IntermediateState.Device) &&
                ::LoadDeviceExtensionFunctions(IntermediateState.Device, static_cast<uint32>(kRequiredExtensionNames.size()), kRequiredExtensionNames.data()))
            {
                vkGetDeviceQueue(IntermediateState.Device, IntermediateState.GraphicsQueueFamilyIndex, 0u, &IntermediateState.GraphicsQueue);

                {
                    VkCommandPoolCreateInfo CreateInfo = {};
                    CreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
                    CreateInfo.queueFamilyIndex = IntermediateState.GraphicsQueueFamilyIndex;
                    CreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

                    VERIFY_VKRESULT(vkCreateCommandPool(IntermediateState.Device, &CreateInfo, nullptr, &IntermediateState.CommandPool));
                }

                OutputState = IntermediateState;
                bResult = true;
            }
        }
    }

    return bResult;
}

void Vulkan::Device::DestroyDevice(Vulkan::Device::DeviceState & State)
{
    VERIFY_VKRESULT(vkDeviceWaitIdle(State.Device));

    if (State.CommandPool)
    {
        vkDestroyCommandPool(State.Device, State.CommandPool, nullptr);
        State.CommandPool = VK_NULL_HANDLE;
    }

    if (State.Device)
    {
        vkDestroyDevice(State.Device, nullptr);
        State.Device = VK_NULL_HANDLE;
    }
}

void Vulkan::Device::CreateCommandBuffers(Vulkan::Device::DeviceState const & State, VkCommandBufferLevel CommandBufferLevel, uint32 const CommandBufferCount, std::vector<VkCommandBuffer> & OutputCommandBuffers)
{
    std::vector CommandBuffers = std::vector<VkCommandBuffer>(CommandBufferCount);

    VkCommandBufferAllocateInfo AllocateInfo = {};
    AllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    AllocateInfo.commandPool = State.CommandPool;
    AllocateInfo.commandBufferCount = CommandBufferCount;
    AllocateInfo.level = CommandBufferLevel;

    VERIFY_VKRESULT(vkAllocateCommandBuffers(State.Device, &AllocateInfo, CommandBuffers.data()));

    OutputCommandBuffers = std::move(CommandBuffers);
}

void Vulkan::Device::DestroyCommandBuffers(Vulkan::Device::DeviceState const & State, std::vector<VkCommandBuffer> & CommandBuffers)
{
    vkFreeCommandBuffers(State.Device, State.CommandPool, static_cast<uint32>(CommandBuffers.size()), CommandBuffers.data());
}

void Vulkan::Device::CreateSemaphore(Vulkan::Device::DeviceState const & State, VkSemaphoreCreateFlags Flags, VkSemaphore & OutputSemaphore)
{
    VkSemaphoreCreateInfo CreateInfo = {};
    CreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    CreateInfo.flags = Flags;

    VERIFY_VKRESULT(vkCreateSemaphore(State.Device, &CreateInfo, nullptr, &OutputSemaphore));
}

void Vulkan::Device::DestroySemaphore(Vulkan::Device::DeviceState const & State, VkSemaphore & Semaphore)
{
    if (Semaphore != VK_NULL_HANDLE)
    {
        vkDestroySemaphore(State.Device, Semaphore, nullptr);
        Semaphore = VK_NULL_HANDLE;
    }
}

void Vulkan::Device::CreateFence(Vulkan::Device::DeviceState const & State, VkFenceCreateFlags Flags, VkFence & OutputFence)
{
    VkFenceCreateInfo CreateInfo = {};
    CreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    CreateInfo.flags = Flags;

    VERIFY_VKRESULT(vkCreateFence(State.Device, &CreateInfo, nullptr, &OutputFence));
}

void Vulkan::Device::DestroyFence(Vulkan::Device::DeviceState const & State, VkFence & Fence)
{
    if (Fence != VK_NULL_HANDLE)
    {
        vkDestroyFence(State.Device, Fence, nullptr);
        Fence = VK_NULL_HANDLE;
    }
}

void Vulkan::Device::CreateEvent(Vulkan::Device::DeviceState const & State, VkEventCreateFlags Flags, VkEvent & OutputEvent)
{
    VkEventCreateInfo CreateInfo = {};
    CreateInfo.sType = VK_STRUCTURE_TYPE_EVENT_CREATE_INFO;
    CreateInfo.flags = Flags;

    VERIFY_VKRESULT(vkCreateEvent(State.Device, &CreateInfo, nullptr, &OutputEvent));
}

void Vulkan::Device::DestroyEvent(Vulkan::Device::DeviceState const & State, VkEvent & Event)
{
    if (Event != VK_NULL_HANDLE)
    {
        vkDestroyEvent(State.Device, Event, nullptr);
        Event = VK_NULL_HANDLE;
    }
}

void Vulkan::Device::CreateFrameBuffer(Vulkan::Device::DeviceState const & State, uint32 const Width, uint32 const Height, VkRenderPass const RenderPass, std::vector<VkImageView> const & Attachments, uint32 & OutputFrameBufferHandle)
{
    VkExtent3D const FrameBufferExtents = { Width, Height, 1u };

    VkFramebufferCreateInfo const CreateInfo = Vulkan::FrameBuffer(FrameBufferExtents, RenderPass, static_cast<uint32>(Attachments.size()), Attachments.data());

    VkFramebuffer NewFrameBuffer = {};
    VERIFY_VKRESULT(vkCreateFramebuffer(State.Device, &CreateInfo, nullptr, &NewFrameBuffer));

    uint32 const kNewFrameBufferHandle = { FrameBufferManager.Resources.Add(NewFrameBuffer) + 1u };

    OutputFrameBufferHandle = kNewFrameBufferHandle;
}

void Vulkan::Device::DestroyFrameBuffer(Vulkan::Device::DeviceState const & State, uint32 const FrameBufferHandle, VkFence const FenceToWaitFor)
{
    if (FrameBufferHandle == 0u)
    {
        Logging::Log(Logging::LogTypes::Error, PBR_TEXT("DestroyFrameBuffer: Invalid frame buffer handle. Cannot destroy NULL frame buffer"));
        return;
    }

    if (FenceToWaitFor == VK_NULL_HANDLE)
    {
        FrameBufferManager.DestroyResource(State, FrameBufferHandle);
    }
    else
    {
        Logging::Log(Logging::LogTypes::Info, String::Format(PBR_TEXT("DestroyFrameBuffer: Queueing frame buffer for deletion [ID: %u]"), FrameBufferHandle));

        std::vector<uint32> & DeletionQueue = FrameBufferManager.FenceToPendingDeletionQueueMap [FenceToWaitFor];
        DeletionQueue.push_back(FrameBufferHandle);
    }
}

void Vulkan::Device::CreateBuffer(Vulkan::Device::DeviceState const & State, uint64 const SizeInBytes, VkBufferUsageFlags UsageFlags, VkMemoryPropertyFlags MemoryFlags, uint32 & OutputBufferHandle)
{
    Vulkan::Resource::Buffer NewBuffer = { SizeInBytes, 0u, VK_NULL_HANDLE };

    VkBufferCreateInfo const CreateInfo = Vulkan::Buffer(SizeInBytes, UsageFlags);

    VERIFY_VKRESULT(vkCreateBuffer(State.Device, &CreateInfo, nullptr, &NewBuffer.Resource));

    VkMemoryRequirements MemoryRequirements = {};
    vkGetBufferMemoryRequirements(State.Device, NewBuffer.Resource, &MemoryRequirements);

    Vulkan::Memory::Allocate(State, MemoryRequirements, MemoryFlags, NewBuffer.MemoryAllocationHandle);

    Vulkan::Memory::AllocationInfo AllocationInfo = {};
    Vulkan::Memory::GetAllocationInfo(NewBuffer.MemoryAllocationHandle, AllocationInfo);

    VERIFY_VKRESULT(vkBindBufferMemory(State.Device, NewBuffer.Resource, AllocationInfo.DeviceMemory, AllocationInfo.OffsetInBytes));

    uint32 const kNewBufferHandle = { BufferManager.Resources.Add(NewBuffer) + 1u };

    OutputBufferHandle = kNewBufferHandle;
}

void Vulkan::Device::DestroyBuffer(Vulkan::Device::DeviceState const & State, uint32 const BufferHandle, VkFence const FenceToWaitFor)
{
    if (BufferHandle == 0u)
    {
        Logging::Log(Logging::LogTypes::Error, PBR_TEXT("DestroyBuffer: Invalid buffer handle. Cannot destroy NULL buffer."));
        return;
    }

    if (FenceToWaitFor == VK_NULL_HANDLE)
    {
        BufferManager.DestroyResource(State, BufferHandle);
    }
    else
    {
        Logging::Log(Logging::LogTypes::Info, String::Format(PBR_TEXT("DestroyBuffer: Queueing buffer for deletion [ID: %u]"), BufferHandle));

        std::vector<uint32> & DeletionQueue = BufferManager.FenceToPendingDeletionQueueMap [FenceToWaitFor];
        DeletionQueue.push_back(BufferHandle);
    }
}

void Vulkan::Device::CreateImage(Vulkan::Device::DeviceState const & State, Vulkan::ImageDescriptor const & Descriptor, VkMemoryPropertyFlags const MemoryFlags, uint32 & OutputImageHandle)
{
    Vulkan::Resource::Image NewImage = { 0u, nullptr, Descriptor.Width, Descriptor.Height };

    VkExtent3D const ImageExtents = VkExtent3D { Descriptor.Width, Descriptor.Height, Descriptor.Depth };

    VkImageCreateInfo const CreateInfo = Vulkan::Image(Descriptor.ImageType, Descriptor.Format,
                                                       Descriptor.UsageFlags, ImageExtents,
                                                       Descriptor.bIsCPUWritable ? VK_IMAGE_LAYOUT_PREINITIALIZED : VK_IMAGE_LAYOUT_UNDEFINED,
                                                       Descriptor.MipLevelCount, Descriptor.ArrayLayerCount, Descriptor.SampleCount,
                                                       Descriptor.Tiling, VK_SHARING_MODE_EXCLUSIVE, 0u, nullptr, Descriptor.Flags);

    VERIFY_VKRESULT(vkCreateImage(State.Device, &CreateInfo, nullptr, &NewImage.Resource));

    VkMemoryRequirements MemoryRequirements = {};
    vkGetImageMemoryRequirements(State.Device, NewImage.Resource, &MemoryRequirements);

    Vulkan::Memory::Allocate(State, MemoryRequirements, MemoryFlags, NewImage.MemoryAllocationHandle);

    Vulkan::Memory::AllocationInfo AllocationInfo = {};
    Vulkan::Memory::GetAllocationInfo(NewImage.MemoryAllocationHandle, AllocationInfo);

    VERIFY_VKRESULT(vkBindImageMemory(State.Device, NewImage.Resource, AllocationInfo.DeviceMemory, AllocationInfo.OffsetInBytes));

    uint32 const kNewImageHandle = { ImageManager.Resources.Add(NewImage) + 1u };

    OutputImageHandle = kNewImageHandle;
}

void Vulkan::Device::DestroyImage(Vulkan::Device::DeviceState const & State, uint32 const ImageHandle, VkFence const FenceToWaitFor)
{
    if (ImageHandle == 0u)
    {
        Logging::Log(Logging::LogTypes::Error, PBR_TEXT("DestroyImage: Invalid image handle. Cannot destroy NULL image."));
        return;
    }

    if (FenceToWaitFor == VK_NULL_HANDLE)
    {
        ImageManager.DestroyResource(State, ImageHandle);
    }
    else
    {
        Logging::Log(Logging::LogTypes::Info, String::Format(PBR_TEXT("DestroyImage: Queueing image for deletion [ID: %u]"), ImageHandle));

        std::vector<uint32> & DeletionQueue = ImageManager.FenceToPendingDeletionQueueMap [FenceToWaitFor];
        DeletionQueue.push_back(ImageHandle);
    }
}

void Vulkan::Device::CreateImageView(Vulkan::Device::DeviceState const & State, uint32 const ImageHandle, Vulkan::ImageViewDescriptor const & Descriptor, uint32 & OutputImageViewHandle)
{
    Vulkan::Resource::Image Image = {};
    Vulkan::Resource::GetImage(ImageHandle, Image);

    VkComponentMapping const ComponentMapping = Descriptor.bReverseComponents 
        ? Vulkan::ComponentMapping(VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_R) 
        : Vulkan::ComponentMapping();

    VkImageSubresourceRange const SubResourceRange = Vulkan::ImageSubResourceRange(Descriptor.AspectFlags, Descriptor.MipLevelCount, Descriptor.ArrayLayerCount);

    VkImageViewCreateInfo const CreateInfo = Vulkan::ImageView(Image.Resource, Descriptor.ViewType, Descriptor.Format, ComponentMapping, SubResourceRange);

    VkImageView NewImageView = {};  
    VERIFY_VKRESULT(vkCreateImageView(State.Device, &CreateInfo, nullptr, &NewImageView));

    uint32 const kNewImageViewHandle = { ImageViewManager.Resources.Add(NewImageView) + 1u };

    OutputImageViewHandle = kNewImageViewHandle;
}

void Vulkan::Device::DestroyImageView(Vulkan::Device::DeviceState const & State, uint32 const ImageViewHandle, VkFence const FenceToWaitFor)
{
    if (ImageViewHandle == 0u)
    {
        Logging::Log(Logging::LogTypes::Info, PBR_TEXT("DestroyImageView: Invalid image view handle. Cannot destroy NULL image view."));
        return;
    }

    if (FenceToWaitFor == VK_NULL_HANDLE)
    {
        ImageViewManager.DestroyResource(State, ImageViewHandle);
    }
    else
    {
        Logging::Log(Logging::LogTypes::Info, String::Format(PBR_TEXT("DestroyImageView: Queueing image view for deletion [ID: %u]"), ImageViewHandle));

        ImageViewManager.FenceToPendingDeletionQueueMap [FenceToWaitFor].push_back(ImageViewHandle);
    }
}

void Vulkan::Device::DestroyUnusedResources(Vulkan::Device::DeviceState const & State)
{
    for (auto & DeletionEntry : BufferManager.FenceToPendingDeletionQueueMap)
    {
        VkResult const FenceStatus = vkGetFenceStatus(State.Device, DeletionEntry.first);

        if (FenceStatus == VK_SUCCESS)
        {
            for (uint32 const BufferHandle : DeletionEntry.second)
            {
                BufferManager.DestroyResource(State, BufferHandle);
            }

            DeletionEntry.second.clear();
        }
    }

    for (auto & DeletionEntry : ImageManager.FenceToPendingDeletionQueueMap)
    {
        VkResult const FenceStatus = vkGetFenceStatus(State.Device, DeletionEntry.first);

        if (FenceStatus == VK_SUCCESS)
        {
            for (uint32 const ImageHandle : DeletionEntry.second)
            {
                ImageManager.DestroyResource(State, ImageHandle);
            }

            DeletionEntry.second.clear();
        }
    }

    for (auto & DeletionEntry : ImageViewManager.FenceToPendingDeletionQueueMap)
    {
        VkResult const FenceStatus = vkGetFenceStatus(State.Device, DeletionEntry.first);

        if (FenceStatus == VK_SUCCESS)
        {
            for (uint32 const ImageViewHandle : DeletionEntry.second)
            {
                ImageViewManager.DestroyResource(State, ImageViewHandle);
            }
        }
    }

    for (std::pair<const VkFence, std::vector<uint32>> & DeletionEntry : FrameBufferManager.FenceToPendingDeletionQueueMap)
    {
        VkResult const FenceStatus = vkGetFenceStatus(State.Device, DeletionEntry.first);

        if (FenceStatus == VK_SUCCESS)
        {
            for (uint32 const FrameBufferHandle : DeletionEntry.second)
            {
                FrameBufferManager.DestroyResource(State, FrameBufferHandle);
            }
        }
    }
}

bool const Vulkan::Resource::GetBuffer(uint32 const BufferHandle, Vulkan::Resource::Buffer & OutputBuffer)
{
    if (BufferHandle == 0u)
    {
        Logging::Log(Logging::LogTypes::Error, PBR_TEXT("GetBuffer: Invalid buffer handle. Cannot get buffer for NULL handle."));
        return false;
    }

    //PBR_ASSERT(BufferHandle <= BufferManager.Resources.size());

    uint32 const kBufferIndex = BufferHandle - 1u;
    OutputBuffer = BufferManager.Resources [kBufferIndex];

    return true;
}

bool const Vulkan::Resource::GetImage(uint32 const ImageHandle, Vulkan::Resource::Image & OutputImage)
{
    if (ImageHandle == 0u)
    {
        Logging::Log(Logging::LogTypes::Error, PBR_TEXT("GetImage: Invalid image handle. Cannot get image for NULL handle."));
        return false;
    }

    //PBR_ASSERT(ImageHandle <= ImageManager.Resources.size());

    uint32 const kImageIndex = ImageHandle - 1u;

    OutputImage = ImageManager.Resources [kImageIndex];

    return true;
}

bool const Vulkan::Resource::GetImageView(uint32 const ImageViewHandle, VkImageView & OutputImageView)
{
    if (ImageViewHandle == 0u)
    {
        Logging::Log(Logging::LogTypes::Error, PBR_TEXT("GetImageView: Invalid image view handle. Cannot get image view for NULL handle."));
        return false;
    }

    //PBR_ASSERT(ImageViewHandle <= ImageViewManager.Resources.size());

    uint32 const kImageViewIndex = ImageViewHandle - 1u;

    OutputImageView = ImageViewManager.Resources [kImageViewIndex];

    return true;
}

bool const Vulkan::Resource::GetFrameBuffer(uint32 const FrameBufferHandle, VkFramebuffer & OutputFrameBuffer)
{
    if (FrameBufferHandle == 0u)
    {
        Logging::Log(Logging::LogTypes::Error, PBR_TEXT("GetFrameBuffer: Invalid frame buffer handle. Cannot get frame buffer for NULL handle."));
        return false;
    }

    //PBR_ASSERT(FrameBufferHandle <= FrameBufferManager.Resources.size());

    uint32 const kFrameBufferIndex = { FrameBufferHandle - 1u };

    OutputFrameBuffer = FrameBufferManager.Resources [kFrameBufferIndex];

    return true;
}