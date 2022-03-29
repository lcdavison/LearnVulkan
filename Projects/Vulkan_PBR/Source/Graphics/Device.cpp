#include "Graphics/Device.hpp"

#include "Graphics/Instance.hpp"

#include <Windows.h>

#include <vector>
#include <string>

static std::vector<char const *> RequiredExtensionNames =
{
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
};

static bool const HasRequiredLayers(VkPhysicalDevice Device)
{
    uint32 LayerCount = {};
    VERIFY_VKRESULT(vkEnumerateDeviceLayerProperties(Device, &LayerCount, nullptr));

    std::vector AvailableLayers = std::vector<VkLayerProperties>(LayerCount);

    VERIFY_VKRESULT(vkEnumerateDeviceLayerProperties(Device, &LayerCount, AvailableLayers.data()));



    return true;
}

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

    for (char const * ExtensionName : RequiredExtensionNames)
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

    return MatchedExtensionCount == RequiredExtensionNames.size();
}

inline static bool const IsSuitableDevice(VkPhysicalDeviceProperties const & DeviceProperties, VkPhysicalDeviceFeatures const & DeviceFeatures)
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

            if (::IsSuitableDevice(DeviceProperties, DeviceFeatures))
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
        CreateInfo.enabledExtensionCount = RequiredExtensionNames.size();
        CreateInfo.ppEnabledExtensionNames = RequiredExtensionNames.data();
        /* Need to figure out what features need to be enabled */
        //CreateInfo.pEnabledFeatures;

        VERIFY_VKRESULT(vkCreateDevice(IntermediateState.PhysicalDevice, &CreateInfo, nullptr, &IntermediateState.Device));

        if (::LoadDeviceFunctions(IntermediateState.Device) &&
            ::LoadDeviceExtensionFunctions(IntermediateState.Device))
        {
            vkGetDeviceQueue(IntermediateState.Device, IntermediateState.GraphicsQueueFamilyIndex, 0u, &IntermediateState.GraphicsQueue);

            OutputState = IntermediateState;
            bResult = true;
        }
    }

    return bResult;
}

void Vulkan::Device::DestroyDevice(Vulkan::Device::DeviceState & State)
{
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