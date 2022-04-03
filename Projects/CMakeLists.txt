cmake_minimum_required(VERSION 3.20)

project(LearnVulkan)

include_directories("../External/Include")

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

include(FetchContent)

message("Fetching GLSLang...")

FetchContent_Declare(
    external-glslang
    GIT_REPOSITORY https://github.com/KhronosGroup/glslang.git
    GIT_TAG master-tot
)

message("Fetch Complete.")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/Runtime")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/Development")

set(BUILD_SHARED_LIBS ON)

# GLSLang Variable Configuration
set(ENABLE_GLSLANG_BINARIES OFF)
set(ENABLE_CTEST OFF)
set(ENABLE_HLSL OFF)
set(ENABLE_SPVREMAPPER OFF)
set(SKIP_GLSLANG_INSTALL ON)
set(BUILD_EXTERNAL OFF)

FetchContent_MakeAvailable(external-glslang)

set_target_properties(
    glslang
    OGLCompiler
    OSDependent
    SPIRV
    PROPERTIES FOLDER "External"
)

add_subdirectory("./Vulkan_Wrapper")
add_subdirectory("./Vulkan_PBR")

set_target_properties(
    VulkanWrapper
    VulkanPBR
    PROPERTIES FOLDER "PBR"
)