#include "AssetManager.hpp"

#include "CommonTypes.hpp"

#include <OBJLoader/OBJLoader.hpp>

#include <vector>
#include <unordered_map>
#include <filesystem>

struct Assets
{
    std::unordered_map<std::string, uint32> AssetLookUpTable;

    std::vector<AssetManager::MeshAsset> MeshAssets;
};

static std::filesystem::path const kAssetDirectoryPath = std::filesystem::current_path() / "Assets";

static std::filesystem::path const kOBJFileExtension = ".obj";

static Assets AssetTable;

static void CreateVertexHash(uint32 const VertexIndex, uint32 const NormalIndex, uint32 const TextureCoordinateIndex, uint32 & OutputHash)
{
    uint32 Result = 0u;

    uint32 HighBits = Result & 0xF8000000;
    Result <<= 5u;
    Result ^= HighBits >> 27u;
    Result ^= VertexIndex;

    HighBits = Result & 0xF8000000;
    Result <<= 5u;
    Result ^= HighBits >> 27u;
    Result ^= NormalIndex;

    HighBits = Result & 0xF8000000;
    Result <<= 5u;
    Result ^= HighBits >> 27u;
    Result ^= TextureCoordinateIndex;

    OutputHash = Result;
}

static void ProcessOBJMeshData(OBJLoader::OBJMeshData const & OBJMeshData, AssetManager::MeshAsset & OutputMeshAsset)
{
    /* Use this to check if we have already encountered a vertex previously */
    std::unordered_map<uint32, uint32> VertexLookUpTable = {};

    AssetManager::MeshAsset IntermediateAsset = {};

    for (uint32 const CurrentFaceOffset : OBJMeshData.FaceOffsets)
    {
        /* Currently this just uses whatever winding order is used in the OBJ file */
        /* We could process the vertices further afterwards to set them to either CW or CCW */
        for (uint8 CurrentFaceVertexIndex = { 0u };
             CurrentFaceVertexIndex < 3u;
             CurrentFaceVertexIndex++)
        {
            uint32 VertexIndex = OBJMeshData.FaceVertexIndices [CurrentFaceOffset + CurrentFaceVertexIndex];

            uint32 NormalIndex = {};
            if (OBJMeshData.FaceNormalIndices.size() > 0u)
            {
                NormalIndex = OBJMeshData.FaceNormalIndices [CurrentFaceOffset + CurrentFaceVertexIndex];
            }

            uint32 TextureCoordinateIndex = {};
            if (OBJMeshData.FaceTextureCoordinateIndices.size() > 0u)
            {
                TextureCoordinateIndex = OBJMeshData.FaceTextureCoordinateIndices [CurrentFaceOffset + CurrentFaceVertexIndex];
            }

            uint32 VertexHash = {};
            ::CreateVertexHash(VertexIndex, NormalIndex, TextureCoordinateIndex, VertexHash);

            uint32 Index = {};

            auto FoundVertex = VertexLookUpTable.find(VertexHash);

            if (FoundVertex != VertexLookUpTable.end())
            {
                Index = FoundVertex->second;
            }
            else
            {
                OBJLoader::OBJVertex const & VertexData = OBJMeshData.Positions [VertexIndex - 1u];
                IntermediateAsset.Vertices.emplace_back(Math::Vector3 { VertexData.X, VertexData.Y, VertexData.Z });

                Index = static_cast<uint32>(IntermediateAsset.Vertices.size()) - 1u;
                VertexLookUpTable [VertexHash] = Index;
            }

            IntermediateAsset.Indices.push_back(Index);
        }
    }

    IntermediateAsset.Vertices.shrink_to_fit();
    IntermediateAsset.Indices.shrink_to_fit();

    OutputMeshAsset = std::move(IntermediateAsset);
}

static bool const FindAssetFileRecursive(std::string const & AssetName, std::filesystem::path const & FileExtension, std::filesystem::path const & CurrentDirectory, std::filesystem::path & OutputAssetFilePath)
{
    bool bResult = false;

    for (auto CurrentEntry : std::filesystem::directory_iterator(CurrentDirectory))
    {
        if (CurrentEntry.is_directory())
        {
            std::filesystem::path FilePath = {};
            if (::FindAssetFileRecursive(AssetName, FileExtension, CurrentEntry.path(), FilePath))
            {
                OutputAssetFilePath = FilePath;
                bResult = true;
                break;
            }
        }
        else
        {
            std::string const CurrentFileName = CurrentEntry.path().stem().string();
            bool const bFileNameMatches = std::equal(CurrentFileName.begin(), CurrentFileName.end(), AssetName.begin(),
                                                     [](char const FirstCharacter, char const SecondCharacter)
                                                     {
                                                         return std::tolower(FirstCharacter) == std::tolower(SecondCharacter);
                                                     });

            if (bFileNameMatches &&
                CurrentEntry.path().extension() == FileExtension)
            {
                OutputAssetFilePath = CurrentEntry.path();
                bResult = true;
                break;
            }
        }
    }

    return bResult;
}

void AssetManager::Initialise()
{
    /* We may want to do something in here */
}

bool const AssetManager::GetMeshData(std::string const & AssetName, AssetManager::MeshAsset const *& OutputMeshData)
{
    bool bResult = false;

    auto FoundAsset = AssetTable.AssetLookUpTable.find(AssetName);

    if (FoundAsset != AssetTable.AssetLookUpTable.end())
    {
        OutputMeshData = &AssetTable.MeshAssets [FoundAsset->second];
        bResult = true;
    }
    else
    {
        std::filesystem::path AssetFilePath = {};
        bool bFoundAssetFile = ::FindAssetFileRecursive(AssetName, kOBJFileExtension, kAssetDirectoryPath, AssetFilePath);

        if (bFoundAssetFile)
        {
            OBJLoader::OBJMeshData LoadedMeshData = {};
            bResult = OBJLoader::LoadFile(AssetFilePath, LoadedMeshData);

            AssetManager::MeshAsset IntermediateMeshAsset = {};
            ::ProcessOBJMeshData(LoadedMeshData, IntermediateMeshAsset);

            if (bResult)
            {
                AssetManager::MeshAsset const & NewMesh = AssetTable.MeshAssets.emplace_back(std::move(IntermediateMeshAsset));
                uint32 NewMeshIndex = static_cast<uint32>(AssetTable.MeshAssets.size()) - 1u;

                AssetTable.AssetLookUpTable [AssetName] = NewMeshIndex;

                OutputMeshData = &NewMesh;
            }
        }
    }

    return bResult;
}