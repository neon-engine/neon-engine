#version 450
#extension GL_GOOGLE_include_directive : require

// Draws triangles that are placed in pixels, such as those of a user
// interface. A place is counted from the left top corner of what is drawn
// to.

#include "ui-frame.glsl"

layout (location = 0) in vec2 attr_position;
layout (location = 1) in vec2 attr_tex_coords;
layout (location = 2) in vec4 attr_color;
layout (location = 3) in float attr_textured;
layout (location = 4) in float attr_shape;
layout (location = 5) in vec2 attr_local;

layout (location = 0) out vec2 tex_coord;
layout (location = 1) out vec4 color;
layout (location = 2) out float textured;
layout (location = 3) out float shape_place;
layout (location = 4) out vec2 local;

void main()
{
    tex_coord = attr_tex_coords;
    color = attr_color;
    textured = attr_textured;
    shape_place = attr_shape;
    local = attr_local;

    // Vulkan counts rows from the top as well, so nothing is turned around
    gl_Position = vec4((attr_position + frame.translation) / frame.size * 2.0 - 1.0, 0.0, 1.0);
}
