project(VulkanPBR)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories("./Include")

list(APPEND HeaderFiles
"Include/VulkanPBR.hpp"
"Include/Vulkan.hpp"
"Include/ErrorHandling.hpp"
)

list(APPEND InlineFiles
"Source/VulkanFunctions.inl"
)

list(APPEND SourceFiles
"Source/VulkanPBR.cpp"
"Source/Vulkan.cpp"
"Source/MacroMagic.cpp"
)

add_executable(VulkanPBR WIN32 ${IncludeFiles} ${InlineFiles} ${SourceFiles})

target_compile_definitions(VulkanPBR PRIVATE VK_NO_PROTOTYPES)