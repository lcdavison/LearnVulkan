#include "Graphics/Instance.hpp"

#include <vector>
#include <string>

static std::vector<VkExtensionProperties> AvailableExtensions = {};

static bool const CheckInstanceExtensions()
{
    uint32 ExtensionCount = {};
    VERIFY_VKRESULT(VulkanFunctions::vkEnumerateInstanceExtensionProperties(nullptr, &ExtensionCount, nullptr));

    AvailableExtensions.resize(ExtensionCount);

    VERIFY_VKRESULT(VulkanFunctions::vkEnumerateInstanceExtensionProperties(nullptr, &ExtensionCount, AvailableExtensions.data()));

#if defined(_DEBUG)
    for (VkExtensionProperties const & Extension : AvailableExtensions)
    {
        std::basic_string ExtensionName = Extension.extensionName;
        ExtensionName += "\n";
        ::OutputDebugStringA(ExtensionName.c_str());
    }
#endif

    return true;
}

bool const Vulkan::Instance::CreateInstance(VkApplicationInfo const & ApplicationInfo, InstanceState & OutputInstanceState)
{
    bool bResult = false;

    bResult = CheckInstanceExtensions();

    std::vector ExtensionNames = std::vector<char const *>();
    for (VkExtensionProperties const & Extension : AvailableExtensions)
    {
        ExtensionNames.push_back(Extension.extensionName);
    }

    VkInstanceCreateInfo InstanceCreateInfo = {};
    InstanceCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    InstanceCreateInfo.pApplicationInfo = &ApplicationInfo;
    InstanceCreateInfo.enabledExtensionCount = AvailableExtensions.size();
    InstanceCreateInfo.ppEnabledExtensionNames = ExtensionNames.data();

    VERIFY_VKRESULT(VulkanFunctions::vkCreateInstance(&InstanceCreateInfo, nullptr, &OutputInstanceState.Instance));
    if (OutputInstanceState.Instance == VK_NULL_HANDLE)
    {
        ::MessageBox(nullptr, TEXT("Failed to create vulkan instance"), TEXT("Fatal Error"), MB_OK);
    }

    return bResult;
}
