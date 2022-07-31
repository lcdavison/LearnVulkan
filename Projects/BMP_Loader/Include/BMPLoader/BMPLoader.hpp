#pragma once

#include <filesystem>

#if defined(BMP_LOADER_EXPORT)
    #define BMP_LOADER_API __declspec(dllexport)
#else
    #define BMP_LOADER_API __declspec(dllimport)
#endif

namespace BMPLoader
{
    struct BMPImageData
    {
        std::byte * RawData = {};
        std::uint64_t DataSizeInBytes = {};
        std::uint32_t WidthInPixels = {};
        std::uint32_t HeightInPixels = {};
        std::uint8_t BitsPerPixel = {};
    };

    extern BMP_LOADER_API bool const LoadFile(std::filesystem::path const & FilePath, BMPImageData & OutputImageData);
}