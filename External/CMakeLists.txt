cmake_minimum_required(VERSION 3.20)

# GLSLang Variable Configuration
option(ENABLE_GLSLANG_BINARIES OFF)
option(ENABLE_CTEST OFF)
option(ENABLE_HLSL OFF)
option(ENABLE_SPVREMAPPER OFF)
option(SKIP_GLSLANG_INSTALL ON)
option(BUILD_EXTERNAL OFF)

add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/Vulkan-Headers")
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/glslang")

set_target_properties(
    Vulkan-Headers
    glslang
    SPIRV
    OGLCompiler
    OSDependent
    GenericCodeGen
    MachineIndependent
    PROPERTIES FOLDER "External"
)