#version 450
#extension GL_GOOGLE_include_directive : require

#include "scene-data.glsl"

layout (location = 0) in vec2 tex_coord;

layout (location = 0) out vec4 frag_color;

layout (set = 0, binding = 2) uniform sampler2D diffuse_texture;

void main()
{
    frag_color = texture(diffuse_texture, tex_coord);
}
