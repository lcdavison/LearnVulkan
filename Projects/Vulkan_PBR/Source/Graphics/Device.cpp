#include "Graphics/Device.hpp"

#include "Graphics/Instance.hpp"

#include "Memory.hpp"

#include <vector>
#include <string>
#include <functional>

struct BufferCollection
{
    std::unordered_map<VkFence, std::vector<uint32>> DeletionQueue = {};
    std::queue<uint32> AvailableResourceHandles = {};

    std::vector<VkBuffer> Resources = {};
    std::vector<DeviceMemoryAllocator::Allocation> MemoryAllocations = {};  // TODO: Allocations should be packed better, to be more cache friendly
    std::vector<VkDeviceSize> SizesInBytes = {};
};

struct ImageCollection
{
    struct Extents
    {
        uint32 Width;
        uint32 Height;
    };

    std::unordered_map<VkFence, std::vector<uint32>> DeletionQueue = {};
    std::queue<uint32> AvailableResourceHandles = {};

    std::vector<VkImage> Resources = {};
    std::vector<DeviceMemoryAllocator::Allocation> MemoryAllocations = {};
    std::vector<Extents> Extents = {};
};

struct ImageViewCollection
{
    std::unordered_map<VkFence, std::vector<uint32>> DeletionQueue = {};
    std::queue<uint32> AvailableResourceHandles = {};

    std::vector<VkImageView> Resources = {};
};

struct FrameBufferCollection
{
    std::unordered_map<VkFence, std::vector<uint32>> DeletionQueue = {};
    std::queue<uint32> AvailableResourceHandles = {};

    std::vector<VkFramebuffer> Resources = {};
};

static BufferCollection Buffers = {};
static ImageCollection Images = {};
static ImageViewCollection ImageViews = {};
static FrameBufferCollection FrameBuffers = {};

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

static bool const GetPhysicalDevice(VkInstance Instance, std::function<bool(VkPhysicalDeviceProperties const &)> PhysicalDeviceRequirementPredicate, VkPhysicalDevice & OutputPhysicalDevice)
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

            if (PhysicalDeviceRequirementPredicate(DeviceProperties))
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

static void CreateDescriptorPool(Vulkan::Device::DeviceState & State)
{
    std::vector PoolSizes = std::vector<VkDescriptorPoolSize>
    {
        VkDescriptorPoolSize { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 4u },
    };

    VkDescriptorPoolCreateInfo CreateInfo = {};
    CreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    CreateInfo.poolSizeCount = static_cast<uint32>(PoolSizes.size());
    CreateInfo.pPoolSizes = PoolSizes.data();
    CreateInfo.maxSets = 8u;

    VERIFY_VKRESULT(vkCreateDescriptorPool(State.Device, &CreateInfo, nullptr, &State.DescriptorPool));
}

bool const Vulkan::Device::CreateDevice(Vulkan::Instance::InstanceState const & InstanceState, Vulkan::Device::DeviceState & OutputState)
{
    /* Use this for atomicity, we don't want to partially fill OutputState */
    DeviceState IntermediateState = {};

    bool bResult = ::GetPhysicalDevice(InstanceState.Instance,
                                       [](VkPhysicalDeviceProperties const & Properties)
                                       {
                                           return Properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;
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
                /* Need to figure out what features need to be enabled */
                //CreateInfo.pEnabledFeatures;

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

                ::CreateDescriptorPool(IntermediateState);

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

    if (State.DescriptorPool)
    {
        vkDestroyDescriptorPool(State.Device, State.DescriptorPool, nullptr);
        State.DescriptorPool = VK_NULL_HANDLE;
    }

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

    uint32 NewFrameBufferHandle = {};
    if (FrameBuffers.AvailableResourceHandles.size() == 0u)
    {
        FrameBuffers.Resources.push_back(NewFrameBuffer);

        NewFrameBufferHandle = static_cast<uint32>(FrameBuffers.Resources.size());
    }
    else
    {
        NewFrameBufferHandle = FrameBuffers.AvailableResourceHandles.front();
        FrameBuffers.AvailableResourceHandles.pop();

        uint32 const NewFrameBufferIndex = { NewFrameBufferHandle - 1u };

        FrameBuffers.Resources [NewFrameBufferIndex] = NewFrameBuffer;
    }

    OutputFrameBufferHandle = NewFrameBufferHandle;
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
        Logging::Log(Logging::LogTypes::Info, String::Format(PBR_TEXT("DestroyFrameBuffer: Destroying frame buffer [ID: %u]"), FrameBufferHandle));

        uint32 const FrameBufferIndex = FrameBufferHandle - 1u;

        VkFramebuffer& FrameBuffer = FrameBuffers.Resources [FrameBufferIndex];
        PBR_ASSERT(FrameBuffer);

        vkDestroyFramebuffer(State.Device, FrameBuffer, nullptr);
        FrameBuffer = VK_NULL_HANDLE;

        FrameBuffers.AvailableResourceHandles.push(FrameBufferHandle);
    }
    else
    {
        Logging::Log(Logging::LogTypes::Info, String::Format(PBR_TEXT("DestroyFrameBuffer: Queueing frame buffer for deletion [ID: %u]"), FrameBufferHandle));

        FrameBuffers.DeletionQueue [FenceToWaitFor].push_back(FrameBufferHandle);
    }
}

void Vulkan::Device::CreateBuffer(Vulkan::Device::DeviceState const & State, uint64 const SizeInBytes, VkBufferUsageFlags UsageFlags, VkMemoryPropertyFlags MemoryFlags, uint32 & OutputBufferHandle)
{
    VkBufferCreateInfo const CreateInfo = Vulkan::Buffer(SizeInBytes, UsageFlags);

    VkBuffer NewBuffer = {};
    VERIFY_VKRESULT(vkCreateBuffer(State.Device, &CreateInfo, nullptr, &NewBuffer));

    VkMemoryRequirements MemoryRequirements = {};
    vkGetBufferMemoryRequirements(State.Device, NewBuffer, &MemoryRequirements);

    DeviceMemoryAllocator::Allocation BufferAllocation = {};
    DeviceMemoryAllocator::AllocateMemory(State, MemoryRequirements, MemoryFlags, BufferAllocation);

    VERIFY_VKRESULT(vkBindBufferMemory(State.Device, NewBuffer, BufferAllocation.Memory, BufferAllocation.OffsetInBytes));

    uint32 NewBufferHandle = {};
    if (Buffers.AvailableResourceHandles.size() > 0u)
    {
        NewBufferHandle = Buffers.AvailableResourceHandles.front();
        Buffers.AvailableResourceHandles.pop();

        Logging::Log(Logging::LogTypes::Info, String::Format(PBR_TEXT("Reusing Buffer Handle [ID: %u]"), NewBufferHandle));

        uint32 const NewBufferIndex = NewBufferHandle - 1u;

        Buffers.Resources [NewBufferIndex] = NewBuffer;
        Buffers.MemoryAllocations [NewBufferIndex] = BufferAllocation;
        Buffers.SizesInBytes [NewBufferIndex] = SizeInBytes;
    }
    else
    {
        Buffers.Resources.push_back(NewBuffer);
        Buffers.MemoryAllocations.push_back(BufferAllocation);
        Buffers.SizesInBytes.push_back(SizeInBytes);

        NewBufferHandle = static_cast<uint32>(Buffers.Resources.size());
    }

    OutputBufferHandle = NewBufferHandle;
}

void Vulkan::Device::DestroyBuffer(Vulkan::Device::DeviceState const & State, uint32 const BufferHandle, VkFence const FenceToWaitFor)
{
    if (BufferHandle == 0u)
    {
        Logging::Log(Logging::LogTypes::Error, PBR_TEXT("DestroyBuffer: Invalid buffer handle. Cannot destroy NULL buffer."));
        return;
    }

    if (FenceToWaitFor)
    {
        Logging::Log(Logging::LogTypes::Info, String::Format(PBR_TEXT("DestroyBuffer: Queueing buffer for deletion [ID: %u]"), BufferHandle));

        Buffers.DeletionQueue [FenceToWaitFor].push_back(BufferHandle);
    }
    else
    {
        Logging::Log(Logging::LogTypes::Info, String::Format(PBR_TEXT("DestroyBuffer: Destroying buffer [ID: %u]"), BufferHandle));

        VkBuffer & Buffer = Buffers.Resources [BufferHandle - 1u];

        if (Buffer != VK_NULL_HANDLE)
        {
            vkDestroyBuffer(State.Device, Buffer, nullptr);
            Buffer = VK_NULL_HANDLE;
        }

        Buffers.AvailableResourceHandles.push(BufferHandle);
    }
}

void Vulkan::Device::CreateImage(Vulkan::Device::DeviceState const & State, Vulkan::ImageDescriptor const & Descriptor, VkMemoryPropertyFlags const MemoryFlags, uint32 & OutputImageHandle)
{
    VkExtent3D const ImageExtents = VkExtent3D { Descriptor.Width, Descriptor.Height, Descriptor.Depth };

    VkImageCreateInfo const CreateInfo = Vulkan::Image(Descriptor.ImageType, Descriptor.Format,
                                                       Descriptor.UsageFlags, ImageExtents,
                                                       Descriptor.bIsCPUWritable ? VK_IMAGE_LAYOUT_PREINITIALIZED : VK_IMAGE_LAYOUT_UNDEFINED,
                                                       Descriptor.MipLevelCount, Descriptor.ArrayLayerCount, Descriptor.SampleCount,
                                                       Descriptor.Tiling, VK_SHARING_MODE_EXCLUSIVE, 0u, nullptr, Descriptor.Flags);

    VkImage NewImage = {};
    VERIFY_VKRESULT(vkCreateImage(State.Device, &CreateInfo, nullptr, &NewImage));

    VkMemoryRequirements MemoryRequirements = {};
    vkGetImageMemoryRequirements(State.Device, NewImage, &MemoryRequirements);

    DeviceMemoryAllocator::Allocation MemoryAllocation = {};
    DeviceMemoryAllocator::AllocateMemory(State, MemoryRequirements, MemoryFlags, MemoryAllocation);

    VERIFY_VKRESULT(vkBindImageMemory(State.Device, NewImage, MemoryAllocation.Memory, MemoryAllocation.OffsetInBytes));

    uint32 NewImageHandle = {};
    if (Images.AvailableResourceHandles.size() > 0u)
    {
        NewImageHandle = Images.AvailableResourceHandles.front();
        Images.AvailableResourceHandles.pop();

        uint32 const NewImageIndex = { NewImageHandle - 1u };
        Images.Resources [NewImageIndex] = NewImage;
        Images.MemoryAllocations [NewImageIndex] = MemoryAllocation;
        Images.Extents [NewImageIndex] = ImageCollection::Extents { Descriptor.Width, Descriptor.Height };
    }
    else
    {
        Images.Resources.push_back(NewImage);
        Images.MemoryAllocations.push_back(MemoryAllocation);
        Images.Extents.push_back(ImageCollection::Extents { Descriptor.Width, Descriptor.Height });

        NewImageHandle = static_cast<uint32>(Images.Resources.size());
    }

    OutputImageHandle = NewImageHandle;
}

void Vulkan::Device::DestroyImage(Vulkan::Device::DeviceState const & State, uint32 const ImageHandle, VkFence const FenceToWaitFor)
{
    if (ImageHandle == 0u)
    {
        Logging::Log(Logging::LogTypes::Error, PBR_TEXT("DestroyImage: Invalid image handle. Cannot destroy NULL image."));
        return;
    }

    if (FenceToWaitFor)
    {
        Logging::Log(Logging::LogTypes::Info, String::Format(PBR_TEXT("DestroyImage: Queueing image for deletion [ID: %u]"), ImageHandle));

        Images.DeletionQueue [FenceToWaitFor].push_back(ImageHandle);
    }
    else
    {
        Logging::Log(Logging::LogTypes::Info, String::Format(PBR_TEXT("DestroyImage: Destroying image [ID: %u]"), ImageHandle));

        VkImage & Image = Images.Resources [ImageHandle - 1u];

        if (Image != VK_NULL_HANDLE)
        {
            vkDestroyImage(State.Device, Image, nullptr);
            Image = VK_NULL_HANDLE;
        }

        Images.AvailableResourceHandles.push(ImageHandle);
    }
}

void Vulkan::Device::CreateImageView(Vulkan::Device::DeviceState const & State, uint32 const ImageHandle, Vulkan::ImageViewDescriptor const & Descriptor, uint32 & OutputImageViewHandle)
{
    Vulkan::Resource::Image Image = {};
    Vulkan::Resource::GetImage(ImageHandle, Image);

    VkComponentMapping const ComponentMapping = Vulkan::ComponentMapping();
    VkImageSubresourceRange const SubResourceRange = Vulkan::ImageSubResourceRange(Descriptor.AspectFlags, Descriptor.MipLevelCount, Descriptor.ArrayLayerCount);

    VkImageViewCreateInfo const CreateInfo = Vulkan::ImageView(Image.Resource, Descriptor.ViewType, Descriptor.Format, ComponentMapping, SubResourceRange);

    VkImageView NewImageView = {};  
    VERIFY_VKRESULT(vkCreateImageView(State.Device, &CreateInfo, nullptr, &NewImageView));

    uint32 NewImageViewHandle = {};
    if (ImageViews.AvailableResourceHandles.size() > 0u)
    {
        NewImageViewHandle = ImageViews.AvailableResourceHandles.front();
        ImageViews.AvailableResourceHandles.pop();

        PBR_ASSERT(NewImageViewHandle > 0u);

        uint32 const NewImageViewIndex = { NewImageViewHandle - 1u };
        ImageViews.Resources [NewImageViewIndex] = NewImageView;
    }
    else
    {
        ImageViews.Resources.push_back(NewImageView);

        NewImageViewHandle = static_cast<uint32>(ImageViews.Resources.size());
    }

    OutputImageViewHandle = NewImageViewHandle;
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
        Logging::Log(Logging::LogTypes::Info, String::Format(PBR_TEXT("DestroyImageView: Destroying image view [ID: %u]"), ImageViewHandle));

        uint32 const ImageViewIndex = { ImageViewHandle - 1u };
        VkImageView & ImageView = ImageViews.Resources [ImageViewIndex];

        if (ImageView != VK_NULL_HANDLE)
        {
            vkDestroyImageView(State.Device, ImageView, nullptr);
            ImageView = VK_NULL_HANDLE;
        }

        ImageViews.AvailableResourceHandles.push(ImageViewHandle);
    }
    else
    {
        Logging::Log(Logging::LogTypes::Info, String::Format(PBR_TEXT("DestroyImageView: Queueing image view for deletion [ID: %u]"), ImageViewHandle));

        ImageViews.DeletionQueue [FenceToWaitFor].push_back(ImageViewHandle);
    }
}

void Vulkan::Device::DestroyUnusedResources(Vulkan::Device::DeviceState const & State)
{
    for (auto & DeletionEntry : Buffers.DeletionQueue)
    {
        VkResult const FenceStatus = vkGetFenceStatus(State.Device, DeletionEntry.first);

        if (FenceStatus == VK_SUCCESS)
        {
            for (uint32 const BufferHandle : DeletionEntry.second)
            {
                Logging::Log(Logging::LogTypes::Info, String::Format(PBR_TEXT("Destroying Buffer [ID = %u]"), BufferHandle));

                uint32 const BufferIndex = BufferHandle - 1u;

                vkDestroyBuffer(State.Device, Buffers.Resources [BufferIndex], nullptr);
                Buffers.Resources [BufferIndex] = VK_NULL_HANDLE;

                DeviceMemoryAllocator::FreeMemory(Buffers.MemoryAllocations [BufferIndex]);
                
                Buffers.AvailableResourceHandles.push(BufferHandle);
            }

            DeletionEntry.second.clear();
        }
    }

    for (auto & DeletionEntry : Images.DeletionQueue)
    {
        VkResult const FenceStatus = vkGetFenceStatus(State.Device, DeletionEntry.first);

        if (FenceStatus == VK_SUCCESS)
        {
            for (uint32 const ImageHandle : DeletionEntry.second)
            {
                Logging::Log(Logging::LogTypes::Info, String::Format(PBR_TEXT("Destroying Image [ID = %u]"), ImageHandle));

                uint32 const ImageIndex = ImageHandle - 1u;

                vkDestroyImage(State.Device, Images.Resources [ImageIndex], nullptr);
                Images.Resources [ImageIndex] = VK_NULL_HANDLE;

                DeviceMemoryAllocator::FreeMemory(Images.MemoryAllocations [ImageIndex]);

                Images.AvailableResourceHandles.push(ImageHandle);
            }

            DeletionEntry.second.clear();
        }
    }

    for (auto & DeletionEntry : ImageViews.DeletionQueue)
    {
        VkResult const FenceStatus = vkGetFenceStatus(State.Device, DeletionEntry.first);

        if (FenceStatus == VK_SUCCESS)
        {
            for (uint32 const ImageViewHandle : DeletionEntry.second)
            {
                Logging::Log(Logging::LogTypes::Info, String::Format(PBR_TEXT("DestroyImageView: Destroying image view [ID: %u]"), ImageViewHandle));

                uint32 const ImageViewIndex = { ImageViewHandle - 1u };
                
                vkDestroyImageView(State.Device, ImageViews.Resources [ImageViewIndex], nullptr);
                ImageViews.Resources [ImageViewIndex] = VK_NULL_HANDLE;

                ImageViews.AvailableResourceHandles.push(ImageViewHandle);
            }
        }
    }

    for (std::pair<const VkFence, std::vector<uint32>> & DeletionEntry : FrameBuffers.DeletionQueue)
    {
        VkResult const FenceStatus = vkGetFenceStatus(State.Device, DeletionEntry.first);

        if (FenceStatus == VK_SUCCESS)
        {
            for (uint32 const FrameBufferHandle : DeletionEntry.second)
            {
                Logging::Log(Logging::LogTypes::Info, String::Format(PBR_TEXT("DestroyFrameBuffer: Destroying frame buffer [ID: %u]"), FrameBufferHandle));

                uint32 const FrameBufferIndex = { FrameBufferHandle - 1u };

                VkFramebuffer & FrameBuffer = FrameBuffers.Resources [FrameBufferIndex];
                PBR_ASSERT(FrameBuffer);

                vkDestroyFramebuffer(State.Device, FrameBuffer, nullptr);
                FrameBuffer = VK_NULL_HANDLE;

                FrameBuffers.AvailableResourceHandles.push(FrameBufferHandle);
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

    PBR_ASSERT(BufferHandle <= Buffers.Resources.size());

    uint32 const BufferIndex = BufferHandle - 1u;

    OutputBuffer.Resource = Buffers.Resources [BufferIndex];
    OutputBuffer.SizeInBytes = Buffers.SizesInBytes [BufferIndex];
    OutputBuffer.MemoryAllocation = &Buffers.MemoryAllocations [BufferIndex];

    return true;
}

bool const Vulkan::Resource::GetImage(uint32 const ImageHandle, Vulkan::Resource::Image & OutputImage)
{
    if (ImageHandle == 0u)
    {
        Logging::Log(Logging::LogTypes::Error, PBR_TEXT("GetImage: Invalid image handle. Cannot get image for NULL handle."));
        return false;
    }

    PBR_ASSERT(ImageHandle <= Images.Resources.size());

    uint32 const ImageIndex = ImageHandle - 1u;

    OutputImage.Resource = Images.Resources [ImageIndex];
    OutputImage.MemoryAllocation = &Images.MemoryAllocations [ImageIndex];
    OutputImage.Width = Images.Extents [ImageIndex].Width;
    OutputImage.Height = Images.Extents [ImageIndex].Height;

    return true;
}

bool const Vulkan::Resource::GetImageView(uint32 const ImageViewHandle, VkImageView & OutputImageView)
{
    if (ImageViewHandle == 0u)
    {
        Logging::Log(Logging::LogTypes::Error, PBR_TEXT("GetImageView: Invalid image view handle. Cannot get image view for NULL handle."));
        return false;
    }

    PBR_ASSERT(ImageViewHandle <= ImageViews.Resources.size());

    uint32 const ImageViewIndex = ImageViewHandle - 1u;
    OutputImageView = ImageViews.Resources [ImageViewIndex];

    return true;
}

bool const Vulkan::Resource::GetFrameBuffer(uint32 const FrameBufferHandle, VkFramebuffer & OutputFrameBuffer)
{
    if (FrameBufferHandle == 0u)
    {
        Logging::Log(Logging::LogTypes::Error, PBR_TEXT("GetFrameBuffer: Invalid frame buffer handle. Cannot get frame buffer for NULL handle."));
        return false;
    }

    PBR_ASSERT(FrameBufferHandle <= FrameBuffers.Resources.size());

    uint32 const FrameBufferIndex = { FrameBufferHandle - 1u };
    OutputFrameBuffer = FrameBuffers.Resources [FrameBufferIndex];

    return true;
}