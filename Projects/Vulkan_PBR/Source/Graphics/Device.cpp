#include "Graphics/Device.hpp"

#include "Graphics/Instance.hpp"

#include <Windows.h>

#include <vector>
#include <string>

struct ResourceCollection
{
    std::vector<VkBuffer> Buffers;
};

struct AllocationInfo
{
    VkDeviceMemory DeviceMemory;
    VkDeviceSize DeviceMemoryOffset;
    uint32 MemoryTypeIndex;
    uint32 AllocationIndex;
};

static uint32 const kMaximumDeviceMemoryAllocationCount = { 4u };
static uint64 const kDefaultMemoryAllocationSizeInBytes = { 4096u };

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

            ::OutputDebugStringA(ExtensionName.c_str());
        }
    }
#endif

    uint16 MatchedExtensionCount = { 0u };

    for (char const * ExtensionName : kRequiredExtensionNames)
    {
        for (VkExtensionProperties const & Extension : AvailableExtensions)
        {
            if (std::strcmp(ExtensionName, Extension.extensionName))
            {
                MatchedExtensionCount++;
                break;
            }
        }
    }

    return MatchedExtensionCount == kRequiredExtensionNames.size();
}

inline static bool const IsSuitableDevice(VkPhysicalDeviceProperties const & DeviceProperties)
{
    return DeviceProperties.deviceType == VkPhysicalDeviceType::VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;
}

static bool const EnumerateDevices(VkInstance Instance, std::vector<VkPhysicalDevice> & OutputPhysicalDevices)
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
        ::MessageBox(nullptr, TEXT("Failed to find a device with Vulkan support."), TEXT("Fatal Error"), MB_OK);
    }

    return bResult;
}

static bool const SelectPhysicalDevice(VkInstance Instance, VkPhysicalDevice & OutputSelectedDevice)
{
    bool bResult = false;

    std::vector Devices = std::vector<VkPhysicalDevice>();
    if (::EnumerateDevices(Instance, Devices))
    {
        VkPhysicalDevice SelectedDevice = {};

        for (VkPhysicalDevice Device : Devices)
        {
            VkPhysicalDeviceProperties DeviceProperties = {};
            vkGetPhysicalDeviceProperties(Device, &DeviceProperties);

            VkPhysicalDeviceFeatures DeviceFeatures = {};
            vkGetPhysicalDeviceFeatures(Device, &DeviceFeatures);

            if (::IsSuitableDevice(DeviceProperties))
            {
#if defined(_DEBUG)
                std::basic_string Output = "Selected Device: ";
                Output += DeviceProperties.deviceName;
                Output += "\n";
                ::OutputDebugStringA(Output.c_str());
#endif

                SelectedDevice = Device;
                break;
            }
        }

        OutputSelectedDevice = SelectedDevice;
        bResult = true;
    }

    return bResult;
}

static bool const SelectQueueFamily(VkPhysicalDevice Device, VkSurfaceKHR Surface, uint32 & OutputFamilyIndex)
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
            if (FamilyProperties.queueFlags & VkQueueFlagBits::VK_QUEUE_GRAPHICS_BIT)
            {
                VkBool32 SupportsPresentation = {};
                VERIFY_VKRESULT(vkGetPhysicalDeviceSurfaceSupportKHR(Device, CurrentFamilyIndex, Surface, &SupportsPresentation));

                if (SupportsPresentation == VK_TRUE)
                {
                    OutputFamilyIndex = CurrentFamilyIndex;
                    bResult = true;
                    break;
                }
            }

            CurrentFamilyIndex++;
        }
    }

    return bResult;
}

static void FindMemoryTypeIndex(VkPhysicalDevice PhysicalDevice, uint32 MemoryTypeMask, VkMemoryPropertyFlags RequiredMemoryTypeFlags, uint32 & OutputMemoryTypeIndex)
{
    VkPhysicalDeviceMemoryProperties MemoryProperties = {};
    vkGetPhysicalDeviceMemoryProperties(PhysicalDevice, &MemoryProperties);

    uint32 SelectedMemoryTypeIndex = { 0u };

    uint32 CurrentMemoryTypeIndex = {};
    while (BitScanForward(reinterpret_cast<DWORD*>(&CurrentMemoryTypeIndex), MemoryTypeMask))
    {
        /* Clear the bit from the mask */
        MemoryTypeMask ^= (1u << CurrentMemoryTypeIndex);

        VkMemoryType const & CurrentMemoryType = MemoryProperties.memoryTypes [CurrentMemoryTypeIndex];

        if ((CurrentMemoryType.propertyFlags & RequiredMemoryTypeFlags) == RequiredMemoryTypeFlags)
        {
            SelectedMemoryTypeIndex = CurrentMemoryTypeIndex;
            break;
        }
    }

    OutputMemoryTypeIndex = SelectedMemoryTypeIndex;
}

static void GetDeviceMemory(Vulkan::Device::DeviceState & State, uint32 const MemoryTypeIndex, uint64 const RequiredSizeInBytes, VkDeviceMemory & OutputDeviceMemory, VkDeviceSize & OutputMemoryOffsetInBytes)
{
    uint32 const MemoryTypeBeginIndex = MemoryTypeIndex * kMaximumDeviceMemoryAllocationCount;

    uint8 const MemoryAllocationMask = State.MemoryAllocationStatusMasks [MemoryTypeIndex];

    for (uint32 CurrentMemoryAllocationIndex = MemoryTypeBeginIndex;
         CurrentMemoryAllocationIndex < MemoryTypeBeginIndex + kMaximumDeviceMemoryAllocationCount;
         CurrentMemoryAllocationIndex++)
    {
        /* If we have reached an empty block then previous blocks were not big enough, so allocate */
        if ((MemoryAllocationMask & (1u << CurrentMemoryAllocationIndex)) == 0u)
        {
            VkMemoryAllocateInfo AllocationInfo = {};
            AllocationInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            AllocationInfo.allocationSize = kDefaultMemoryAllocationSizeInBytes;
            AllocationInfo.memoryTypeIndex = MemoryTypeIndex;

            VERIFY_VKRESULT(vkAllocateMemory(State.Device, &AllocationInfo, nullptr, &State.MemoryAllocations [CurrentMemoryAllocationIndex]));
            State.MemoryAllocationOffsetsInBytes [CurrentMemoryAllocationIndex] = 0u;
            State.MemoryAllocationStatusMasks [MemoryTypeIndex] |= (1u << (CurrentMemoryAllocationIndex & kMaximumDeviceMemoryAllocationCount - 1u));
        }

        if (State.MemoryAllocationOffsetsInBytes [CurrentMemoryAllocationIndex] + RequiredSizeInBytes < kDefaultMemoryAllocationSizeInBytes)
        {
            OutputDeviceMemory = State.MemoryAllocations [CurrentMemoryAllocationIndex];

            OutputMemoryOffsetInBytes = State.MemoryAllocationOffsetsInBytes [CurrentMemoryAllocationIndex];
            State.MemoryAllocationOffsetsInBytes [CurrentMemoryAllocationIndex] += RequiredSizeInBytes;

            break;
        }
    }
}

bool const Vulkan::Device::CreateDevice(Vulkan::Instance::InstanceState const & InstanceState, Vulkan::Device::DeviceState & OutputState)
{
    bool bResult = false;

    /* Use this for atomicity, we don't want to partially fill OutputState */
    DeviceState IntermediateState = {};

    if (::SelectPhysicalDevice(InstanceState.Instance, IntermediateState.PhysicalDevice) &&
        ::HasRequiredExtensions(IntermediateState.PhysicalDevice) &&
        ::SelectQueueFamily(IntermediateState.PhysicalDevice, InstanceState.Surface, IntermediateState.GraphicsQueueFamilyIndex))
    {
        float const QueuePriority = 1.0f;

        VkDeviceQueueCreateInfo QueueCreateInfo = {};
        QueueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        QueueCreateInfo.queueFamilyIndex = IntermediateState.GraphicsQueueFamilyIndex;
        QueueCreateInfo.queueCount = 1u;
        QueueCreateInfo.pQueuePriorities = &QueuePriority;

        VkDeviceCreateInfo CreateInfo = {};
        CreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        CreateInfo.queueCreateInfoCount = 1u;
        CreateInfo.pQueueCreateInfos = &QueueCreateInfo;
        CreateInfo.enabledExtensionCount = static_cast<uint32>(kRequiredExtensionNames.size());
        CreateInfo.ppEnabledExtensionNames = kRequiredExtensionNames.data();
        /* Need to figure out what features need to be enabled */
        //CreateInfo.pEnabledFeatures;

        VERIFY_VKRESULT(vkCreateDevice(IntermediateState.PhysicalDevice, &CreateInfo, nullptr, &IntermediateState.Device));

        if (::LoadDeviceFunctions(IntermediateState.Device) &&
            ::LoadDeviceExtensionFunctions(IntermediateState.Device))
        {
            vkGetDeviceQueue(IntermediateState.Device, IntermediateState.GraphicsQueueFamilyIndex, 0u, &IntermediateState.GraphicsQueue);

            vkGetPhysicalDeviceMemoryProperties(IntermediateState.PhysicalDevice, &IntermediateState.MemoryProperties);

            /* Matrix of memory allocations */
            IntermediateState.MemoryAllocations.resize(IntermediateState.MemoryProperties.memoryTypeCount * kMaximumDeviceMemoryAllocationCount);
            IntermediateState.MemoryAllocationOffsetsInBytes.resize(IntermediateState.MemoryProperties.memoryTypeCount * kMaximumDeviceMemoryAllocationCount);
            IntermediateState.MemoryAllocationStatusMasks.resize(IntermediateState.MemoryProperties.memoryTypeCount);

            OutputState = IntermediateState;
            bResult = true;
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

void Vulkan::Device::CreateCommandPool(Vulkan::Device::DeviceState const & State, VkCommandPool & OutputCommandPool)
{
    VkCommandPoolCreateInfo CreateInfo = {};
    CreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    CreateInfo.queueFamilyIndex = State.GraphicsQueueFamilyIndex;
    CreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

    VERIFY_VKRESULT(vkCreateCommandPool(State.Device, &CreateInfo, nullptr, &OutputCommandPool));
}

void Vulkan::Device::CreateCommandBuffer(Vulkan::Device::DeviceState const & State, VkCommandBufferLevel CommandBufferLevel, VkCommandBuffer & OutputCommandBuffer)
{
    VkCommandBufferAllocateInfo AllocateInfo = {};
    AllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    AllocateInfo.commandPool = State.CommandPool;
    AllocateInfo.commandBufferCount = 1u;
    AllocateInfo.level = CommandBufferLevel;

    VERIFY_VKRESULT(vkAllocateCommandBuffers(State.Device, &AllocateInfo, &OutputCommandBuffer));
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

void Vulkan::Device::CreateBuffer(Vulkan::Device::DeviceState & State, uint64 SizeInBytes, VkBufferUsageFlags UsageFlags, VkMemoryPropertyFlags MemoryFlags, VkBuffer & OutputBuffer, VkDeviceMemory & OutputDeviceMemory)
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

    uint32 MemoryTypeIndex = {};
    ::FindMemoryTypeIndex(State.PhysicalDevice, MemoryRequirements.memoryTypeBits, MemoryFlags, MemoryTypeIndex);

    AllocationInfo BufferAllocationInfo = {};

    VkDeviceMemory DeviceMemory = {};
    VkDeviceSize DeviceMemoryOffset = {};
    ::GetDeviceMemory(State, MemoryTypeIndex, MemoryRequirements.size, DeviceMemory, DeviceMemoryOffset);

    VERIFY_VKRESULT(vkBindBufferMemory(State.Device, NewBuffer, DeviceMemory, DeviceMemoryOffset));

    OutputBuffer = NewBuffer;
    OutputDeviceMemory = DeviceMemory;
}

void Vulkan::Device::DestroyBuffer(Vulkan::Device::DeviceState const & State, VkBuffer & Buffer)
{
    if (Buffer != VK_NULL_HANDLE)
    {
        vkDestroyBuffer(State.Device, Buffer, nullptr);
        Buffer = VK_NULL_HANDLE;
    }
}