cmake_minimum_required(VERSION 3.20)

project(VulkanWrapper)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

list(
    APPEND SourceFiles 
    "Source/Vulkan.cpp"
)

add_library(VulkanWrapper SHARED)

target_include_directories(
    VulkanWrapper
    SYSTEM
    PRIVATE "${LearnVulkan_SOURCE_DIR}/External/vulkan/Include"
)

target_include_directories(
    VulkanWrapper 
    PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/Include"
)

target_sources(
    VulkanWrapper
    PRIVATE ${SourceFiles}
)

target_compile_definitions(
    VulkanWrapper 
    PRIVATE VULKAN_WRAPPER_EXPORT
    PUBLIC VK_NO_PROTOTYPES
    PUBLIC VK_USE_PLATFORM_WIN32_KHR
)