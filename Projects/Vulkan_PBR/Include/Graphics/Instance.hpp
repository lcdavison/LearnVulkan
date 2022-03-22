#include "Vulkan.hpp"

namespace Vulkan::Instance
{
    struct InstanceState
    {
        VkInstance Instance;
    };

    extern bool const CreateInstance(VkApplicationInfo const & ApplicationInfo, InstanceState & OutputInstanceState);

    extern void DestroyInstance(InstanceState & State);
}