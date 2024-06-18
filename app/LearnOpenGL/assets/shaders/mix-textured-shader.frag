#version 330 core

out vec4 color;

in vec3 our_color;
in vec2 tex_coord;

uniform sampler2D texture1;
uniform sampler2D texture2;

void main()
{
    vec4 texture1_color = texture(texture1, tex_coord);
    vec4 texture2_color = texture(texture2, tex_coord);
    color = vec4(mix(texture1_color.rgb, texture2_color.rgb, 0.2), 1.0);
}
