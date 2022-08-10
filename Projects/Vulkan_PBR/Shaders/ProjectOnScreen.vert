#version 450

layout (set = 0, binding = 0) 
uniform PerFrameData
{
    mat4x4 WorldToViewMatrix;
    mat4x4 ViewToClipMatrix;
};

layout (set = 0, binding = 1)
uniform PerDrawData
{
    mat4x4 ModelToWorldMatrix;
};

layout (location = 0) in vec3 Position;
layout (location = 1) in vec3 Normal;
layout (location = 2) in vec3 UV;

layout (location = 0) out vec3 FragmentColour;
layout (location = 1) out vec3 FragmentNormal;
layout (location = 2) out vec3 FragmentWorldPosition;
layout (location = 3) out vec3 FragmentUV;
layout (location = 4) out vec3 ViewPositionWorldSpace;

const vec3 DiffuseAlbedo = vec3(0.1f, 0.1f, 0.8f);

const mat4x4 UpAxisConversion = mat4x4(1.0f, 0.0f, 0.0f, 0.0f,
                                       0.0f, 0.0f, 1.0f, 0.0f,
                                       0.0f, -1.0f, 0.0f, 0.0f, 
                                       0.0f, 0.0f, 0.0f, 1.0f);

void main()
{
    mat4x4 Transformation = ModelToWorldMatrix * UpAxisConversion;

    vec4 WorldPosition = Transformation * vec4(Position.xyz, 1.0f);

    gl_Position = ViewToClipMatrix * (WorldToViewMatrix) * WorldPosition;

    FragmentColour = DiffuseAlbedo;
    FragmentNormal = (vec4(Normal.xyz, 0.0f) * inverse(Transformation)).xyz;
    FragmentWorldPosition = WorldPosition.xyz;
    FragmentUV = UV;
    ViewPositionWorldSpace = (inverse(WorldToViewMatrix) [3u]).xyz;
}