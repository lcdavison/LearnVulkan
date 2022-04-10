#pragma once

#include <Windows.h>

#include "CommonTypes.hpp"

#include <filesystem>
#include <vector>
#include <future>

namespace ShaderCompiler
{
    extern bool const Initialise();
    extern void Destroy();

    extern bool const CompileShader(std::filesystem::path const & FilePath, unsigned int *& OutputByteCode, uint64 & OutputByteCodeSizeInBytes, std::basic_string<TCHAR> & OutputErrorMessage);
}