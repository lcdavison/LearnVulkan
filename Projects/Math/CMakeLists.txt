cmake_minimum_required(VERSION 3.20)

project(MathLib)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

list(
    APPEND HeaderFiles
    "Include/Math/Vector.hpp"
    "Include/Math/Matrix.hpp"
    "Include/Math/Transform.hpp"
    "Include/Math/Utilities.hpp"
)

list(
    APPEND SourceFiles
    "Source/Vector.cpp"
    "Source/Matrix.cpp"
    "Source/Transform.cpp"
)

add_library(MathLib STATIC)

target_include_directories(
    MathLib
    PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/Include"
)

target_sources(
    MathLib
    PRIVATE ${HeaderFiles}
    PRIVATE ${SourceFiles}
)

target_compile_options(
    MathLib
    PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX>
)

target_compile_definitions(
    MathLib
    PRIVATE $<$<BOOL:SUPPORTS_SSE2>:USE_SSE2>
)