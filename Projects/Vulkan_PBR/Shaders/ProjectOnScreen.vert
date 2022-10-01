#version 450

layout (set = 0, binding = 0) 
uniform PerFrameData
{
    mat4x4 WorldToViewMatrix;
    mat4x4 ViewToClipMatrix;
};

layout (set = 1, binding = 0)
uniform PerDrawData
{
    mat4x4 ModelToWorldMatrix;
};

layout (location = 0) in vec3 Position;
layout (location = 1) in vec3 Normal;
layout (location = 2) in vec4 Tangent;
layout (location = 3) in vec3 UV;

layout (location = 0) out vec3 FragmentPositionWS;
layout (location = 1) out vec3 FragmentPositionVS;
layout (location = 2) out vec3 FragmentNormalWS;
layout (location = 3) out vec4 FragmentTangentWS;
layout (location = 4) out vec3 FragmentUV;
layout (location = 5) out vec3 ViewPositionWS;

const vec3 DiffuseAlbedo = vec3(0.1f, 0.1f, 0.8f);

const mat4x4 UpAxisConversion = mat4x4(1.0f, 0.0f, 0.0f, 0.0f,
                                       0.0f, 0.0f, 1.0f, 0.0f,
                                       0.0f, -1.0f, 0.0f, 0.0f, 
                                       0.0f, 0.0f, 0.0f, 1.0f);

void main()
{
    mat4x4 Transformation = ModelToWorldMatrix * UpAxisConversion;

    vec4 PositionWS = Transformation * vec4(Position.xyz, 1.0f);
    vec4 PositionVS = WorldToViewMatrix * PositionWS;
    gl_Position = ViewToClipMatrix * PositionVS;

    FragmentPositionWS = PositionWS.xyz;
    FragmentPositionVS = PositionVS.xyz;
    FragmentNormalWS = (Transformation * vec4(Normal.xyz, 0.0f)).xyz; // This is fine as long as we don't use non-uniform scaling
    FragmentTangentWS = vec4((Transformation * vec4(Tangent.xyz, 0.0f)).xyz, Tangent.w);
    FragmentUV = UV;
    ViewPositionWS = (inverse(WorldToViewMatrix) [3u]).xyz;
}