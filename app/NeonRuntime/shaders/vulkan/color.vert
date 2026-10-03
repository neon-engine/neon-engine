#version 450
#extension GL_GOOGLE_include_directive : require

// Draws an object in the plain color of its material, without lighting.

#define object objects[gl_InstanceIndex]
#include "scene-data.glsl"

layout (location = 0) in vec3 attr_pos_coords;
layout (location = 1) in vec3 attr_normal_coords;
layout (location = 2) in vec2 attr_tex_coords;

layout (location = 0) flat out uint object_index;

void main() {
    object_index = gl_InstanceIndex;
    gl_Position = scene.projection * scene.view * object.model * vec4(attr_pos_coords, 1.0);
}
