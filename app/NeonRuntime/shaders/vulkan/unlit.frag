#version 450
#extension GL_GOOGLE_include_directive : require

#include "scene-data.glsl"

layout (location = 0) in vec2 tex_coord;

layout (location = 0) out vec4 frag_color;

layout (set = 0, binding = 2) uniform sampler2D diffuse_texture;

void main()
{
    vec4 texel = texture(diffuse_texture, tex_coord);
    frag_color = vec4(texel.rgb, object_alpha(texel.a));
}
