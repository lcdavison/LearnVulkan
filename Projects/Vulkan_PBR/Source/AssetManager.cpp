#include "AssetManager.hpp"

#include "Common.hpp"

#include "Platform/Windows.hpp"

#include "Logging.hpp"

#include <OBJLoader/OBJLoader.hpp>

#include <vector>
#include <queue>
#include <unordered_map>
#include <filesystem>
#include <thread>
#include <future>

enum class AssetTypes
{
    Mesh = 0x1,
};

struct Assets
{
    static uint32 const kMaximumAssetHandleValue = { 1u << 16u };

    std::unordered_map<std::string, uint32> AssetLookUpTable = {};

    std::unordered_map<uint32, std::future<std::pair<bool, int32>>> PendingAssets = {};

    /* Store as unique_ptrs atm, will need to write an allocator to make memory allocation more efficient */
    /* Storing the raw objects is dangerous since this will change when new assets are loaded and so all pointers/references will be invalidated */
    std::vector<std::unique_ptr<AssetManager::MeshAsset>> MeshAssets = {};
    std::vector<std::string> MeshFileNames = {};

    std::queue<uint32> FreeMeshIndices = {};

    std::vector<bool> MeshLoadedFlags = {};
};

struct AssetMailbox
{
    std::mutex MeshQueueMutex = {};
    std::queue<int32> FreeMeshIndices = {};
    std::vector<std::unique_ptr<AssetManager::MeshAsset>> Meshes = {};
};

namespace AssetLoaderThread
{
    using AssetLoadTask = std::packaged_task<std::pair<bool, int32>()>;

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
static AssetMailbox LoadedAssets;

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

                OBJLoader::OBJTextureCoordinate const & UVData = OBJMeshData.TextureCoordinates [TextureCoordinateIndex - 1u];
                IntermediateAsset.UVs.emplace_back(Math::Vector3 { UVData.U, UVData.V, UVData.W });

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

static std::pair<bool, int32> LoadOBJMeshData(std::filesystem::path const & AssetFilePath)
{
    OBJLoader::OBJMeshData MeshData = {};

    /* TODO: These materials need to be stored somewhere */
    std::vector<OBJLoader::OBJMaterialData> Materials = {};

    bool bLoadedMesh = OBJLoader::LoadFile(AssetFilePath, MeshData, Materials);

    int32 OutputIndex = { -1l };

    if (bLoadedMesh)
    {
        std::unique_ptr Asset = std::make_unique<AssetManager::MeshAsset>();
        ::ProcessOBJMeshData(MeshData, *Asset);

        std::scoped_lock Lock = std::scoped_lock(LoadedAssets.MeshQueueMutex);

        if (LoadedAssets.FreeMeshIndices.size() == 0u)
        {
            OutputIndex = static_cast<int32>(LoadedAssets.Meshes.size());
            LoadedAssets.Meshes.emplace_back(std::move(Asset));
        }
        else
        {
            OutputIndex = { LoadedAssets.FreeMeshIndices.front() };
            LoadedAssets.FreeMeshIndices.pop();

            LoadedAssets.Meshes [OutputIndex] = std::move(Asset);
        }
    }

    return std::make_pair(bLoadedMesh, OutputIndex);
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

bool const AssetManager::LoadMeshAsset(std::filesystem::path const & RelativeFilePath, uint32 & OutputAssetHandle)
{
    bool bResult = false;

    std::filesystem::path const AbsoluteFilePath = kAssetDirectoryPath / RelativeFilePath;

    if (std::filesystem::exists(AbsoluteFilePath))
    {
        std::string const AssetName = AbsoluteFilePath.string();

        auto const FoundAsset = AssetTable.AssetLookUpTable.find(AssetName);

        if (FoundAsset == AssetTable.AssetLookUpTable.cend())
        {
            PBR_ASSERT(AssetTable.MeshAssets.size() + 1u <= Assets::kMaximumAssetHandleValue);

            uint32 NewAssetHandle = {};

            if (AssetTable.FreeMeshIndices.size() == 0u)
            {
                AssetTable.MeshAssets.emplace_back();
                AssetTable.MeshFileNames.emplace_back();
                AssetTable.MeshLoadedFlags.push_back(false);

                NewAssetHandle = (static_cast<uint32>(AssetTypes::Mesh) << 16u) | static_cast<uint16>(AssetTable.MeshAssets.size());
            }
            else
            {
                NewAssetHandle = AssetTable.FreeMeshIndices.front();
                AssetTable.FreeMeshIndices.pop();
            }

            uint16 const NewMeshIndex = { (NewAssetHandle & 0xFFFF) - 1u };

            AssetTable.MeshFileNames [NewMeshIndex] = AbsoluteFilePath.filename().string();

            AssetLoaderThread::AssetLoadTask LoadTask = AssetLoaderThread::AssetLoadTask(
                [AssetFilePath = std::move(AbsoluteFilePath)]()
                {
                    return ::LoadOBJMeshData(AssetFilePath);
                });

            AssetTable.PendingAssets [NewAssetHandle] = LoadTask.get_future();

            {
                std::scoped_lock Lock = std::scoped_lock(AssetLoaderThread::WorkQueueMutex);

                AssetLoaderThread::WorkQueue.emplace(std::move(LoadTask));
            }

            AssetLoaderThread::WorkCondition.notify_one();

            AssetTable.AssetLookUpTable [std::move(AssetName)] = NewAssetHandle;
            OutputAssetHandle = NewAssetHandle;
        }
        else
        {
            OutputAssetHandle = AssetTable.AssetLookUpTable [AssetName];
        }

        bResult = true;
    }
    else
    {
        Logging::Log(Logging::LogTypes::Info, String::Format(PBR_TEXT("LoadMeshAsset: Could not load mesh asset at [%s]. Please make sure the supplied path exists."), AbsoluteFilePath.c_str()));
    }

    return bResult;
}

bool const AssetManager::GetMeshAsset(uint32 const AssetHandle, AssetManager::MeshAsset const *& OutputMeshAsset)
{
    if ((AssetHandle & 0xFFFF) == 0u
        || ((AssetHandle & 0xFFFF0000) >> 16u) != static_cast<uint32>(AssetTypes::Mesh))
    {
        Logging::Log(Logging::LogTypes::Error, PBR_TEXT("GetMeshAsset: Asset handle is NULL OR handle does not reference a mesh."));
        return false;
    }

    uint16 const AssetIndex = { (AssetHandle & 0xFFFF) - 1u };

    PBR_ASSERT(AssetIndex < AssetTable.MeshAssets.size());

    if (!AssetTable.MeshLoadedFlags [AssetIndex])
    {
        auto const FoundPendingAsset = AssetTable.PendingAssets.find(AssetHandle);
        if (FoundPendingAsset == AssetTable.PendingAssets.cend())
        {
            Logging::Log(Logging::LogTypes::Error, PBR_TEXT("Trying to access mesh data for an asset that isn't pending load. This is likely due to the asset failing to load previously."));
            return false;
        }
        else
        {
            auto [bResult, MailboxIndex] = FoundPendingAsset->second.get();
            AssetTable.PendingAssets.erase(AssetHandle);

            if (bResult)
            {
                std::scoped_lock Lock = std::scoped_lock(LoadedAssets.MeshQueueMutex);

                AssetTable.MeshAssets [AssetIndex] = std::move(LoadedAssets.Meshes [MailboxIndex]);
                AssetTable.MeshLoadedFlags [AssetIndex] = true;

                LoadedAssets.FreeMeshIndices.push(MailboxIndex);
            }
            else
            {
                Logging::Log(Logging::LogTypes::Error, String::Format(PBR_TEXT("Failed to load mesh [%s]"), AssetTable.MeshFileNames [AssetIndex].c_str()));

                OutputMeshAsset = nullptr;
                return false;
            }
        }
    }

    OutputMeshAsset = AssetTable.MeshAssets [AssetIndex].get();

    return true;
}