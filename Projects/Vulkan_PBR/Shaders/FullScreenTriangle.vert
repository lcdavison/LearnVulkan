#version 450 core

const vec2 kTriangleVertices [3u] =
{
    vec2(-1.0f, -1.0f),
    vec2( 3.0f, -1.0f),
    vec2(-1.0f,  3.0f),
};

void main()
{
    gl_Position = vec4(kTriangleVertices [gl_VertexIndex], 0.0f, 1.0f);
}