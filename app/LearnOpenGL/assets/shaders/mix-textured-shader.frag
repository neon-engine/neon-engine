#version 330 core

out vec4 color;

in vec3 our_color;
in vec2 tex_coord;

uniform sampler2D texture1;
uniform sampler2D texture2;

void main()
{
    color = mix(texture(texture1, tex_coord), texture(texture2, tex_coord), 0.2);
}
