cmake_minimum_required(VERSION 3.20)

project(BMPLoader)

list(
    APPEND HeaderFiles
    "Include/BMPLoader/BMPLoader.hpp"
)

list(
    APPEND SourceFiles
    "Source/BMPLoader.cpp"
)

add_library(BMPLoader SHARED)

target_include_directories(
    BMPLoader
    PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/Include"
)

target_sources(
    BMPLoader
    PRIVATE ${HeaderFiles}
    PRIVATE ${SourceFiles}
)

target_compile_definitions(
    BMPLoader
    PRIVATE BMP_LOADER_EXPORT
)