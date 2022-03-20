#ifndef VK_EXPORTED_FUNCTION
#define VK_EXPORTED_FUNCTION(FunctionName)
#endif

VK_EXPORTED_FUNCTION(vkGetInstanceProcAddr);

#undef VK_EXPORTED_FUNCTION

#ifndef VK_GLOBAL_FUNCTION
#define VK_GLOBAL_FUNCTION(FunctionName)
#endif

VK_GLOBAL_FUNCTION(vkEnumerateInstanceExtensionProperties);
VK_GLOBAL_FUNCTION(vkEnumerateInstanceLayerProperties);
VK_GLOBAL_FUNCTION(vkCreateInstance);

#undef VK_GLOBAL_FUNCTION

#ifndef VK_INSTANCE_FUNCTION
#define VK_INSTANCE_FUNCTION(FunctionName)
#endif

VK_INSTANCE_FUNCTION(vkGetDeviceProcAddr);
VK_INSTANCE_FUNCTION(vkEnumeratePhysicalDevices);
VK_INSTANCE_FUNCTION(vkEnumerateDeviceExtensionProperties);
VK_INSTANCE_FUNCTION(vkGetPhysicalDeviceProperties);
VK_INSTANCE_FUNCTION(vkGetPhysicalDeviceFeatures);
VK_INSTANCE_FUNCTION(vkGetPhysicalDeviceQueueFamilyProperties);
VK_INSTANCE_FUNCTION(vkCreateDevice);
VK_INSTANCE_FUNCTION(vkDestroyDevice);

#undef VK_INSTANCE_FUNCTION

#ifndef VK_DEVICE_FUNCTION
#define VK_DEVICE_FUNCTION(FunctionName)
#endif

#undef VK_DEVICE_FUNCTION