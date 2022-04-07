cmake_minimum_required(VERSION 3.20)

project(LearnVulkan)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/Runtime")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/Development")

#set(BUILD_SHARED_LIBS ON)

add_subdirectory("./External")
add_subdirectory("./Projects")