#include "Assets/Material.hpp"

#include <vector>
#include <queue>

#include <unordered_map>

struct MaterialCollection
{
    std::vector<uint32> AlbedoTextures = {};
    std::vector<uint32> NormalTextures = {};
    std::vector<uint32> SpecularTextures = {};
    std::vector<uint32> RoughnessTextures = {};
    std::vector<uint32> AmbientOcclusionTextures = {};
};

static MaterialCollection Materials = {};

static std::unordered_map<std::string, uint32> AssetNameToHandleMap = {};

bool const Assets::Material::CreateMaterial(Assets::Material::MaterialData const & MaterialDesc, std::string AssetName, uint32 & OutputMaterialHandle)
{
    /* TODO: Check the textures and use a default texture if any in desc are NULL */

    Materials.AlbedoTextures.push_back(MaterialDesc.AlbedoTexture);
    Materials.NormalTextures.push_back(MaterialDesc.NormalTexture);
    Materials.SpecularTextures.push_back(MaterialDesc.SpecularTexture);
    Materials.RoughnessTextures.push_back(MaterialDesc.RoughnessTexture);
    Materials.AmbientOcclusionTextures.push_back(MaterialDesc.AmbientOcclusionTexture);

    OutputMaterialHandle = static_cast<uint32>(Materials.AlbedoTextures.size());

    AssetNameToHandleMap [std::move(AssetName)] = OutputMaterialHandle;

    return true;
}