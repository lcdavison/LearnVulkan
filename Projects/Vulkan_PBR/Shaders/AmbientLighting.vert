#version 450

layout (set = 0, binding = 0) 
uniform PerFrameData
{
    mat4x4 PerspectiveMatrix;
    float Brightness;
};

layout (location = 0) in vec3 Position;
layout (location = 1) in vec3 Normal;

layout (location = 0) out vec3 FragmentColour;
layout (location = 1) out vec3 FragmentNormal;
layout (location = 2) out float FragmentBrightness;

const vec3 DiffuseAlbedo = vec3(0.7f, 0.2f, 0.1f);

void main()
{
    float kScale = 2.5f;

    gl_Position = PerspectiveMatrix * vec4(kScale * Position.xyz + vec3(-5.0f, 2.5f, 15.0f), 1.0f);

    FragmentColour = DiffuseAlbedo;
    FragmentNormal = normalize(Normal);
    FragmentBrightness = Brightness;
}