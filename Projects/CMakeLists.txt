cmake_minimum_required(VERSION 3.20)

project(LearnVulkan)

include_directories("../External/Include")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/Runtime")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/Development")

add_subdirectory("./Vulkan_Wrapper")
add_subdirectory("./Vulkan_PBR")
