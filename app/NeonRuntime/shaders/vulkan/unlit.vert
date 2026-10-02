#version 450
#extension GL_GOOGLE_include_directive : require

// Draws an object with its texture as it is, without lighting.

#include "scene-data.glsl"

layout (location = 0) in vec3 attr_pos_coords;
layout (location = 1) in vec3 attr_normal_coords;
layout (location = 2) in vec2 attr_tex_coords;
layout (location = 3) in vec4 attr_color;

layout (location = 0) out vec2 tex_coord;
layout (location = 1) out vec4 vertex_color;

void main()
{
    tex_coord = attr_tex_coords * object.texture_scale.xy;
    vertex_color = attr_color;

    gl_Position = scene.projection * scene.view * object.model * vec4(attr_pos_coords, 1.0);
}
