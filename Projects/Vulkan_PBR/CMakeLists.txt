cmake_minimum_required(VERSION 3.20)

project(VulkanPBR)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(USE_UNICODE OFF)

list(
    APPEND HeaderFiles
    "Include/Assets/Texture.hpp"
    "Include/Assets/Material.hpp"
    "Include/Assets/StaticMesh.hpp"
    "Include/Components/StaticMeshComponent.hpp"
    "Include/Components/TransformComponent.hpp"
    "Include/Graphics/Allocators.hpp"
    "Include/Graphics/VulkanModule.hpp"
    "Include/Graphics/Instance.hpp"
    "Include/Graphics/Device.hpp"
    "Include/Graphics/Memory.hpp"
    "Include/Graphics/Viewport.hpp"
    "Include/Graphics/ShaderLibrary.hpp"
    "Include/Graphics/Descriptors.hpp"
    "Include/Input/InputManager.hpp"
    "Include/Platform/Windows.hpp"
    "Include/ShaderCompiler/ShaderCompiler.hpp"
    "Include/Utilities/Containers.hpp"
    "Include/Utilities/Iterator.hpp"
    "Include/Utilities/String.hpp"
    "Include/Common.hpp"
    "Include/CommonTypes.hpp"
    "Include/Camera.hpp"
    "Include/ForwardRenderer.hpp"
    "Include/Logging.hpp"
    "Include/Scene.hpp"
    "Include/VulkanPBR.hpp"
)
    
list(
    APPEND SourceFiles
    "Source/Assets/Texture.cpp"
    "Source/Assets/Material.cpp"
    "Source/Assets/StaticMesh.cpp"
    "Source/Components/StaticMeshComponent.cpp"
    "Source/Components/TransformComponent.cpp"
    "Source/Graphics/VulkanModule.cpp"
    "Source/Graphics/Allocators.cpp"
    "Source/Graphics/Instance.cpp"
    "Source/Graphics/Device.cpp"
    "Source/Graphics/Memory.cpp"
    "Source/Graphics/Viewport.cpp"
    "Source/Graphics/ShaderLibrary.cpp"
    "Source/Graphics/Descriptors.cpp"
    "Source/Input/InputManager.cpp"
    "Source/Platform/Windows.cpp"
    "Source/ShaderCompiler/ShaderCompiler.cpp"
    "Source/Camera.cpp"
    "Source/ForwardRenderer.cpp"
    "Source/Logging.cpp"
    "Source/Scene.cpp"
    "Source/VulkanPBR.cpp"
)

add_executable(VulkanPBR WIN32)

target_include_directories(
    VulkanPBR
    SYSTEM
    PRIVATE "${CMAKE_SOURCE_DIR}/External/vulkan/Include"
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

# TODO: Setup Unicode stuff properly
target_compile_definitions(
    VulkanPBR
    PRIVATE $<TARGET_PROPERTY:VulkanWrapper,INTERFACE_COMPILE_DEFINITIONS>
    PRIVATE USE_UNICODE=$<IF:$<BOOL:${USE_UNICODE}>,1,0>
    PRIVATE $<$<BOOL:${USE_UNICODE}>:_UNICODE>
)

target_compile_options(
    VulkanPBR
    PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX>
    PRIVATE $<$<CONFIG:Release>:/Ot /Ob2>
)

target_link_libraries(
    VulkanPBR
    glslang
    SPIRV
    VulkanWrapper
    MathLib
    OBJLoader
    BMPLoader
    dwmapi
)

# This is for debugging the .exe since we have required resources here
if(MSVC)
    set_property(
        TARGET VulkanPBR 
        PROPERTY VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    )
endif()