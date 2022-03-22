#include "Graphics/Device.hpp"

#include <Windows.h>

#include <vector>
#include <string>

extern bool const LoadDeviceFunctions(VkDevice);

static std::vector<char const *> RequiredExtensionNames = 
{
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
};

static bool const HasRequiredExtensions(VkPhysicalDevice Device)
{
    uint32 ExtensionCount = {};
    VERIFY_VKRESULT(VulkanFunctions::vkEnumerateDeviceExtensionProperties(Device, nullptr, &ExtensionCount, nullptr));

    std::vector AvailableExtensions = std::vector<VkExtensionProperties>(ExtensionCount);

    VERIFY_VKRESULT(VulkanFunctions::vkEnumerateDeviceExtensionProperties(Device, nullptr, &ExtensionCount, AvailableExtensions.data()));

#if defined(_DEBUG)
    for (VkExtensionProperties const & Extension : AvailableExtensions)
    {
        std::string ExtensionName = "Device Extension: ";
        ExtensionName += Extension.extensionName;
        ExtensionName += "\n";

        ::OutputDebugStringA(ExtensionName.c_str());
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
    VERIFY_VKRESULT(VulkanFunctions::vkEnumeratePhysicalDevices(Instance, &DeviceCount, nullptr));

    if (DeviceCount > 0u)
    {
        OutputPhysicalDevices.resize(DeviceCount);

        VERIFY_VKRESULT(VulkanFunctions::vkEnumeratePhysicalDevices(Instance, &DeviceCount, OutputPhysicalDevices.data()));

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
            VulkanFunctions::vkGetPhysicalDeviceProperties(Device, &DeviceProperties);

            VkPhysicalDeviceFeatures DeviceFeatures = {};
            VulkanFunctions::vkGetPhysicalDeviceFeatures(Device, &DeviceFeatures);

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

bool const Vulkan::Device::CreateDevice(VkInstance Instance, DeviceState & OutputDeviceState)
{
    bool bResult = false;

    /* Use this for atomicity, we don't want to partially fill OutputDeviceState */
    DeviceState IntermediateState = {};

    if (::SelectPhysicalDevice(Instance, IntermediateState.PhysicalDevice) &&
        ::HasRequiredExtensions(IntermediateState.PhysicalDevice))
    {
        /* Find Queue family with required capabilities */

        /* Query device queue families */
        uint32 PropertyCount = {};
        VulkanFunctions::vkGetPhysicalDeviceQueueFamilyProperties(IntermediateState.PhysicalDevice, &PropertyCount, nullptr);

        std::vector Properties = std::vector<VkQueueFamilyProperties>(PropertyCount);
        VulkanFunctions::vkGetPhysicalDeviceQueueFamilyProperties(IntermediateState.PhysicalDevice, &PropertyCount, Properties.data());

        {
            uint32 CurrentFamilyIndex = {};
            for (VkQueueFamilyProperties const & FamilyProperties : Properties)
            {
                if (FamilyProperties.queueFlags & VkQueueFlagBits::VK_QUEUE_GRAPHICS_BIT)
                {
                    IntermediateState.GraphicsQueueFamilyIndex = CurrentFamilyIndex;
                    break;
                }

                CurrentFamilyIndex++;
            }
        }

        VkDeviceQueueCreateInfo QueueCreateInfo = {};
        QueueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        QueueCreateInfo.queueFamilyIndex = IntermediateState.GraphicsQueueFamilyIndex;
        QueueCreateInfo.queueCount = 1u;

        VkDeviceCreateInfo CreateInfo = {};
        CreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        CreateInfo.queueCreateInfoCount = 1u;
        CreateInfo.pQueueCreateInfos = &QueueCreateInfo;
        CreateInfo.enabledExtensionCount = RequiredExtensionNames.size();
        CreateInfo.ppEnabledExtensionNames = RequiredExtensionNames.data();
        /* Need to figure out what features need to be enabled */
        //CreateInfo.pEnabledFeatures;

        VERIFY_VKRESULT(VulkanFunctions::vkCreateDevice(IntermediateState.PhysicalDevice, &CreateInfo, nullptr, &IntermediateState.Device));

        LoadDeviceFunctions(IntermediateState.Device);

        OutputDeviceState = IntermediateState;
        bResult = true;
    }

    return bResult;
}

void Vulkan::Device::DestroyDevice(DeviceState & State)
{
    if (State.Device)
    {
        VulkanFunctions::vkDestroyDevice(State.Device, nullptr);
        State.Device = VK_NULL_HANDLE;
    }
}