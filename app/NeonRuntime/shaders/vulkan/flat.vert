#version 450

// Draws triangles that are placed in pixels, such as those of a user
// interface. A place is counted from the left top corner of the frame.

layout (push_constant) uniform Frame
{
    // the size of the frame in pixels
    vec2 size;

    // added to the place of every corner, in pixels
    vec2 translation;
} frame;

layout (location = 0) in vec2 attr_position;
layout (location = 1) in vec2 attr_tex_coords;
layout (location = 2) in vec4 attr_color;
layout (location = 3) in float attr_textured;

layout (location = 0) out vec2 tex_coord;
layout (location = 1) out vec4 color;
layout (location = 2) out float textured;

void main()
{
    tex_coord = attr_tex_coords;
    color = attr_color;
    textured = attr_textured;

    // Vulkan counts rows from the top as well, so nothing is turned around
    gl_Position = vec4((attr_position + frame.translation) / frame.size * 2.0 - 1.0, 0.0, 1.0);
}
