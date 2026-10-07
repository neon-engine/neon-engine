#version 450
#extension GL_GOOGLE_include_directive : require

#define object objects[gl_InstanceIndex]
#include "scene-data.glsl"

layout (location = 0) in vec3 attr_pos_coords;
layout (location = 1) in vec3 attr_normal_coords;
layout (location = 2) in vec2 attr_tex_coords;
layout (location = 3) in vec4 attr_color;
layout (location = 4) in vec2 attr_lightmap_coords;

layout (location = 0) out vec3 frag_coord;
layout (location = 1) out vec3 normal_coord;
layout (location = 2) out vec2 tex_coord;
layout (location = 3) out vec4 vertex_color;
layout (location = 4) flat out uint object_index;
layout (location = 5) out vec2 lightmap_coord;

void main() {
    object_index = gl_InstanceIndex;
    frag_coord = vec3(object.model * vec4(attr_pos_coords, 1.0));
    normal_coord = mat3(object.normal_matrix) * attr_normal_coords;
    tex_coord = attr_tex_coords * object.texture_scale.xy;
    vertex_color = attr_color;
    lightmap_coord = attr_lightmap_coords;

    gl_Position = scene.projection * scene.view * object.model * vec4(attr_pos_coords, 1.0);
}
