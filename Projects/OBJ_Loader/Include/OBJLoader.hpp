#pragma once

#include <vector>
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
        std::vector<OBJVertex> Positions;
        std::vector<OBJNormal> Normals;
        std::vector<OBJTextureCoordinate> TextureCoordinates;

        std::vector<std::uint32_t> FaceOffsets;
        std::vector<std::uint32_t> FaceVertexIndices;
        std::vector<std::uint32_t> FaceNormalIndices;
        std::vector<std::uint32_t> FaceTextureCoordinateIndices;
    };

    OBJ_LOADER_API bool const LoadFile(std::filesystem::path const & OBJFilePath, OBJMeshData & OutputMeshData);
}