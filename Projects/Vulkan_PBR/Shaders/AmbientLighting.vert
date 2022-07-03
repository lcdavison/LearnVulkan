#version 450

layout (set = 0, binding = 0) 
uniform PerFrameData
{
    mat4x4 WorldToViewMatrix;
    mat4x4 ViewToClipMatrix;
};

layout (location = 0) in vec3 Position;
layout (location = 1) in vec3 Normal;

layout (location = 0) out vec3 FragmentColour;
layout (location = 1) out vec3 FragmentNormal;
layout (location = 2) out vec3 FragmentWorldPosition;

const vec3 DiffuseAlbedo = vec3(0.1f, 0.1f, 0.8f);

const mat4x4 UpAxisConversion = mat4x4(1.0f, 0.0f, 0.0f, 0.0f,
                                       0.0f, 0.0f, 1.0f, 0.0f,
                                       0.0f, -1.0f, 0.0f, 0.0f, 
                                       0.0f, 0.0f, 0.0f, 1.0f);

void main()
{
    const float kScale = 0.1f;
    const mat4x4 kModelToWorldMatrix = mat4x4(kScale, 0.0f, 0.0f, 0.0f,
                                              0.0f, kScale, 0.0f, 0.0f,
                                              0.0f, 0.0f, kScale, 0.0f,
                                              0.0f, 0.0f, 0.0f, 1.0f) * UpAxisConversion;

    vec3 WorldPosition = (kModelToWorldMatrix * vec4(Position.xyz, 1.0f)).xyz;
    WorldPosition += vec3(0.0f, 100.0f, -50.0f);

    gl_Position = ViewToClipMatrix * (WorldToViewMatrix) * vec4(WorldPosition, 1.0f);

    FragmentColour = DiffuseAlbedo;
    FragmentNormal = normalize(vec4(Normal.xyz, 0.0f) * inverse(kModelToWorldMatrix)).xyz;
    FragmentWorldPosition = WorldPosition;
}