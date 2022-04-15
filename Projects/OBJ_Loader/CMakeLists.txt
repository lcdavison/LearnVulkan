cmake_minimum_required(VERSION 3.20)

project(OBJLoader)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

list(
    APPEND HeaderFiles
    "./Include/OBJLoader.hpp"
)

list(
    APPEND SourceFiles
    "./Source/OBJLoader.cpp"
)

add_library(OBJLoader SHARED)

target_include_directories(
    OBJLoader
    PUBLIC "./Include"
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
