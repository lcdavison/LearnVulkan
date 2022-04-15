#include "OBJLoader.hpp"

#include <fstream>
#include <string>

static void ProcessVertexAttribute(std::string const& AttributeLine, std::vector<float>& OutputAttributeData)
{
	
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
		char const& LineStartCharacter = CurrentFileLine[0u];

		switch (LineStartCharacter)
		{
		case '#':
			break;
		case 'v':
		{
			/* Vertex, Normal etc... */
			char const& AttributeCharacter = CurrentFileLine[1u];

			switch (AttributeCharacter)
			{
			case ' ':
				::ProcessVertexAttribute(CurrentFileLine, Positions);
				break;
			case 'n':
				::ProcessVertexAttribute(CurrentFileLine, Normals);
				break;
			case 't':
				::ProcessVertexAttribute(CurrentFileLine, TextureCoordinates);
				break;
			}
		}
			break;
		case 'f':
			/* Face */
			break;
		}
	}

	return bResult;
}

bool const OBJLoader::LoadFile(std::filesystem::path const & OBJFilePath, OBJMeshData & OutputMeshData)
{
	bool bResult = false;

	if (std::filesystem::exists(OBJFilePath) &&
		OBJFilePath.has_extension() && OBJFilePath.extension() == ".obj")
	{
		std::ifstream FileStream = std::ifstream(OBJFilePath);

		bResult = ::ParseOBJFile(FileStream);

		FileStream.close();
	}

	return bResult;
}