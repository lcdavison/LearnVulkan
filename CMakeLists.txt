cmake_minimum_required(VERSION 3.20)

project(LearnVulkan)

if(PROJECT_SOURCE_DIR STREQUAL PROJECT_BINARY_DIR)
    message(FATAL_ERROR "In-Source builds are not allowed.")
endif()

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/Runtime")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/Development")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

cmake_host_system_information(
    RESULT SUPPORTS_SSE2
    QUERY HAS_SSE2
)

if(SUPPORTS_SSE2)
    message(STATUS "Host Supports SSE2")
endif()

if(CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_CONFIGURATION_TYPES Debug Release)
    set(
        CMAKE_CONFIGURATION_TYPES "${CMAKE_CONFIGURATION_TYPES}" CACHE STRING
        "Set configuration types."
        FORCE)
endif()

cmake_policy(SET CMP0077 NEW)

#set(BUILD_SHARED_LIBS ON)

add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/Projects")
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/External")