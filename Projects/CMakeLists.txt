cmake_minimum_required(VERSION 3.20)

cmake_host_system_information(
    RESULT SUPPORTS_SSE2
    QUERY HAS_SSE2
)

if(SUPPORTS_SSE2)
    message(STATUS "Host Supports SSE2")
endif()

add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/BMP_Loader")
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/OBJ_Loader")
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/Math")
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/Vulkan_Wrapper")
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/Vulkan_PBR")

set_target_properties(
    BMPLoader
    OBJLoader
    MathLib
    VulkanWrapper
    VulkanPBR
    PROPERTIES FOLDER "PBR"
)