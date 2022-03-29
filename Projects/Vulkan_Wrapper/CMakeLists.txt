cmake_minimum_required(VERSION 3.20)

project(VulkanWrapper)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

list(APPEND HeaderFiles 
"Include/Vulkan.hpp"
)

list(APPEND SourceFiles 
"Source/Vulkan.cpp"
"Source/VulkanFunctions.inl"
)

add_library(VulkanWrapper SHARED ${HeaderFiles} ${SourceFiles})

target_include_directories(VulkanWrapper 
PUBLIC "./Include"
)

target_sources(VulkanWrapper
PRIVATE ${HeaderFiles}
PRIVATE ${SourceFiles}
)

target_compile_definitions(VulkanWrapper 
PRIVATE VULKAN_WRAPPER_EXPORT
)