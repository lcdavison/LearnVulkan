project(VulkanPBR)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories("./Include")

list(APPEND HeaderFiles
"Include/VulkanPBR.hpp"
"Include/Graphics/Vulkan.hpp"
"Include/Graphics/Instance.hpp"
"Include/ErrorHandling.hpp"
)

list(APPEND InlineFiles
"Include/Graphics/VulkanFunctions.inl"
)

list(APPEND SourceFiles
"Source/VulkanPBR.cpp"
"Source/Graphics/Vulkan.cpp"
"Source/Graphics/Instance.cpp"
"Source/MacroMagic.cpp"
)

add_executable(VulkanPBR WIN32 ${IncludeFiles} ${InlineFiles} ${SourceFiles})

# This is because I am using Macro magic
target_compile_definitions(VulkanPBR PRIVATE VK_NO_PROTOTYPES)