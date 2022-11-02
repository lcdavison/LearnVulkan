cmake_minimum_required(VERSION 3.20)

project(OBJLoader)

list(
    APPEND HeaderFiles
    "Include/OBJLoader/OBJLoader.hpp"
)

list(
    APPEND SourceFiles
    "Source/OBJLoader.cpp"
)

add_library(OBJLoader SHARED)

target_include_directories(
    OBJLoader
    PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/Include"
)

target_sources(
    OBJLoader
    PRIVATE ${HeaderFiles}
    PRIVATE ${SourceFiles}
)

target_compile_definitions(
    OBJLoader
    PRIVATE OBJ_LOADER_EXPORT
)
