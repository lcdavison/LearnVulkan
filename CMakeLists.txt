cmake_minimum_required(VERSION 3.20)

project(LearnVulkan)

if(PROJECT_SOURCE_DIR STREQUAL PROJECT_BINARY_DIR)
    message(FATAL_ERROR "In-Source builds are not allowed.")
endif()

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/Runtime")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/Development")

#set(BUILD_SHARED_LIBS ON)

add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/Projects")
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/External")