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

                vkGetPhysicalDeviceMemoryProperties(IntermediateState.PhysicalDevice, &IntermediateState.MemoryProperties);

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

void Vulkan::Device::CreateCommandBuffers(DeviceState const & State, VkCommandBufferLevel CommandBufferLevel, uint32 const CommandBufferCount, std::vector<VkCommandBuffer> & OutputCommandBuffers)
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

void Vulkan::Device::CreateFrameBuffer(Vulkan::Device::DeviceState const & State, uint32 Width, uint32 Height, VkRenderPass RenderPass, std::vector<VkImageView> const & Attachments, VkFramebuffer & OutputFrameBuffer)
{
    VkFramebufferCreateInfo CreateInfo = {};
    CreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    CreateInfo.renderPass = RenderPass;
    CreateInfo.width = Width;
    CreateInfo.height = Height;
    CreateInfo.layers = 1u;
    CreateInfo.attachmentCount = static_cast<uint32>(Attachments.size());
    CreateInfo.pAttachments = Attachments.data();

    VERIFY_VKRESULT(vkCreateFramebuffer(State.Device, &CreateInfo, nullptr, &OutputFrameBuffer));
}

void Vulkan::Device::DestroyFrameBuffer(Vulkan::Device::DeviceState const & State, VkFramebuffer & FrameBuffer)
{
    if (FrameBuffer != VK_NULL_HANDLE)
    {
        vkDestroyFramebuffer(State.Device, FrameBuffer, nullptr);
        FrameBuffer = VK_NULL_HANDLE;
    }
}

void Vulkan::Device::CreateBuffer(Vulkan::Device::DeviceState const & State, uint64 SizeInBytes, VkBufferUsageFlags UsageFlags, VkMemoryPropertyFlags MemoryFlags, VkBuffer & OutputBuffer, DeviceMemoryAllocator::Allocation & OutputDeviceMemory)
{
    VkBufferCreateInfo CreateInfo = {};
    CreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    CreateInfo.size = SizeInBytes;
    CreateInfo.usage = UsageFlags;
    CreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VkBuffer NewBuffer = {};
    VERIFY_VKRESULT(vkCreateBuffer(State.Device, &CreateInfo, nullptr, &NewBuffer));

    VkMemoryRequirements MemoryRequirements = {};
    vkGetBufferMemoryRequirements(State.Device, NewBuffer, &MemoryRequirements);

    DeviceMemoryAllocator::Allocation MemoryAllocation = {};
    DeviceMemoryAllocator::AllocateMemory(State, MemoryRequirements, MemoryFlags, MemoryAllocation);

    VERIFY_VKRESULT(vkBindBufferMemory(State.Device, NewBuffer, MemoryAllocation.Memory, MemoryAllocation.OffsetInBytes));

    OutputBuffer = NewBuffer;
    OutputDeviceMemory = MemoryAllocation;
}

void Vulkan::Device::DestroyBuffer(Vulkan::Device::DeviceState const & State, VkBuffer & Buffer)
{
    if (Buffer != VK_NULL_HANDLE)
    {
        vkDestroyBuffer(State.Device, Buffer, nullptr);
        Buffer = VK_NULL_HANDLE;
    }
}