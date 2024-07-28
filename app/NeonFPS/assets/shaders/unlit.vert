#version 330 core
layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal_coord;
layout (location = 2) in vec2 uv_coord;

out vec3 out_normal;
out vec2 out_uv_coord;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    gl_Position = projection * view * model * vec4(position, 1.0);
    out_normal = normal_coord;
    out_uv_coord = uv_coord;
}
