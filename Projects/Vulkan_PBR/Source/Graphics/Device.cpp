#include "Graphics/Device.hpp"

#include "Graphics/Instance.hpp"

#include "Memory.hpp"

#include <vector>
#include <string>
#include <functional>

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

void Vulkan::Device::CreateFrameBuffer(Vulkan::Device::DeviceState const & State, uint32 const Width, uint32 const Height, VkRenderPass const RenderPass, std::vector<VkImageView> const & Attachments, GPUResourceManager::FrameBufferHandle & OutputFrameBufferHandle)
{
    VkFramebufferCreateInfo CreateInfo = {};
    CreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    CreateInfo.renderPass = RenderPass;
    CreateInfo.width = Width;
    CreateInfo.height = Height;
    CreateInfo.layers = 1u;
    CreateInfo.attachmentCount = static_cast<uint32>(Attachments.size());
    CreateInfo.pAttachments = Attachments.data();

    GPUResource::FrameBuffer NewFrameBuffer = {};
    VERIFY_VKRESULT(vkCreateFramebuffer(State.Device, &CreateInfo, nullptr, &NewFrameBuffer.VkResource));

    GPUResourceManager::FrameBufferHandle NewFrameBufferHandle = {};
    GPUResourceManager::RegisterResource(NewFrameBuffer, NewFrameBufferHandle);

    OutputFrameBufferHandle = NewFrameBufferHandle;
}

void Vulkan::Device::DestroyFrameBuffer(Vulkan::Device::DeviceState const & State, GPUResourceManager::FrameBufferHandle const FrameBufferHandle, VkFence const FenceToWaitFor)
{
    if (FenceToWaitFor)
    {
        GPUResourceManager::QueueResourceDeletion(FenceToWaitFor, FrameBufferHandle);
    }
    else
    {
        GPUResourceManager::DestroyResource(FrameBufferHandle, State);
    }
}

void Vulkan::Device::CreateBuffer(Vulkan::Device::DeviceState const & State, uint64 SizeInBytes, VkBufferUsageFlags UsageFlags, VkMemoryPropertyFlags MemoryFlags, GPUResourceManager::BufferHandle & OutputBufferHandle)
{
    VkBufferCreateInfo CreateInfo = {};
    CreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    CreateInfo.size = SizeInBytes;
    CreateInfo.usage = UsageFlags;
    CreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    GPUResource::Buffer NewBuffer = {};
    VERIFY_VKRESULT(vkCreateBuffer(State.Device, &CreateInfo, nullptr, &NewBuffer.VkResource));

    VkMemoryRequirements MemoryRequirements = {};
    vkGetBufferMemoryRequirements(State.Device, NewBuffer.VkResource, &MemoryRequirements);

    DeviceMemoryAllocator::AllocateMemory(State, MemoryRequirements, MemoryFlags, NewBuffer.MemoryAllocation);

    VERIFY_VKRESULT(vkBindBufferMemory(State.Device, NewBuffer.VkResource, NewBuffer.MemoryAllocation.Memory, NewBuffer.MemoryAllocation.OffsetInBytes));

    GPUResourceManager::BufferHandle NewBufferHandle = {};
    GPUResourceManager::RegisterResource(NewBuffer, NewBufferHandle);

    OutputBufferHandle = NewBufferHandle;
}

void Vulkan::Device::DestroyBuffer(Vulkan::Device::DeviceState const & State, GPUResourceManager::BufferHandle const BufferHandle, VkFence const FenceToWaitFor)
{
    if (FenceToWaitFor)
    {
        GPUResourceManager::QueueResourceDeletion(FenceToWaitFor, BufferHandle);
    }
    else
    {
        GPUResourceManager::DestroyResource(BufferHandle, State);
    }
}

void Vulkan::Device::CreateImage(Vulkan::Device::DeviceState const & State, Vulkan::ImageDescriptor const & Descriptor, VkMemoryPropertyFlags const MemoryFlags, GPUResourceManager::ImageHandle & OutputImageHandle)
{
    VkImageCreateInfo CreateInfo = {};
    CreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    CreateInfo.imageType = Descriptor.ImageType;
    CreateInfo.format = Descriptor.Format;
    CreateInfo.usage = Descriptor.UsageFlags;
    CreateInfo.extent.width = Descriptor.Width;
    CreateInfo.extent.height = Descriptor.Height;
    CreateInfo.extent.depth = Descriptor.Depth;
    CreateInfo.arrayLayers = Descriptor.ArrayLayerCount;
    CreateInfo.mipLevels = Descriptor.MipLevelCount;
    CreateInfo.samples = static_cast<VkSampleCountFlagBits>(Descriptor.SampleCount);
    CreateInfo.initialLayout = Descriptor.bIsCPUWritable ? VK_IMAGE_LAYOUT_PREINITIALIZED : VK_IMAGE_LAYOUT_UNDEFINED;
    CreateInfo.tiling = Descriptor.Tiling;
    CreateInfo.flags = Descriptor.Flags;
    CreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    GPUResource::Image NewImage = {};
    VERIFY_VKRESULT(vkCreateImage(State.Device, &CreateInfo, nullptr, &NewImage.VkResource));

    VkMemoryRequirements MemoryRequirements = {};
    vkGetImageMemoryRequirements(State.Device, NewImage.VkResource, &MemoryRequirements);

    DeviceMemoryAllocator::AllocateMemory(State, MemoryRequirements, MemoryFlags, NewImage.MemoryAllocation);

    VERIFY_VKRESULT(vkBindImageMemory(State.Device, NewImage.VkResource, NewImage.MemoryAllocation.Memory, NewImage.MemoryAllocation.OffsetInBytes));

    GPUResourceManager::ImageHandle NewImageHandle = {};
    GPUResourceManager::RegisterResource(NewImage, NewImageHandle);

    OutputImageHandle = NewImageHandle;
}

void Vulkan::Device::DestroyImage(Vulkan::Device::DeviceState const & State, GPUResourceManager::ImageHandle ImageHandle, VkFence const FenceToWaitFor)
{
    if (FenceToWaitFor)
    {
        GPUResourceManager::QueueResourceDeletion(FenceToWaitFor, ImageHandle);
    }
    else
    {
        GPUResourceManager::DestroyResource(ImageHandle, State);
    }
}

void Vulkan::Device::CreateImageView(Vulkan::Device::DeviceState const & State, GPUResourceManager::ImageHandle ImageHandle, Vulkan::ImageViewDescriptor const & Descriptor, GPUResourceManager::ImageViewHandle & OutputImageViewHandle)
{
    GPUResource::Image Image = {};
    GPUResourceManager::GetResource(ImageHandle, Image);

    VkImageViewCreateInfo CreateInfo = {};
    CreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    CreateInfo.image = Image.VkResource;
    CreateInfo.format = Descriptor.Format;
    CreateInfo.viewType = Descriptor.ViewType;
    CreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    CreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    CreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    CreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    CreateInfo.subresourceRange.aspectMask = Descriptor.AspectFlags;
    CreateInfo.subresourceRange.baseArrayLayer = Descriptor.FirstArrayLayerIndex;
    CreateInfo.subresourceRange.layerCount = Descriptor.ArrayLayerCount;
    CreateInfo.subresourceRange.baseMipLevel = Descriptor.FirstMipLevelIndex;
    CreateInfo.subresourceRange.levelCount = Descriptor.MipLevelCount;

    GPUResource::ImageView NewImageView = {};
    VERIFY_VKRESULT(vkCreateImageView(State.Device, &CreateInfo, nullptr, &NewImageView.VkResource));

    GPUResourceManager::ImageViewHandle ImageViewHandle = {};
    GPUResourceManager::RegisterResource(NewImageView, ImageViewHandle);

    OutputImageViewHandle = ImageViewHandle;
}