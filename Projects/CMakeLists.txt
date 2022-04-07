cmake_minimum_required(VERSION 3.20)

include_directories("../External/Include")

add_subdirectory("./Vulkan_Wrapper")
add_subdirectory("./Vulkan_PBR")

set_target_properties(
    VulkanWrapper
    VulkanPBR
    PROPERTIES FOLDER "PBR"
)