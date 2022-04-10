cmake_minimum_required(VERSION 3.20)

add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/Vulkan_Wrapper")
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/Vulkan_PBR")

set_target_properties(
    VulkanWrapper
    VulkanPBR
    PROPERTIES FOLDER "PBR"
)