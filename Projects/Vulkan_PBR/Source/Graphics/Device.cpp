#include "Graphics/Device.hpp"

#include <vector>
#include <string>
#include <algorithm>

static std::vector<VkExtensionProperties> AvailableExtensions = {};

static bool const GetDeviceExtensions(VkPhysicalDevice Device)
{
    uint32 ExtensionCount = {};
    VERIFY_VKRESULT(VulkanFunctions::vkEnumerateDeviceExtensionProperties(Device, nullptr, &ExtensionCount, nullptr));

    AvailableExtensions.resize(ExtensionCount);

    VERIFY_VKRESULT(VulkanFunctions::vkEnumerateDeviceExtensionProperties(Device, nullptr, &ExtensionCount, AvailableExtensions.data()));

    return true;
}

inline static bool const IsSuitableDevice(VkPhysicalDeviceProperties const & DeviceProperties, VkPhysicalDeviceFeatures const & DeviceFeatures)
{
    return DeviceProperties.deviceType == VkPhysicalDeviceType::VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;
}

static bool const EnumerateDevices(VkInstance Instance, std::vector<VkPhysicalDevice> & OutputPhysicalDevices)
{
    uint32 DeviceCount = {};
    VERIFY_VKRESULT(VulkanFunctions::vkEnumeratePhysicalDevices(Instance, &DeviceCount, nullptr));

    if (DeviceCount == 0u)
    {
        ::MessageBox(nullptr, TEXT("No devices with vulkan support"), TEXT("Fatal Error"), MB_OK);
        return false;
    }

    OutputPhysicalDevices.resize(DeviceCount);

    VERIFY_VKRESULT(VulkanFunctions::vkEnumeratePhysicalDevices(Instance, &DeviceCount, OutputPhysicalDevices.data()));

    return true;
}

static bool const SelectPhysicalDevice(VkInstance Instance, VkPhysicalDevice & OutputSelectedDevice)
{
    std::vector Devices = std::vector<VkPhysicalDevice>();
    if (!::EnumerateDevices(Instance, Devices))
    {
        return false;
    }

    VkPhysicalDevice SelectedDevice = {};

    for (VkPhysicalDevice Device : Devices)
    {
        VkPhysicalDeviceProperties DeviceProperties = {};
        VulkanFunctions::vkGetPhysicalDeviceProperties(Device, &DeviceProperties);

        VkPhysicalDeviceFeatures DeviceFeatures = {};
        VulkanFunctions::vkGetPhysicalDeviceFeatures(Device, &DeviceFeatures);

        if (IsSuitableDevice(DeviceProperties, DeviceFeatures))
        {
            std::basic_string Output = "Selected Device: ";
            Output += DeviceProperties.deviceName;
            Output += "\n";
            ::OutputDebugStringA(Output.c_str());

            SelectedDevice = Device;
            break;
        }
    }

    OutputSelectedDevice = SelectedDevice;

    return true;
}

bool const Vulkan::Device::CreateDevice(VkInstance Instance, DeviceState & OutputDeviceState)
{
    DeviceState State = {};

    ::SelectPhysicalDevice(Instance, State.PhysicalDevice);

    /* Query device queue families */
    uint32 PropertyCount = {};
    VulkanFunctions::vkGetPhysicalDeviceQueueFamilyProperties(State.PhysicalDevice, &PropertyCount, nullptr);

    std::vector Properties = std::vector<VkQueueFamilyProperties>(PropertyCount);
    VulkanFunctions::vkGetPhysicalDeviceQueueFamilyProperties(State.PhysicalDevice, &PropertyCount, Properties.data());

    {
        uint32 CurrentFamilyIndex = {};
        for (VkQueueFamilyProperties const & FamilyProperties : Properties)
        {
            if (FamilyProperties.queueFlags & VkQueueFlagBits::VK_QUEUE_GRAPHICS_BIT)
            {
                State.GraphicsQueueFamilyIndex = CurrentFamilyIndex;
                break;
            }

            CurrentFamilyIndex++;
        }
    }

    VkDeviceQueueCreateInfo QueueCreateInfo = {};
    QueueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    QueueCreateInfo.queueFamilyIndex = State.GraphicsQueueFamilyIndex;
    QueueCreateInfo.queueCount = 1u;

    std::vector ExtensionNames = std::vector<char const *>(AvailableExtensions.size());
    for (VkExtensionProperties const & Extension : AvailableExtensions)
    {
#if defined(_DEBUG)
        std::basic_string ExtensionName = "Found Extension: ";
        ExtensionName += Extension.extensionName;
        ExtensionName += "\n";
        ::OutputDebugStringA(ExtensionName.c_str());
#endif

        ExtensionNames.push_back(Extension.extensionName);
    }

    VkDeviceCreateInfo CreateInfo = {};
    CreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    CreateInfo.queueCreateInfoCount = 1u;
    CreateInfo.pQueueCreateInfos = &QueueCreateInfo;
    /* Should figure out what extensions are actually necessary */
    //CreateInfo.enabledExtensionCount = AvailableExtensions.size();
    //CreateInfo.ppEnabledExtensionNames = ExtensionNames.data();
    /* Need to figure out what features need to be enabled */
    //CreateInfo.pEnabledFeatures;

    VERIFY_VKRESULT(VulkanFunctions::vkCreateDevice(State.PhysicalDevice, &CreateInfo, nullptr, &State.Device));

    OutputDeviceState = State;

    return true;
}

void Vulkan::Device::DestroyDevice(DeviceState & State)
{
    if (State.Device)
    {
        VulkanFunctions::vkDestroyDevice(State.Device, nullptr);
    }
}