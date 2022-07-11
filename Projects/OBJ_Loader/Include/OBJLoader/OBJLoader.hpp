#pragma once

#include <vector>
#include <array>
#include <filesystem>

#if defined(OBJ_LOADER_EXPORT)
#define OBJ_LOADER_API __declspec(dllexport)
#else
#define OBJ_LOADER_API __declspec(dllimport)
#endif

namespace OBJLoader
{
    struct OBJVertex
    {
        float X;
        float Y;
        float Z;
        float W;
    };

    struct OBJNormal
    {
        float X;
        float Y;
        float Z;
    };

    struct OBJTextureCoordinate
    {
        float U;
        float V;
        float W;
    };

    struct OBJMeshData
    {
        std::vector<OBJVertex> Positions = {};
        std::vector<OBJNormal> Normals = {};
        std::vector<OBJTextureCoordinate> TextureCoordinates = {};

        std::vector<std::uint32_t> FaceOffsets = {};
        std::vector<std::uint32_t> FaceVertexIndices = {};
        std::vector<std::uint32_t> FaceNormalIndices = {};
        std::vector<std::uint32_t> FaceTextureCoordinateIndices = {};
    };

    /* Indices into OBJMaterialData::TexturePaths */
    enum TexturePaths
    {
        AmbientMap,
        DiffuseMap,
        SpecularMap,
        PathCount,
    };

    struct OBJMaterialData
    {
        std::string MaterialName = {};
        std::array<std::string, TexturePaths::PathCount> TexturePaths = {};
        std::array<float, 3u> AmbientColour = {};
        std::array<float, 3u> DiffuseColour = {};
        std::array<float, 3u> SpecularColour = {};
        std::array<float, 3u> TransmissionFilter = {};
        float SpecularExponent = {};
        float Transparency = {};
        float IndexOfRefraction = {};
    };

    OBJ_LOADER_API bool const LoadFile(std::filesystem::path const & OBJFilePath, OBJMeshData & OutputMeshData, std::vector<OBJMaterialData> & OutputMaterials);
}