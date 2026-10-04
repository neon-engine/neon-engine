#version 450
#extension GL_GOOGLE_include_directive : require

// Where a corner of the square is on the screen. The shader of an extension
// reads what every shader of the engine reads, from the same declarations.

#define object objects[gl_InstanceIndex]
#include "scene-data.glsl"

layout (location = 0) in vec3 attr_pos_coords;
layout (location = 1) in vec3 attr_normal_coords;
layout (location = 2) in vec2 attr_tex_coords;
layout (location = 3) in vec4 attr_color;
layout (location = 4) in vec2 attr_lightmap_coords;

layout (location = 0) out vec2 tex_coord;

void main()
{
    tex_coord = attr_tex_coords;
    gl_Position = scene.projection * scene.view * object.model * vec4(attr_pos_coords, 1.0);
}
