#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(push_constant) uniform PushConstants
{
    mat4 projection;
    mat4 modelView;
};

layout(location = 0) in vec3 vertex;
layout(location = 1) in vec4 color;

layout(location = 2) out vec4 rasterColor;

out gl_PerVertex
{
    vec4 gl_Position;
};

void main()
{
    rasterColor = color;
    gl_Position = (projection * modelView) * vec4(vertex, 1.0);
}
