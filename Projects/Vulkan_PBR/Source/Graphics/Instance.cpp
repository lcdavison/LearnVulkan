#include "Graphics/Instance.hpp"

#include "VulkanPBR.hpp"

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

static std::vector<char const *> const RequiredLayerNames =
{
#if _DEBUG
    "VK_LAYER_KHRONOS_validation",
#endif
};

static bool const HasRequiredLayers()
{
    uint32 PropertyCount = {};
    VERIFY_VKRESULT(VulkanFunctions::vkEnumerateInstanceLayerProperties(&PropertyCount, nullptr));

    std::vector AvailableLayers = std::vector<VkLayerProperties>(PropertyCount);

    VERIFY_VKRESULT(VulkanFunctions::vkEnumerateInstanceLayerProperties(&PropertyCount, AvailableLayers.data()));

#if _DEBUG
    {
        char const OutputPrefix [] = "Layer: ";

        std::string LayerName = std::string();
        LayerName.reserve(sizeof(OutputPrefix) + VK_MAX_EXTENSION_NAME_SIZE);

        for (VkLayerProperties const & Layer : AvailableLayers)
        {
            LayerName.clear();

            LayerName += OutputPrefix;
            LayerName += Layer.layerName;
            LayerName += "\n";

            ::OutputDebugStringA(LayerName.c_str());
        }
    }
#endif

    uint32 MatchedLayerCount = { 0u };

    for (char const * RequiredLayerName : RequiredLayerNames)
    {
        for (VkLayerProperties const & Layer : AvailableLayers)
        {
            if (std::strcmp(RequiredLayerName, Layer.layerName))
            {
                MatchedLayerCount++;
                break;
            }
        }
    }

    return MatchedLayerCount == RequiredLayerNames.size();
}

static bool const HasRequiredExtensions()
{
    uint32 ExtensionCount = {};
    VERIFY_VKRESULT(VulkanFunctions::vkEnumerateInstanceExtensionProperties(nullptr, &ExtensionCount, nullptr));

    std::vector AvailableExtensions = std::vector<VkExtensionProperties>(ExtensionCount);

    VERIFY_VKRESULT(VulkanFunctions::vkEnumerateInstanceExtensionProperties(nullptr, &ExtensionCount, AvailableExtensions.data()));

#if _DEBUG
    {
        char const OutputPrefix [] = "Instance Extension: ";

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

bool const Vulkan::Instance::CreateInstance(VkApplicationInfo const & ApplicationInfo, InstanceState & OutputState)
{
    bool bResult = false;

    InstanceState IntermediateState = {};

    if (::HasRequiredExtensions() &&
        ::HasRequiredLayers())
    {
        VkInstanceCreateInfo InstanceCreateInfo = {};
        InstanceCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        InstanceCreateInfo.pApplicationInfo = &ApplicationInfo;
        InstanceCreateInfo.enabledExtensionCount = RequiredExtensions.size();
        InstanceCreateInfo.ppEnabledExtensionNames = RequiredExtensions.data();
        InstanceCreateInfo.enabledLayerCount = RequiredLayerNames.size();
        InstanceCreateInfo.ppEnabledLayerNames = RequiredLayerNames.data();

        VERIFY_VKRESULT(VulkanFunctions::vkCreateInstance(&InstanceCreateInfo, nullptr, &IntermediateState.Instance));

        std::unordered_set const ExtensionSet = std::unordered_set<std::string>(RequiredExtensions.cbegin(), RequiredExtensions.cend());

        if (::LoadInstanceFunctions(IntermediateState.Instance) &&
            ::LoadInstanceExtensionFunctions(IntermediateState.Instance, ExtensionSet))
        {
            VkWin32SurfaceCreateInfoKHR SurfaceCreateInfo = {};
            SurfaceCreateInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
            SurfaceCreateInfo.hinstance = Application::State.ProcessHandle;
            SurfaceCreateInfo.hwnd = Application::State.WindowHandle;

            VERIFY_VKRESULT(VulkanFunctions::vkCreateWin32SurfaceKHR(IntermediateState.Instance, &SurfaceCreateInfo, nullptr, &IntermediateState.Surface));

            OutputState = IntermediateState;
            bResult = true;
        }
    }

    return bResult;
}

void Vulkan::Instance::DestroyInstance(InstanceState & State)
{
    if (State.Surface)
    {
        VulkanFunctions::vkDestroySurfaceKHR(State.Instance, State.Surface, nullptr);
        State.Surface = VK_NULL_HANDLE;
    }

    if (State.Instance)
    {
        VulkanFunctions::vkDestroyInstance(State.Instance, nullptr);
        State.Instance = VK_NULL_HANDLE;
    }
}
