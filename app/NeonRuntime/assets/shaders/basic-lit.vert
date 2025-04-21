#version 330 core
layout (location = 0) in vec3 attr_pos_coords;
layout (location = 1) in vec3 attr_normal_coords;
layout (location = 2) in vec2 attr_tex_coords;

out vec3 frag_coord;
out vec3 normal_coord;
out vec2 tex_coord;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform vec2 texture_scale;

void main() {
    frag_coord = vec3(model * vec4(attr_pos_coords, 1.0));
    normal_coord = mat3(transpose(inverse(model))) * attr_normal_coords;
    tex_coord = attr_tex_coords;
    tex_coord.x = tex_coord.x * texture_scale.x;
    tex_coord.y = tex_coord.y * texture_scale.y;

    gl_Position = projection * view * model * vec4(attr_pos_coords, 1.0);
}