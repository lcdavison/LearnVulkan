#include "ShaderCompiler/ShaderCompiler.hpp"

#include <glslang/Include/glslang_c_interface.h>

#include "CommonTypes.hpp"

#include <fstream>
#include <sstream>
#include <string_view>
#include <unordered_map>

using namespace Platform;

/* Too many underscores, can't ... type .. fast */
using GLSLangInput = glslang_input_t;
using GLSLangStage = glslang_stage_t;
using GLSLangShader = glslang_shader_t;
using GLSLangProgram = glslang_program_t;
using GLSLangResourceInfo = glslang_resource_t;

static GLSLangResourceInfo const kDefaultResourceInfo =
{
    /* MaxLights */
    32,
    /* MaxClipPlanes */
    6,
    /* .MaxTextureUnits = */
    32,
    /* MaxTextureCoords */
    32,
    /* MaxVertexAttribs */
    64,
    /* .MaxVertexUniformComponents = */
    4096,
    /* .MaxVaryingFloats = */
    64,
    /* .MaxVertexTextureImageUnits = */
    32,
    /* .MaxCombinedTextureImageUnits = */
    80,
    /* .MaxTextureImageUnits = */
    32,
    /* .MaxFragmentUniformComponents = */
    4096,
    /* .MaxDrawBuffers = */
    32,
    /* .MaxVertexUniformVectors = */
    128,
    /* .MaxVaryingVectors = */
    8,
    /* .MaxFragmentUniformVectors = */
    16,
    /* .MaxVertexOutputVectors = */
    16,
    /* .MaxFragmentInputVectors = */
    15,
    /* .MinProgramTexelOffset = */
    -8,
    /* .MaxProgramTexelOffset = */
    7,
    /* .MaxClipDistances = */
    8,
    /* .MaxComputeWorkGroupCountX = */
    65535,
    /* .MaxComputeWorkGroupCountY = */
    65535,
    /* .MaxComputeWorkGroupCountZ = */
    65535,
    /* .MaxComputeWorkGroupSizeX = */
    1024,
    /* .MaxComputeWorkGroupSizeY = */
    1024,
    /* .MaxComputeWorkGroupSizeZ = */
    64,
    /* .MaxComputeUniformComponents = */
    1024,
    /* .MaxComputeTextureImageUnits = */
    16,
    /* .MaxComputeImageUniforms = */
    8,
    /* .MaxComputeAtomicCounters = */
    8,
    /* .MaxComputeAtomicCounterBuffers = */
    1,
    /* .MaxVaryingComponents = */
    60,
    /* .MaxVertexOutputComponents = */
    64,
    /* .MaxGeometryInputComponents = */
    64,
    /* .MaxGeometryOutputComponents = */
    128,
    /* .MaxFragmentInputComponents = */
    128,
    /* .MaxImageUnits = */
    8,
    /* .MaxCombinedImageUnitsAndFragmentOutputs = */
    8,
    /* .MaxCombinedShaderOutputResources = */
    8,
    /* .MaxImageSamples = */
    0,
    /* .MaxVertexImageUniforms = */
    0,
    /* .MaxTessControlImageUniforms = */
    0,
    /* .MaxTessEvaluationImageUniforms = */
    0,
    /* .MaxGeometryImageUniforms = */
    0,
    /* .MaxFragmentImageUniforms = */
    8,
    /* .MaxCombinedImageUniforms = */
    8,
    /* .MaxGeometryTextureImageUnits = */
    16,
    /* .MaxGeometryOutputVertices = */
    256,
    /* .MaxGeometryTotalOutputComponents = */
    1024,
    /* .MaxGeometryUniformComponents = */
    1024,
    /* .MaxGeometryVaryingComponents = */
    64,
    /* .MaxTessControlInputComponents = */
    128,
    /* .MaxTessControlOutputComponents = */
    128,
    /* .MaxTessControlTextureImageUnits = */
    16,
    /* .MaxTessControlUniformComponents = */
    1024,
    /* .MaxTessControlTotalOutputComponents = */
    4096,
    /* .MaxTessEvaluationInputComponents = */
    128,
    /* .MaxTessEvaluationOutputComponents = */
    128,
    /* .MaxTessEvaluationTextureImageUnits = */
    16,
    /* .MaxTessEvaluationUniformComponents = */
    1024,
    /* .MaxTessPatchComponents = */
    120,
    /* .MaxPatchVertices = */
    32,
    /* .MaxTessGenLevel = */
    64,
    /* .MaxViewports = */
    16,
    /* .MaxVertexAtomicCounters = */
    0,
    /* .MaxTessControlAtomicCounters = */
    0,
    /* .MaxTessEvaluationAtomicCounters = */
    0,
    /* .MaxGeometryAtomicCounters = */
    0,
    /* .MaxFragmentAtomicCounters = */
    8,
    /* .MaxCombinedAtomicCounters = */
    8,
    /* .MaxAtomicCounterBindings = */
    1,
    /* .MaxVertexAtomicCounterBuffers = */
    0,
    /* .MaxTessControlAtomicCounterBuffers = */
    0,
    /* .MaxTessEvaluationAtomicCounterBuffers = */
    0,
    /* .MaxGeometryAtomicCounterBuffers = */
    0,
    /* .MaxFragmentAtomicCounterBuffers = */
    1,
    /* .MaxCombinedAtomicCounterBuffers = */
    1,
    /* .MaxAtomicCounterBufferSize = */
    16384,
    /* .MaxTransformFeedbackBuffers = */
    4,
    /* .MaxTransformFeedbackInterleavedComponents = */
    64,
    /* .MaxCullDistances = */
    8,
    /* .MaxCombinedClipAndCullDistances = */
    8,
    /* .MaxSamples = */
    4,
    /* .maxMeshOutputVerticesNV = */
    256,
    /* .maxMeshOutputPrimitivesNV = */
    512,
    /* .maxMeshWorkGroupSizeX_NV = */
    32,
    /* .maxMeshWorkGroupSizeY_NV = */
    1,
    /* .maxMeshWorkGroupSizeZ_NV = */
    1,
    /* .maxTaskWorkGroupSizeX_NV = */
    32,
    /* .maxTaskWorkGroupSizeY_NV = */
    1,
    /* .maxTaskWorkGroupSizeZ_NV = */
    1,
    /* .maxMeshViewCountNV = */
    4,
    /* .maxDualSourceDrawBuffersEXT = */
    1,

    /* .limits = */
    {
        /* .nonInductiveForLoops = */
    1,
    /* .whileLoops = */
    1,
    /* .doWhileLoops = */
    1,
    /* .generalUniformIndexing = */
    1,
    /* .generalAttributeMatrixVectorIndexing = */
    1,
    /* .generalVaryingIndexing = */
    1,
    /* .generalSamplerIndexing = */
    1,
    /* .generalVariableIndexing = */
    1,
    /* .generalConstantMatrixVectorIndexing = */
    1,
    }
};

static std::unordered_map<std::string, GLSLangStage> const ShaderStageMap =
{
    std::make_pair(".vert", GLSLANG_STAGE_VERTEX),
    std::make_pair(".frag", GLSLANG_STAGE_FRAGMENT),
};

bool const ShaderCompiler::Initialise()
{
    return glslang_initialize_process() != 0l;
}

void ShaderCompiler::Destroy()
{
    glslang_finalize_process();
}

GLSLangStage GetShaderStage(std::filesystem::path const & ShaderFilePath)
{
    std::filesystem::path const ShaderFileExtension = ShaderFilePath.extension();
    return ShaderStageMap.at(ShaderFileExtension.string());
}

bool const ShaderCompiler::CompileShader(std::filesystem::path const & ShaderFilePath, unsigned int *& OutputByteCode, uint64 & OutputByteCodeSizeInBytes, std::basic_string<Windows::TCHAR> & OutputErrorMessage)
{
    bool bResult = false;

    if (std::filesystem::exists(ShaderFilePath))
    {
        std::fstream ShaderFileStream = std::fstream(ShaderFilePath, std::fstream::in | std::fstream::binary);

        uint64 const FileSizeInBytes = std::filesystem::file_size(ShaderFilePath);

        std::string FileContent = std::string(FileSizeInBytes, '\0');
        if (ShaderFileStream.read(FileContent.data(), FileSizeInBytes))
        {
            GLSLangShader * Shader = {};
            GLSLangProgram * Program = {};

            do
            {
                GLSLangStage const ShaderStage = ::GetShaderStage(ShaderFilePath);

                GLSLangInput Input = {};
                Input.language = GLSLANG_SOURCE_GLSL;
                Input.client = GLSLANG_CLIENT_VULKAN;
                Input.client_version = GLSLANG_TARGET_VULKAN_1_3;
                Input.target_language = GLSLANG_TARGET_SPV;
                Input.target_language_version = GLSLANG_TARGET_SPV_1_3;
                Input.resource = &kDefaultResourceInfo;
                Input.messages = GLSLANG_MSG_DEFAULT_BIT;
                Input.default_profile = GLSLANG_NO_PROFILE;
                Input.forward_compatible = 0l;
                Input.stage = ShaderStage;
                Input.code = FileContent.data();

                Shader = glslang_shader_create(&Input);

                /* This returns int, but this is implicitly cast from a bool */
                if (!glslang_shader_preprocess(Shader, &Input))
                {
                    std::basic_stringstream ErrorOutput = std::basic_stringstream<Windows::TCHAR>();
                    ErrorOutput << "Info Log:\n" << glslang_shader_get_info_log(Shader) << "\n";
                    ErrorOutput << "Debug Info Log:\n" << glslang_shader_get_info_debug_log(Shader) << "\n";

                    OutputErrorMessage = ErrorOutput.str();
                    break;
                }

                if (!glslang_shader_parse(Shader, &Input))
                {
                    std::basic_stringstream ErrorOutput = std::basic_stringstream<Windows::TCHAR>();
                    ErrorOutput << "Info Log:\n" << glslang_shader_get_info_log(Shader);
                    ErrorOutput << "Debug Info Log:\n" << glslang_shader_get_info_debug_log(Shader);

                    OutputErrorMessage = ErrorOutput.str();
                    break;
                }

                Program = glslang_program_create();
                glslang_program_add_shader(Program, Shader);

                if (!glslang_program_link(Program, 0l))
                {
                    std::basic_stringstream ErrorOutput = std::basic_stringstream<Windows::TCHAR>();
                    ErrorOutput << "Info Log:\n" << glslang_program_get_info_log(Program) << "\n";
                    ErrorOutput << "Debug Info Log:\n" << glslang_program_get_info_debug_log(Program) << "\n";

                    OutputErrorMessage = ErrorOutput.str();
                    break;
                }

                glslang_program_SPIRV_generate(Program, ShaderStage);

                uint64 const ByteCodeSizeInWords = glslang_program_SPIRV_get_size(Program);
                unsigned int * ByteCode = new unsigned int [ByteCodeSizeInWords];

                glslang_program_SPIRV_get(Program, ByteCode);

                char const * SPIRVMessages = glslang_program_SPIRV_get_messages(Program);

                if (SPIRVMessages)
                {
                    std::basic_stringstream SPIRVOutput = std::basic_stringstream<Windows::TCHAR>();
                    SPIRVOutput << "SPIRV Output:\n" << SPIRVMessages;

                    OutputErrorMessage = SPIRVOutput.str();
                    break;
                }

                OutputByteCodeSizeInBytes = ByteCodeSizeInWords * sizeof(unsigned int);
                OutputByteCode = ByteCode;
                bResult = true;
            }
            while (false);

            glslang_program_delete(Program);
            glslang_shader_delete(Shader);
        }

        ShaderFileStream.close();
    }

    return bResult;
}