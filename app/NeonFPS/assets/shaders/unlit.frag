#version 330 core

out vec4 color;

in vec3 out_normal;
in vec2 out_uv_coord;

uniform sampler2D texture0;

void main()
{
    color = texture(texture0, out_uv_coord);
}
