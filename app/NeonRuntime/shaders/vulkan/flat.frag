#version 450

layout (location = 0) in vec2 tex_coord;
layout (location = 1) in vec4 color;
layout (location = 2) in float textured;

layout (location = 0) out vec4 frag_color;

// alpha is multiplied into its colours
layout (set = 0, binding = 0) uniform sampler2D image;

void main()
{
    // a corner that reads no texture is white where the texture would be
    vec4 texel = mix(vec4(1.0), texture(image, tex_coord), textured);

    // Alpha is multiplied into the colour here. What is written is then
    // added to what is behind it, less the part this pixel covers.
    frag_color = texel * vec4(color.rgb * color.a, color.a);
}
