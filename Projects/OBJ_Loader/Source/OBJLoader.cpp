#include "OBJLoader.hpp"

#include <fstream>
#include <string>
#include <array>

static inline std::string const kOBJFileExtension = ".obj";

static inline char const kOBJCommentCharacter = '#';
static inline char const kOBJAttributeCharacter = 'v';
static inline char const kOBJTextureCoordinateCharacter = 't';
static inline char const kOBJNormalCharacter = 'n';
static inline char const kOBJFaceCharacter = 'f';
static inline char const kOBJFaceDelimitingCharacter = '/';

static inline char const kWhitespaceCharacter = ' ';

inline static bool const IsCharacterANumber(char const Character)
{
    return Character >= '0' && Character <= '9';
}

static void ProcessVertexAttribute(std::string const & AttributeLine, std::vector<float> & OutputAttributeData)
{
    std::uint16_t CurrentCharacterIndex = { 0u };

    while (CurrentCharacterIndex < AttributeLine.size())
    {
        char const CurrentCharacter = AttributeLine [CurrentCharacterIndex];

        if (::IsCharacterANumber(CurrentCharacter))
        {
            std::string Number = {};
            Number.reserve(256u);

            while (CurrentCharacterIndex < AttributeLine.size() && AttributeLine [CurrentCharacterIndex] != kWhitespaceCharacter)
            {
                Number += AttributeLine [CurrentCharacterIndex];
                CurrentCharacterIndex++;
            }

            OutputAttributeData.emplace_back(std::atof(Number.c_str()));
        }
        else
        {
            CurrentCharacterIndex++;
        }
    }
}

static void ProcessFaceData(std::string const & FaceLine)
{
    /*
    *   Faces laid out as follows
    *     vi = Vertex Index
    *     ti = Texture Coordinate Index
    *     ni = Normal Index
    *     - f vi vi vi
    *     - f vi/ti vi/ti vi/ti
    *     - f vi/ti/ni vi/ti/ni vi/ti/ni
    *     - f vi//ni vi//ni vi//ni
    */

    std::uint16_t CurrentCharacterIndex = { 0u };

    while (CurrentCharacterIndex < FaceLine.size())
    {
        char const CurrentCharacter = FaceLine [CurrentCharacterIndex];

        if (::IsCharacterANumber(CurrentCharacter))
        {
            std::array<std::string, 3u> Indices = {};

            std::uint8_t CurrentStringIndex = { 0u };
            while (CurrentCharacterIndex < FaceLine.size() && FaceLine [CurrentCharacterIndex] != kWhitespaceCharacter)
            {
                if (FaceLine [CurrentCharacterIndex] == kOBJFaceDelimitingCharacter)
                {
                    CurrentStringIndex++;
                }
                else
                {
                    Indices [CurrentStringIndex] += FaceLine [CurrentCharacterIndex];
                    CurrentCharacterIndex++;
                }
            }
        }
    }
}

static bool const ParseOBJFile(std::ifstream & OBJFileStream)
{
    bool bResult = false;

    std::string CurrentFileLine = {};

    std::vector Positions = std::vector<float>();
    std::vector Normals = std::vector<float>();
    std::vector TextureCoordinates = std::vector<float>();

    while (std::getline(OBJFileStream, CurrentFileLine))
    {
        char const & LineStartCharacter = CurrentFileLine [0u];

        switch (LineStartCharacter)
        {
            case kOBJCommentCharacter:
                break;
            case kOBJAttributeCharacter:
            {
                /* Vertex, Normal etc... */
                char const & AttributeCharacter = CurrentFileLine [1u];

                switch (AttributeCharacter)
                {
                    case kWhitespaceCharacter:
                    {
                        std::uint32_t const CurrentPositionSize = Positions.size();
                        ::ProcessVertexAttribute(CurrentFileLine, Positions);

                        /* This should only be needed if 3 values are given instead of 4 */
                        std::uint32_t const PaddingValueCount = 4u - (Positions.size() - CurrentPositionSize);
                        Positions.insert(Positions.cend(), PaddingValueCount, 1.0f);
                    }
                    break;
                    case kOBJNormalCharacter:
                        ::ProcessVertexAttribute(CurrentFileLine, Normals);
                        break;
                    case kOBJTextureCoordinateCharacter:
                    {
                        std::uint32_t const CurrentTextureCoordinatesSize = TextureCoordinates.size();
                        ::ProcessVertexAttribute(CurrentFileLine, TextureCoordinates);

                        std::uint32_t const PaddingValueCount = 3u - (TextureCoordinates.size() - CurrentTextureCoordinatesSize);
                        TextureCoordinates.insert(TextureCoordinates.cend(), PaddingValueCount, 0.0f);
                    }
                    break;
                }
            }
            break;
            case kOBJFaceCharacter:
                break;
        }
    }

    return bResult;
}

bool const OBJLoader::LoadFile(std::filesystem::path const & OBJFilePath, OBJMeshData & OutputMeshData)
{
    bool bResult = false;

    if (std::filesystem::exists(OBJFilePath) &&
        OBJFilePath.has_extension() && OBJFilePath.extension() == kOBJFileExtension)
    {
        std::ifstream FileStream = std::ifstream(OBJFilePath);

        bResult = ::ParseOBJFile(FileStream);

        FileStream.close();
    }

    return bResult;
}