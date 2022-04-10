#version 450

layout (location = 0) in vec3 FragmentColour;

layout (location = 0) out vec4 OutputFragmentColour;

void main()
{
    OutputFragmentColour = vec4(FragmentColour.rgb, 1.0f);
}