cmake_minimum_required(VERSION 3.20)

project(VulkanWrapper)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

list(
    APPEND HeaderFiles
    "Include/Vulkan.hpp"
    "Include/VulkanWin32.hpp"
)

list(
    APPEND SourceFiles 
    "Source/Vulkan.cpp"
    "Source/VulkanFunctions.inl"
)

add_library(VulkanWrapper SHARED)

target_include_directories(
    VulkanWrapper
    SYSTEM
    PRIVATE $<TARGET_PROPERTY:Vulkan-Headers,INTERFACE_INCLUDE_DIRECTORIES>
)

target_include_directories(
    VulkanWrapper 
    PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/Include"
)

target_sources(
    VulkanWrapper
    PRIVATE ${HeaderFiles}
    PRIVATE ${SourceFiles}
)

target_compile_definitions(
    VulkanWrapper 
    PRIVATE VULKAN_WRAPPER_EXPORT
    PUBLIC VK_NO_PROTOTYPES
    PUBLIC VK_USE_PLATFORM_WIN32_KHR
)

# Treat warnings as errors
target_compile_options(
    VulkanWrapper
    PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX>
)