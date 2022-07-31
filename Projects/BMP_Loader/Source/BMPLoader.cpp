#include "BMPLoader/BMPLoader.hpp"

#include <fstream>

#pragma pack(1)
struct BMPHeader
{
    std::uint16_t ID = {};
    std::uint32_t SizeInBytes = {};
    std::uint16_t Reserved0 = {};
    std::uint16_t Reserved1 = {};
    std::uint32_t DataStartOffset = {};
};

struct BitmapInfoHeader
{
    std::uint32_t WidthInPixels = {};
    std::uint32_t HeightInPixels = {};
    std::uint16_t ColourPlaneCount = {};
    std::uint16_t BitsPerPixel = {};
    std::uint32_t CompressionMethod = {};
    std::uint32_t ImageSize = {};
    std::int32_t HorizontalResolutionInPPM = {}; /* PPM = Pixels per Metre */
    std::int32_t VerticalResolutionInPPM = {};
    std::uint32_t ColourPaletteColourCount = {};
    std::uint32_t ImportantColourCount = {};
};
#pragma pack()

static inline std::uint16_t const kBMPFileID = 0x4D42;

static bool const ReadBMPData(std::ifstream & BMPFile, BMPLoader::BMPImageData & OutputImageData)
{
    /* This will be very basic at the moment, I can add features as and when I need them */

    bool bResult = false;

    BMPHeader FileHeader = {};
    BMPFile.read(reinterpret_cast<char *>(&FileHeader), sizeof(FileHeader));

    if (FileHeader.ID == kBMPFileID)
    {
        BMPLoader::BMPImageData ImageData = {};

        std::uint32_t DIBHeaderSizeInBytes = {};
        BMPFile.read(reinterpret_cast<char *>(&DIBHeaderSizeInBytes), sizeof(DIBHeaderSizeInBytes));

        if (DIBHeaderSizeInBytes == 40u)
        {
            BitmapInfoHeader InfoHeader = {};
            BMPFile.read(reinterpret_cast<char *>(&InfoHeader), sizeof(InfoHeader));

            std::uint32_t const RowStrideInBytes = (InfoHeader.BitsPerPixel * InfoHeader.WidthInPixels) >> 3u;
            std::uint32_t const AlignedStrideInBytes = (RowStrideInBytes + 3u) & ~3u; // Rows are aligned to 4 bytes
            std::uint32_t const DataSizeInBytes = RowStrideInBytes * InfoHeader.HeightInPixels;

            ImageData.WidthInPixels = InfoHeader.WidthInPixels;
            ImageData.HeightInPixels = InfoHeader.HeightInPixels;
            ImageData.DataSizeInBytes = 4u * InfoHeader.WidthInPixels * InfoHeader.HeightInPixels;
            ImageData.RawData = new std::byte [ImageData.DataSizeInBytes];

            BMPFile.seekg(FileHeader.DataStartOffset, std::ifstream::beg);

            std::byte * const IntermediateImageData = new std::byte [DataSizeInBytes];

            std::uint32_t DataOutputOffset = {};

            for (uint32_t CurrentRowIndex = {};
                 CurrentRowIndex < InfoHeader.HeightInPixels;
                 CurrentRowIndex++)
            {
                BMPFile.read(reinterpret_cast<char *>(IntermediateImageData + DataOutputOffset), RowStrideInBytes);

                /* I want to skip any padding, skipping a couple of bytes shouldn't be too bad they should be buffered */
                BMPFile.seekg(AlignedStrideInBytes - RowStrideInBytes, std::ifstream::cur);

                DataOutputOffset += RowStrideInBytes;
            }

            std::byte const * const ImageDataEndAddress = { IntermediateImageData + DataSizeInBytes };

            std::uint32_t ImageOutputOffset = {};

            for (std::byte const * CurrentImageOffset = { IntermediateImageData };
                 CurrentImageOffset < ImageDataEndAddress;
                 CurrentImageOffset += 6u)
            {
                ::memcpy_s(&ImageData.RawData [ImageOutputOffset], 4u, CurrentImageOffset, 3u);
                ::memcpy_s(&ImageData.RawData [ImageOutputOffset + 4u], 4u, CurrentImageOffset + 3u, 3u);

                ImageOutputOffset += 8u;
            }

            OutputImageData = std::move(ImageData);

            bResult = true;
        }
    }

    return bResult;
}

bool const BMPLoader::LoadFile(std::filesystem::path const & FilePath, BMPLoader::BMPImageData & OutputImageData)
{
    bool bResult = false;

    if (std::filesystem::exists(FilePath)
        && FilePath.has_extension()
        && FilePath.extension() == ".bmp")
    {
        std::ifstream BMPFile = std::ifstream(FilePath, std::ifstream::binary);

        BMPLoader::BMPImageData ImageData = {};
        bResult = ::ReadBMPData(BMPFile, ImageData);

        OutputImageData = std::move(ImageData);

        BMPFile.close();
    }

    return bResult;
}