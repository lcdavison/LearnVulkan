#version 450

layout (set = 0, binding = 0) 
uniform PerFrameData
{
    mat4x4 PerspectiveMatrix;
    float Brightness;
};

layout (location = 0) out vec3 FragmentColour;
layout (location = 1) out float FragmentBrightness;

vec2 TriangleVertexPositions [3u] =
{
    vec2(-0.5f,  0.5f),
    vec2( 0.5f,  0.5f),
    vec2( 0.0f, -0.5f),
};

vec3 Colours [3u] =
{
    vec3(1.0f, 0.0f, 0.0f),
    vec3(0.0f, 1.0f, 0.0f),
    vec3(0.0f, 0.0f, 1.0f),
};

void main()
{
    gl_Position = vec4(TriangleVertexPositions [gl_VertexIndex].xy, 0.0f, 1.0f);

    FragmentColour = Colours [gl_VertexIndex];
    FragmentBrightness = Brightness;
}