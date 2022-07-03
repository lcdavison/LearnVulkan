#include "AssetManager.hpp"

#include "Common.hpp"

#include "Platform/Windows.hpp"

#include <OBJLoader/OBJLoader.hpp>

#include <vector>
#include <queue>
#include <unordered_map>
#include <filesystem>
#include <thread>
#include <future>

struct Assets
{
    std::unordered_map<std::string, uint32> AssetLookUpTable;

    std::unordered_map<uint32, std::future<bool>> PendingMeshAssets = {};

    std::vector<AssetManager::MeshAsset> MeshAssets;

    std::queue<uint32> FreeMeshIndices = {};

    std::vector<bool> ReadyMeshFlags = {};
};

namespace AssetLoaderThread
{
    using AssetLoadTask = std::packaged_task<bool()>;

    static std::wstring const kThreadName = L"Asset Loader Thread";

    static std::thread Thread;

    static std::mutex WorkQueueMutex = {};
    static std::condition_variable WorkCondition = {};

    static std::queue<AssetLoadTask> WorkQueue = {};

    static std::atomic_bool bStopThread = { false };

    static void Main()
    {
        while (!bStopThread)
        {
            AssetLoadTask LoadTask = {};

            {
                std::unique_lock Lock = std::unique_lock(WorkQueueMutex);

                WorkCondition.wait(Lock,
                                   []()
                                   {
                                       return WorkQueue.size() > 0u || bStopThread;
                                   });

                if (WorkQueue.size() > 0u)
                {
                    LoadTask = std::move(WorkQueue.front());
                    WorkQueue.pop();
                }
            }

            if (LoadTask.valid())
            {
                LoadTask();
            }
        }
    }

    static void Start()
    {
        Thread = std::thread(Main);

        Platform::Windows::SetThreadDescription(Thread.native_handle(), kThreadName.c_str());
    }

    static void Stop()
    {
        bStopThread = true;
        WorkCondition.notify_one();

        Thread.join();
    }
}

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
    std::unordered_map<std::string, uint32> VertexLookUpTable = {};

    AssetManager::MeshAsset IntermediateAsset = {};

    for (uint32 const CurrentFaceOffset : OBJMeshData.FaceOffsets)
    {
        /* Currently this just uses whatever winding order is used in the OBJ file */
        /* We could process the vertices further afterwards to set them to either CW or CCW */
        for (uint8 CurrentFaceVertexIndex = { 0u };
             CurrentFaceVertexIndex < 3u;
             CurrentFaceVertexIndex++)
        {
            uint32 const VertexIndex = OBJMeshData.FaceVertexIndices [CurrentFaceOffset + CurrentFaceVertexIndex];

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

            std::string const IndexString = std::to_string(VertexIndex) + std::to_string(NormalIndex) + std::to_string(TextureCoordinateIndex);

            auto FoundVertex = VertexLookUpTable.find(IndexString);

            if (FoundVertex != VertexLookUpTable.end())
            {
                Index = FoundVertex->second;
            }
            else
            {
                Index = static_cast<uint32>(IntermediateAsset.Vertices.size());

                OBJLoader::OBJVertex const & VertexData = OBJMeshData.Positions [VertexIndex - 1u];
                IntermediateAsset.Vertices.emplace_back(Math::Vector3 { VertexData.X, VertexData.Y, VertexData.Z });

                OBJLoader::OBJNormal const & NormalData = OBJMeshData.Normals [NormalIndex - 1u];
                IntermediateAsset.Normals.emplace_back(Math::Vector3 { NormalData.X, NormalData.Y, NormalData.Z });

                VertexLookUpTable [IndexString] = Index;
            }

            IntermediateAsset.Indices.push_back(Index);
        }
    }

    IntermediateAsset.Vertices.shrink_to_fit();
    IntermediateAsset.Normals.shrink_to_fit();
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
    AssetLoaderThread::Start();
}

void AssetManager::Destroy()
{
    AssetLoaderThread::Stop();
}

bool const AssetManager::LoadMeshAsset(std::string const & AssetName, AssetManager::AssetHandle<AssetManager::MeshAsset> & OutputMeshHandle)
{
    bool bResult = false;

    auto FoundAsset = AssetTable.AssetLookUpTable.find(AssetName);

    if (FoundAsset != AssetTable.AssetLookUpTable.end())
    {
        OutputMeshHandle.AssetIndex = FoundAsset->second;
        bResult = true;
    }
    else
    {
        std::filesystem::path AssetFilePath = {};
        bool bFoundAssetFile = ::FindAssetFileRecursive(AssetName, kOBJFileExtension, kAssetDirectoryPath, AssetFilePath);

        if (bFoundAssetFile)
        {
            uint32 NewMeshHandle = {};
            uint32 NewMeshIndex = {};

            if (AssetTable.FreeMeshIndices.size() > 0u)
            {
                NewMeshHandle = AssetTable.FreeMeshIndices.front();
                AssetTable.FreeMeshIndices.pop();
            }
            else
            {
                AssetTable.MeshAssets.emplace_back();
                AssetTable.ReadyMeshFlags.emplace_back();

                NewMeshHandle = static_cast<uint32>(AssetTable.MeshAssets.size());
            }

            NewMeshIndex = NewMeshHandle - 1u;

            AssetTable.ReadyMeshFlags [NewMeshIndex] = false;

            AssetLoaderThread::AssetLoadTask LoadTask = AssetLoaderThread::AssetLoadTask(
                [FilePath = std::move(AssetFilePath), NewMeshIndex]()
                {
                    OBJLoader::OBJMeshData LoadedMeshData = {};
                    bool bResult = OBJLoader::LoadFile(FilePath, LoadedMeshData);

                    if (bResult)
                    {
                        AssetManager::MeshAsset NewMesh = {};
                        ::ProcessOBJMeshData(LoadedMeshData, NewMesh);

                        AssetTable.MeshAssets [NewMeshIndex] = std::move(NewMesh);
                    }

                    return bResult;
                }
            );

            AssetTable.PendingMeshAssets [NewMeshIndex] = LoadTask.get_future();

            {
                std::scoped_lock Lock = std::scoped_lock(AssetLoaderThread::WorkQueueMutex);

                AssetLoaderThread::WorkQueue.push(std::move(LoadTask));
            }

            AssetLoaderThread::WorkCondition.notify_one();

            /* Remove this from table if loading fails */
            AssetTable.AssetLookUpTable [AssetName] = NewMeshHandle;
            OutputMeshHandle.AssetIndex = NewMeshHandle;
            bResult = true;
        }
    }

    return bResult;
}

bool const AssetManager::GetMeshData(AssetManager::AssetHandle<AssetManager::MeshAsset> const MeshHandle, AssetManager::MeshAsset const *& OutputMeshData)
{
    bool bResult = true;

    uint32 const AssetIndex = MeshHandle.AssetIndex - 1u;

    if (!AssetTable.ReadyMeshFlags [AssetIndex])
    {
        /* TODO: Check that a future exists for the mesh first */

        bResult = AssetTable.PendingMeshAssets [AssetIndex].get();

        AssetTable.ReadyMeshFlags [AssetIndex] = bResult;

        AssetTable.PendingMeshAssets.erase(AssetIndex);

        if (bResult)
        {
            OutputMeshData = &AssetTable.MeshAssets [AssetIndex];
        }
    }

    OutputMeshData = &AssetTable.MeshAssets [AssetIndex];

    return bResult;
}