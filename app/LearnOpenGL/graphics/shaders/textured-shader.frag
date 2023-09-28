#version 330 core
out vec4 color;

in vec3 our_color;
in vec2 tex_coord;

uniform sampler2D ourTexture;

void main()
{
    color = texture(ourTexture, tex_coord);
}
