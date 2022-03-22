#include "Graphics/Instance.hpp"

#include <Windows.h>
#include "CommonTypes.hpp"

#include <vector>
#include <string>
#include <unordered_set>

extern bool const LoadInstanceFunctions(VkInstance);
extern bool const LoadInstanceExtensionFunctions(VkInstance, std::unordered_set<std::string> const &);

static std::vector<char const *> const RequiredExtensions = 
{
    VK_KHR_SURFACE_EXTENSION_NAME,
    VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
};

static bool const HasRequiredExtensions()
{
    uint32 ExtensionCount = {};
    VERIFY_VKRESULT(VulkanFunctions::vkEnumerateInstanceExtensionProperties(nullptr, &ExtensionCount, nullptr));

    std::vector AvailableExtensions = std::vector<VkExtensionProperties>(ExtensionCount);

    VERIFY_VKRESULT(VulkanFunctions::vkEnumerateInstanceExtensionProperties(nullptr, &ExtensionCount, AvailableExtensions.data()));

#if defined(_DEBUG)
    for (VkExtensionProperties const & Extension : AvailableExtensions)
    {
        std::string ExtensionName = "Instance Extension: ";
        ExtensionName += Extension.extensionName;
        ExtensionName += "\n";

        ::OutputDebugStringA(ExtensionName.c_str());
    }
#endif

    uint16 MatchedExtensionCount = { 0u };

    for (char const * ExtensionName : RequiredExtensions)
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

    return MatchedExtensionCount == RequiredExtensions.size();
}

bool const Vulkan::Instance::CreateInstance(VkApplicationInfo const & ApplicationInfo, InstanceState & OutputInstanceState)
{
    bool bResult = false;

    InstanceState IntermediateState = {};

    if (::HasRequiredExtensions())
    {
        VkInstanceCreateInfo InstanceCreateInfo = {};
        InstanceCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        InstanceCreateInfo.pApplicationInfo = &ApplicationInfo;
        InstanceCreateInfo.enabledExtensionCount = RequiredExtensions.size();
        InstanceCreateInfo.ppEnabledExtensionNames = RequiredExtensions.data();

        VERIFY_VKRESULT(VulkanFunctions::vkCreateInstance(&InstanceCreateInfo, nullptr, &IntermediateState.Instance));

        bool const bFunctionsLoaded = ::LoadInstanceFunctions(IntermediateState.Instance) &&
                                      ::LoadInstanceExtensionFunctions(IntermediateState.Instance,
                                                                       std::unordered_set<std::string>(RequiredExtensions.cbegin(), RequiredExtensions.cend()));

        if (bFunctionsLoaded)
        {
            OutputInstanceState = IntermediateState;
            bResult = true;
        }
    }

    return bResult;
}

void Vulkan::Instance::DestroyInstance(InstanceState & State)
{
    if (State.Instance)
    {
        VulkanFunctions::vkDestroyInstance(State.Instance, nullptr);
        State.Instance = VK_NULL_HANDLE;
    }
}
