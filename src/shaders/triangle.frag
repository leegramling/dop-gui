#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 2) in vec4 rasterColor;
layout(location = 0) out vec4 fragmentColor;

void main()
{
    fragmentColor = rasterColor;
}
