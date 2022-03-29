cmake_minimum_required(VERSION 3.20)

project(VulkanPBR)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

list(APPEND HeaderFiles
"Include/VulkanPBR.hpp"
"Include/Graphics/VulkanModule.hpp"
"Include/Graphics/Instance.hpp"
"Include/Graphics/Device.hpp"
"Include/Graphics/Viewport.hpp"
"Include/ErrorHandling.hpp"
"Include/CommonTypes.hpp"
"Include/ForwardRenderer.hpp"
)

list(APPEND SourceFiles
"Source/VulkanPBR.cpp"
"Source/Graphics/VulkanModule.cpp"
"Source/Graphics/Instance.cpp"
"Source/Graphics/Device.cpp"
"Source/Graphics/Viewport.cpp"
"Source/ForwardRenderer.cpp"
)

add_executable(VulkanPBR WIN32)

target_include_directories(VulkanPBR
PRIVATE "$<TARGET_PROPERTY:VulkanWrapper,INTERFACE_INCLUDE_DIRECTORIES>"
PRIVATE "./Include"
)

target_sources(VulkanPBR
PRIVATE ${HeaderFiles}
PRIVATE ${SourceFiles}
)

target_compile_definitions(VulkanPBR
PRIVATE "$<TARGET_PROPERTY:VulkanWrapper,INTERFACE_COMPILE_DEFINITIONS>"
)

target_link_libraries(VulkanPBR VulkanWrapper)