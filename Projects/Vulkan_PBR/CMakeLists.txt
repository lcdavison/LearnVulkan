cmake_minimum_required(VERSION 3.20)

project(VulkanPBR)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

list(
    APPEND HeaderFiles
    "Include/VulkanPBR.hpp"
    "Include/Graphics/VulkanModule.hpp"
    "Include/Graphics/Instance.hpp"
    "Include/Graphics/Device.hpp"
    "Include/Graphics/Viewport.hpp"
    "Include/Graphics/ShaderLibrary.hpp"
    "Include/ShaderCompiler/ShaderCompiler.hpp"
    "Include/ForwardRenderer.hpp"
)

list(
    APPEND SourceFiles
    "Source/VulkanPBR.cpp"
    "Source/Graphics/VulkanModule.cpp"
    "Source/Graphics/Instance.cpp"
    "Source/Graphics/Device.cpp"
    "Source/Graphics/Viewport.cpp"
    "Source/Graphics/ShaderLibrary.cpp"
    "Source/ShaderCompiler/ShaderCompiler.cpp"
    "Source/ForwardRenderer.cpp"
)

add_executable(VulkanPBR WIN32)

target_include_directories(
    VulkanPBR
    SYSTEM
    PRIVATE "${LearnVulkan_SOURCE_DIR}/External/vulkan/Include"
)

target_include_directories(
    VulkanPBR
    PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/Include"
)

target_sources(
    VulkanPBR
    PRIVATE ${HeaderFiles}
    PRIVATE ${SourceFiles}
)

target_compile_definitions(
    VulkanPBR
    PRIVATE "$<TARGET_PROPERTY:VulkanWrapper,INTERFACE_COMPILE_DEFINITIONS>"
)

target_compile_options(
    VulkanPBR
    PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX>
    PRIVATE $<$<CONFIG:DEBUG>:/Ob0>
    PRIVATE $<$<CONFIG:RELEASE>:/Ot /Ob2>
)

target_link_libraries(
    VulkanPBR
    glslang
    SPIRV
    VulkanWrapper
    MathLib
    OBJLoader
)

# This is for debugging the .exe since we have required resources here
if(MSVC)
    set_property(
        TARGET VulkanPBR 
        PROPERTY VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    )
endif()