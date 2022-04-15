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
	struct OBJMeshData
	{
		std::vector<float> Positions;
		//std::vector<float> Normals;
		//std::vector<float> TextureCoordinates;
	};

	OBJ_LOADER_API bool const LoadFile(std::filesystem::path const & OBJFilePath, OBJMeshData & OutputMeshData);
}