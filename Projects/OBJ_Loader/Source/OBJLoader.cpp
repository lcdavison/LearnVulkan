#include "OBJLoader.hpp"

#include <fstream>
#include <string>
#include <array>

static inline std::string const kOBJFileExtension = ".obj";

static inline std::uint8_t const kOBJMaxVertexComponentCount = { 4u };
static inline std::uint8_t const kOBJMaxTextureCoordinateComponentCount = { 3u };

static inline char const kOBJAttributeCharacter = 'v';
static inline char const kOBJTextureCoordinateCharacter = 't';
static inline char const kOBJNormalCharacter = 'n';
static inline char const kOBJFaceCharacter = 'f';
static inline char const kOBJFaceDelimitingCharacter = '/';
static inline char const kOBJNullIndexCharacter = '0';

static inline char const kWhitespaceCharacter = ' ';

inline static bool const IsCharacterANumber(char const Character)
{
    return Character >= '0' && Character <= '9';
}

static void ProcessVertexAttribute(std::string const & AttributeLine, std::vector<float> & OutputAttributeData)
{
    std::uint16_t CurrentCharacterIndex = { 0u };

    std::string CurrentNumber = {};
    CurrentNumber.reserve(256u);

    while (CurrentCharacterIndex < AttributeLine.size())
    {
        char const CurrentCharacter = AttributeLine [CurrentCharacterIndex];

        if (::IsCharacterANumber(CurrentCharacter))
        {
            while (CurrentCharacterIndex < AttributeLine.size() && AttributeLine [CurrentCharacterIndex] != kWhitespaceCharacter)
            {
                CurrentNumber += AttributeLine [CurrentCharacterIndex];
                CurrentCharacterIndex++;
            }

            OutputAttributeData.push_back(std::stof(CurrentNumber));
            CurrentNumber.clear();
        }
        else
        {
            CurrentCharacterIndex++;
        }
    }
}

static void ProcessFaceData(std::string const & FaceLine, std::vector<std::uint32_t> & VertexIndices, std::vector<std::uint32_t> & TextureCoordinateIndices, std::vector<std::uint32_t> & NormalIndices)
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

    /* Indices start at 1 so we can use 0 to indicate no index */
    std::array<std::string, 3u> IndexStrings =
    {
        std::string(1u, kOBJNullIndexCharacter),
        std::string(1u, kOBJNullIndexCharacter),
        std::string(1u, kOBJNullIndexCharacter),
    };

    IndexStrings [0u].reserve(8u);
    IndexStrings [1u].reserve(8u);
    IndexStrings [2u].reserve(8u);

    while (CurrentCharacterIndex < FaceLine.size())
    {
        char const CurrentCharacter = FaceLine [CurrentCharacterIndex];

        if (::IsCharacterANumber(CurrentCharacter))
        {
            std::uint8_t CurrentStringIndex = { 0u };
            while (CurrentCharacterIndex < FaceLine.size() && FaceLine [CurrentCharacterIndex] != kWhitespaceCharacter)
            {
                if (FaceLine [CurrentCharacterIndex] == kOBJFaceDelimitingCharacter)
                {
                    CurrentStringIndex++;
                }
                else
                {
                    IndexStrings [CurrentStringIndex] += FaceLine [CurrentCharacterIndex];
                }

                CurrentCharacterIndex++;
            }

            VertexIndices.push_back(std::stoul(IndexStrings [0u]));
            TextureCoordinateIndices.push_back(std::stoul(IndexStrings [1u]));
            NormalIndices.push_back(std::stoul(IndexStrings [2u]));

            IndexStrings [0u] = kOBJNullIndexCharacter;
            IndexStrings [1u] = kOBJNullIndexCharacter;
            IndexStrings [2u] = kOBJNullIndexCharacter;
        }
        else
        {
            CurrentCharacterIndex++;
        }
    }
}

static bool const ParseOBJFile(std::ifstream & OBJFileStream, OBJLoader::OBJMeshData & OutputMeshData)
{
    std::string CurrentFileLine = {};

    std::vector Vertices = std::vector<float>();
    std::vector TextureCoordinates = std::vector<float>();
    std::vector Normals = std::vector<float>();

    std::vector FaceOffsets = std::vector<std::uint32_t>();

    std::vector VertexIndices = std::vector<std::uint32_t>();
    std::vector TextureCoordinateIndices = std::vector<std::uint32_t>();
    std::vector NormalIndices = std::vector<std::uint32_t>();

    while (std::getline(OBJFileStream, CurrentFileLine))
    {
        char const & LineStartCharacter = CurrentFileLine [0u];

        switch (LineStartCharacter)
        {
            case kOBJAttributeCharacter:
            {
                /* Vertex, Normal etc... */
                char const & AttributeCharacter = CurrentFileLine [1u];

                switch (AttributeCharacter)
                {
                    case kWhitespaceCharacter:
                    {
                        std::uint32_t const CurrentPositionSize = Vertices.size();
                        ::ProcessVertexAttribute(CurrentFileLine, Vertices);

                        /* This should only be needed if 3 values are given instead of 4 */
                        std::uint32_t const PaddingValueCount = kOBJMaxVertexComponentCount - (Vertices.size() - CurrentPositionSize);
                        Vertices.insert(Vertices.cend(), PaddingValueCount, 1.0f);
                    }
                    break;
                    case kOBJNormalCharacter:
                    {
                        ::ProcessVertexAttribute(CurrentFileLine, Normals);
                    }
                    break;
                    case kOBJTextureCoordinateCharacter:
                    {
                        std::uint32_t const CurrentTextureCoordinatesSize = TextureCoordinates.size();
                        ::ProcessVertexAttribute(CurrentFileLine, TextureCoordinates);

                        std::uint32_t const PaddingValueCount = kOBJMaxTextureCoordinateComponentCount - (TextureCoordinates.size() - CurrentTextureCoordinatesSize);
                        TextureCoordinates.insert(TextureCoordinates.cend(), PaddingValueCount, 0.0f);
                    }
                    break;
                    default:
                        break;
                }
            }
            break;
            case kOBJFaceCharacter:
            {
                std::uint32_t const CurrentFaceOffset = VertexIndices.size();

                ::ProcessFaceData(CurrentFileLine, VertexIndices, TextureCoordinateIndices, NormalIndices);

                FaceOffsets.push_back(CurrentFaceOffset);
            }
            break;
            default:
                /* Just skip other lines */
                break;
        }
    }

    /* We could do the data organisation below concurrently, depending on the amount of data */

    for (std::uint32_t CurrentVertexIndex = { 0u }; CurrentVertexIndex < Vertices.size(); CurrentVertexIndex += 4u)
    {
        OBJLoader::OBJVertex & CurrentVertex = OutputMeshData.Positions.emplace_back();

        CurrentVertex.X = Vertices [CurrentVertexIndex + 0u];
        CurrentVertex.Y = Vertices [CurrentVertexIndex + 1u];
        CurrentVertex.Z = Vertices [CurrentVertexIndex + 2u];
        CurrentVertex.W = Vertices [CurrentVertexIndex + 3u];
    }

    for (std::uint32_t CurrentTextureCoordinateIndex = { 0u }; CurrentTextureCoordinateIndex < TextureCoordinates.size(); CurrentTextureCoordinateIndex += 3u)
    {
        OBJLoader::OBJTextureCoordinate & CurrentTextureCoordinate = OutputMeshData.TextureCoordinates.emplace_back();

        CurrentTextureCoordinate.U = TextureCoordinates [CurrentTextureCoordinateIndex + 0u];
        CurrentTextureCoordinate.V = TextureCoordinates [CurrentTextureCoordinateIndex + 1u];
        CurrentTextureCoordinate.W = TextureCoordinates [CurrentTextureCoordinateIndex + 2u];
    }

    for (std::uint32_t CurrentNormalIndex = { 0u }; CurrentNormalIndex < Normals.size(); CurrentNormalIndex += 3u)
    {
        OBJLoader::OBJNormal & CurrentNormal = OutputMeshData.Normals.emplace_back();

        CurrentNormal.X = Normals [CurrentNormalIndex + 0u];
        CurrentNormal.Y = Normals [CurrentNormalIndex + 1u];
        CurrentNormal.Z = Normals [CurrentNormalIndex + 2u];
    }

    OutputMeshData.FaceOffsets = std::move(FaceOffsets);
    OutputMeshData.FaceVertexIndices = std::move(VertexIndices);
    OutputMeshData.FaceTextureCoordinateIndices = std::move(TextureCoordinateIndices);
    OutputMeshData.FaceNormalIndices = std::move(NormalIndices);

    return VertexIndices.size() > 0u || OutputMeshData.FaceOffsets.size() > 0u;
}

bool const OBJLoader::LoadFile(std::filesystem::path const & OBJFilePath, OBJMeshData & OutputMeshData)
{
    bool bResult = false;

    if (std::filesystem::exists(OBJFilePath) &&
        OBJFilePath.has_extension() && OBJFilePath.extension() == kOBJFileExtension)
    {
        std::ifstream FileStream = std::ifstream(OBJFilePath);

        OBJMeshData IntermediateMeshData = {};

        bResult = ::ParseOBJFile(FileStream, IntermediateMeshData);

        if (bResult)
        {
            OutputMeshData = std::move(IntermediateMeshData);
        }

        FileStream.close();
    }

    return bResult;
}