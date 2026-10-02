#version 450
#extension GL_GOOGLE_include_directive : require

#include "scene-data.glsl"

layout (location = 0) in vec2 tex_coord;

layout (location = 0) out vec4 frag_color;

// the texture, and the sampler it is read through, bound two bindings on
layout (set = 0, binding = 2) uniform texture2D diffuse_texture;
layout (set = 0, binding = 4) uniform sampler diffuse_sampler;

void main()
{
    vec4 texel = texture(sampler2D(diffuse_texture, diffuse_sampler), tex_coord);
    frag_color = vec4(texel.rgb, object_alpha(texel.a));
}
