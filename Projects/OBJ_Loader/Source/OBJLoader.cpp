#include "OBJLoader/OBJLoader.hpp"

#include <fstream>
#include <string>
#include <array>
#include <unordered_map>

static inline std::string const kOBJFileExtension = ".obj";
static inline std::string const kMTLFileExtension = ".mtl";

static inline std::uint8_t const kOBJMinVertexComponentCount = { 3u };
static inline std::uint8_t const kOBJMinTextureCoordinateComponentCount = { 2u };

static inline char const kOBJCommentCharacter = '#';
static inline char const kOBJAttributeCharacter = 'v';
static inline char const kOBJTextureCoordinateCharacter = 't';
static inline char const kOBJNormalCharacter = 'n';
static inline char const kOBJFaceCharacter = 'f';
static inline char const kOBJFaceDelimitingCharacter = '/';

static inline char const kWhitespaceCharacter = ' ';

static bool const StartsWithString(std::string_view const String, std::string const StringPattern)
{
    if (StringPattern.size() > String.size())
    {
        return false;
    }

    std::uint16_t MatchedCharacterCount = {};

    for (std::uint16_t CurrentCharacterIndex = {};
         String [CurrentCharacterIndex] == StringPattern [CurrentCharacterIndex]
         && CurrentCharacterIndex < StringPattern.size();
         CurrentCharacterIndex++)
    {
        MatchedCharacterCount++;
    }

    return MatchedCharacterCount == StringPattern.size();
}

inline void SkipWhitespace(std::string const & FileLine, std::uint16_t CurrentIndex, std::uint16_t & OutputIndex)
{
    while (CurrentIndex < FileLine.size() && (FileLine [CurrentIndex] == ' ' || FileLine [CurrentIndex] == '\t'))
    {
        CurrentIndex++;
    }

    OutputIndex = CurrentIndex;
}

static void ExtractProperties(std::ifstream & FileStream, std::vector<std::string> & OutputPropertyIDs, std::vector<std::string> & OutputPropertyValues, std::vector<std::uint32_t> & OutputPropertyValueOffsets, std::vector<std::uint8_t> & OutputPropertyValueCounts)
{
    std::vector<std::string> PropertyIDs = {};
    std::vector<std::string> PropertyValues = {};
    std::vector<std::uint8_t> PropertyValueCounts = {};
    std::vector<std::uint32_t> PropertyValueOffsets = {};

    std::string ValueString = {};
    ValueString.reserve(256u);

    std::string CurrentFileLine = {};
    while (std::getline(FileStream, CurrentFileLine))
    {
        std::uint16_t CurrentCharacterIndex = {};
        ::SkipWhitespace(CurrentFileLine, CurrentCharacterIndex, CurrentCharacterIndex);

        if (CurrentFileLine.length() == 0u || CurrentFileLine [CurrentCharacterIndex] == kOBJCommentCharacter)
        {
            continue;
        }

        while (CurrentCharacterIndex < CurrentFileLine.length() && CurrentFileLine [CurrentCharacterIndex] != kWhitespaceCharacter)
        {
            ValueString += CurrentFileLine [CurrentCharacterIndex];
            CurrentCharacterIndex++;
        }

        PropertyIDs.emplace_back(std::move(ValueString));

        std::uint32_t const PreviousParameterCount = PropertyValues.size();

        while (CurrentCharacterIndex < CurrentFileLine.size())
        {
            ::SkipWhitespace(CurrentFileLine, CurrentCharacterIndex, CurrentCharacterIndex);

            while (CurrentCharacterIndex < CurrentFileLine.length() && CurrentFileLine [CurrentCharacterIndex] != kWhitespaceCharacter)
            {
                ValueString += CurrentFileLine [CurrentCharacterIndex];
                CurrentCharacterIndex++;
            }

            if (ValueString.length() > 0u)
            {
                PropertyValues.emplace_back(std::move(ValueString));
            }
        }

        PropertyValueCounts.push_back(PropertyValues.size() - PreviousParameterCount);
        PropertyValueOffsets.push_back(PreviousParameterCount);
    }

    OutputPropertyIDs = std::move(PropertyIDs);
    OutputPropertyValues = std::move(PropertyValues);
    OutputPropertyValueOffsets = std::move(PropertyValueOffsets);
    OutputPropertyValueCounts = std::move(PropertyValueCounts);
}

static bool const ParseOBJFile(std::ifstream & OBJFileStream, OBJLoader::OBJMeshData & OutputMeshData, std::vector<std::filesystem::path> & OutputMaterialFilePaths)
{
    OBJLoader::OBJMeshData MeshData = {};

    std::vector<std::string> PropertyIDs = {};
    std::vector<std::string> PropertyValues = {};
    std::vector<std::uint8_t> PropertyValueCounts = {};
    std::vector<std::uint32_t> PropertyValueOffsets = {};

    ::ExtractProperties(OBJFileStream, PropertyIDs, PropertyValues, PropertyValueOffsets, PropertyValueCounts);

    for (std::uint32_t CurrentPropertyIndex = {};
         CurrentPropertyIndex < PropertyIDs.size();
         CurrentPropertyIndex++)
    {
        std::string const & PropertyID = PropertyIDs [CurrentPropertyIndex];

        switch (PropertyID [0u])
        {
            case kOBJAttributeCharacter:
            {
                switch (PropertyID [1u])
                {
                    case kOBJNormalCharacter:
                    {
                        MeshData.Normals.emplace_back(OBJLoader::OBJNormal
                                                        {
                                                            std::stof(PropertyValues [PropertyValueOffsets [CurrentPropertyIndex] + 0u]),
                                                            std::stof(PropertyValues [PropertyValueOffsets [CurrentPropertyIndex] + 1u]),
                                                            std::stof(PropertyValues [PropertyValueOffsets [CurrentPropertyIndex] + 2u]),
                                                        });
                    }
                    break;
                    case kOBJTextureCoordinateCharacter:
                    {
                        std::uint8_t const ValueCount = PropertyValueCounts [CurrentPropertyIndex];

                        MeshData.TextureCoordinates.emplace_back(OBJLoader::OBJTextureCoordinate
                                                                    {
                                                                        std::stof(PropertyValues [PropertyValueOffsets [CurrentPropertyIndex] + 0u]),
                                                                        std::stof(PropertyValues [PropertyValueOffsets [CurrentPropertyIndex] + 1u]),
                                                                        ValueCount > kOBJMinTextureCoordinateComponentCount ? std::stof(PropertyValues [PropertyValueOffsets [CurrentPropertyIndex] + 1u]) : 0.0f,
                                                                    });
                    }
                    break;
                    default:
                    {
                        std::uint8_t const ValueCount = PropertyValueCounts [CurrentPropertyIndex];

                        MeshData.Positions.emplace_back(OBJLoader::OBJVertex
                                                        {
                                                            std::stof(PropertyValues [PropertyValueOffsets [CurrentPropertyIndex] + 0u]),
                                                            std::stof(PropertyValues [PropertyValueOffsets [CurrentPropertyIndex] + 1u]),
                                                            std::stof(PropertyValues [PropertyValueOffsets [CurrentPropertyIndex] + 2u]),
                                                            ValueCount > kOBJMinVertexComponentCount ? std::stof(PropertyValues [PropertyValueOffsets [CurrentPropertyIndex] + 3u]) : 1.0f,
                                                        });
                    }
                    break;
                }
            }
            continue;
            case kOBJFaceCharacter:
            {
                std::array<std::uint32_t, 3u> Indices = {};

                MeshData.FaceOffsets.push_back(MeshData.FaceVertexIndices.size());

                std::string IndexString = {};
                
                for (std::uint8_t CurrentVertexIndex = {};
                     CurrentVertexIndex < PropertyValueCounts [CurrentPropertyIndex];
                     CurrentVertexIndex++)
                {
                    std::string const & FaceString = PropertyValues [PropertyValueOffsets [CurrentPropertyIndex] + CurrentVertexIndex];

                    std::uint8_t OutputIndex = {};

                    std::uint16_t CurrentCharacterIndex = {};
                    while (CurrentCharacterIndex < FaceString.size())
                    {
                        IndexString.clear();

                        while (CurrentCharacterIndex < FaceString.size() && FaceString [CurrentCharacterIndex] != kOBJFaceDelimitingCharacter)
                        {
                            IndexString += FaceString [CurrentCharacterIndex];
                            CurrentCharacterIndex++;
                        }

                        Indices [OutputIndex] = std::stoul(IndexString);
                        OutputIndex++;
                        CurrentCharacterIndex++;
                    }

                    MeshData.FaceVertexIndices.push_back(Indices [0u]);
                    MeshData.FaceTextureCoordinateIndices.push_back(Indices [1u]);
                    MeshData.FaceNormalIndices.push_back(Indices [2u]);
                }
            }
            continue;
            default:
                break;
        }

        if (PropertyID == "mtllib")
        {
            OutputMaterialFilePaths.push_back(PropertyValues [CurrentPropertyIndex]);
        }
    }

    bool const bSuccess = MeshData.Positions.size() > 0u;

    if (bSuccess)
    {
        OutputMeshData = std::move(MeshData);
    }

    return bSuccess;
}

static bool const ParseMTLFile(std::ifstream & MTLFileStream, std::filesystem::path const & ParentDirectory, OBJLoader::OBJMaterialData & OutputMaterialData)
{
    OBJLoader::OBJMaterialData MaterialData = {};

    std::vector<std::string> PropertyIDs = {};
    std::vector<std::string> PropertyValues = {};
    std::vector<std::uint8_t> PropertyValueCounts = {};
    std::vector<std::uint32_t> PropertyValueOffsets = {};

    ::ExtractProperties(MTLFileStream, PropertyIDs, PropertyValues, PropertyValueOffsets, PropertyValueCounts);

    for (std::uint32_t CurrentPropertyIndex = {};
         CurrentPropertyIndex < PropertyIDs.size();
         CurrentPropertyIndex++)
    {
        std::string const & PropertyName = PropertyIDs [CurrentPropertyIndex];

        switch (PropertyName [0u])
        {
            case 'K':
            {
                std::array<float, 3u> Values =
                {
                    std::stof(PropertyValues [PropertyValueOffsets [CurrentPropertyIndex] + 0u]),
                    std::stof(PropertyValues [PropertyValueOffsets [CurrentPropertyIndex] + 1u]),
                    std::stof(PropertyValues [PropertyValueOffsets [CurrentPropertyIndex] + 2u]),
                };

                switch (PropertyName [1u])
                {
                    /* Ambient Reflectivity */
                    case 'a':
                    {
                        MaterialData.AmbientColour = Values;
                    }
                    break;
                    /* Diffuse Reflectivity */
                    case 'd':
                    {
                        MaterialData.DiffuseColour = Values;
                    }
                    break;
                    /* Specular Reflectivity */
                    case 's':
                    {
                        MaterialData.SpecularColour = Values;
                    }
                    break;
                }
            }
            continue;
            case 'T':
            {
                switch (PropertyName [1u])
                {
                    /* Transparency */
                    case 'r':
                    {
                        MaterialData.Transparency = std::stof(PropertyValues [PropertyValueOffsets [CurrentPropertyIndex]]);
                    }
                    break;
                    /* Transmission Filter */
                    case 'f':
                    {
                        MaterialData.TransmissionFilter =
                        {
                            std::stof(PropertyValues [PropertyValueOffsets [CurrentPropertyIndex] + 0u]),
                            std::stof(PropertyValues [PropertyValueOffsets [CurrentPropertyIndex] + 1u]),
                            std::stof(PropertyValues [PropertyValueOffsets [CurrentPropertyIndex] + 2u]),
                        };
                    }
                    break;
                }
            }
            continue;
            case 'N':
            {
                switch (PropertyName [1u])
                {
                    /* Specular Exponent */
                    case 's':
                    {
                        MaterialData.SpecularExponent = std::stof(PropertyValues [PropertyValueOffsets [CurrentPropertyIndex]]);
                    }
                    break;
                    /* Optical Density (IoR) */
                    case 'i':
                    {
                        MaterialData.IndexOfRefraction = std::stof(PropertyValues [PropertyValueOffsets [CurrentPropertyIndex]]);
                    }
                    break;
                }
            }
            continue;
        }

        /* This is very basic at the moment and will fail when texture properties are used */
        std::string TextureFilePath = {};

        for (std::uint8_t CurrentChunkIndex = {};
             CurrentChunkIndex < PropertyValueCounts [CurrentPropertyIndex];
             CurrentChunkIndex++)
        {
            TextureFilePath += PropertyValues [PropertyValueOffsets [CurrentPropertyIndex] + CurrentChunkIndex];
        }

        std::unordered_map<std::string, OBJLoader::TexturePaths> const TexturePathLUT =
        {
            std::make_pair("map_Ka", OBJLoader::TexturePaths::AmbientMap),
            std::make_pair("map_Kd", OBJLoader::TexturePaths::DiffuseMap),
            std::make_pair("map_Ks", OBJLoader::TexturePaths::SpecularMap),
        };

        if (::StartsWithString(PropertyName, "map_"))
        {
            auto FoundIndex = TexturePathLUT.find(PropertyName);

            if (FoundIndex == TexturePathLUT.cend())
            {
                continue;
            }

            OBJLoader::TexturePaths const TexturePathIndex = FoundIndex->second;
            MaterialData.TexturePaths [TexturePathIndex] = std::move(TextureFilePath);
        }
        else if (PropertyName == "newmtl")
        {
            MaterialData.MaterialName = PropertyValues [PropertyValueOffsets [CurrentPropertyIndex]];
        }
        else if (PropertyName == "bump")
        {
            /* TODO: Add bump map path */
        }
    }

    bool const bSuccess = MaterialData.MaterialName != "";

    if (bSuccess)
    {
        OutputMaterialData = std::move(MaterialData);
    }

    return bSuccess;
}

bool const OBJLoader::LoadFile(std::filesystem::path const & OBJFilePath, OBJLoader::OBJMeshData & OutputMeshData, std::vector<OBJMaterialData> & OutputMaterials)
{
    bool bResult = false;

    if (std::filesystem::exists(OBJFilePath)
        && OBJFilePath.has_extension()
        && OBJFilePath.extension() == kOBJFileExtension)
    {
        std::ifstream FileStream = std::ifstream(OBJFilePath);

        OBJLoader::OBJMeshData IntermediateMeshData = {};
        std::vector<std::filesystem::path> MaterialFilePaths = {};

        bResult = ::ParseOBJFile(FileStream, IntermediateMeshData, MaterialFilePaths);

        if (bResult)
        {
            OutputMeshData = std::move(IntermediateMeshData);
        }

        FileStream.close();

        /* Now we parse any material data */
        std::filesystem::path const OBJDirectory = OBJFilePath.parent_path();

        std::vector<OBJLoader::OBJMaterialData> Materials = {};

        for (std::filesystem::path const & MaterialFilePath : MaterialFilePaths)
        {
            std::filesystem::path const AbsoluteFilePath = OBJDirectory / MaterialFilePath;

            if (std::filesystem::exists(AbsoluteFilePath))
            {
                std::ifstream MTLFileStream = std::ifstream(AbsoluteFilePath);

                OBJLoader::OBJMaterialData Material = {};
                bool bParsedMTLFile = ::ParseMTLFile(MTLFileStream, OBJDirectory, Material);

                if (bParsedMTLFile)
                {
                    Materials.emplace_back(std::move(Material));
                }

                MTLFileStream.close();
            }
        }

        OutputMaterials = std::move(Materials);
    }

    return bResult;
}