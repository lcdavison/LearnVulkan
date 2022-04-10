cmake_minimum_required(VERSION 3.20)

# GLSLang Variable Configuration
set(ENABLE_GLSLANG_BINARIES OFF)
set(ENABLE_CTEST OFF)
set(ENABLE_HLSL OFF)
set(ENABLE_SPVREMAPPER OFF)
set(SKIP_GLSLANG_INSTALL ON)
set(BUILD_EXTERNAL OFF)

add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/glslang")

set_target_properties(
    glslang
    OGLCompiler
    OSDependent
    SPIRV
    PROPERTIES FOLDER "External"
)