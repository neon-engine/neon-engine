#version 330 core
layout (location = 0) in vec3 attr_position_coord;
layout (location = 1) in vec3 attr_normal_coord;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec3 frag_coord;
out vec3 normal_coord;

void main() {
    gl_Position = projection * view * model * vec4(attr_position_coord, 1.0);
    frag_coord = vec3(model * vec4(attr_position_coord, 1.0));
    normal_coord = attr_normal_coord;
}