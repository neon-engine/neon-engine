#version 450
#extension GL_GOOGLE_include_directive : require

#define object objects[gl_InstanceIndex]
#include "scene-data.glsl"

// The pass that draws the shadow map: every caster as the direction light
// sees it, its depth alone, once for each cascade, which a push constant
// names. The pipeline writes the depth, and the fragment half has nothing
// to do.

layout (push_constant) uniform Cascade {
    uint index;
} cascade;

layout (location = 0) in vec3 attr_pos_coords;
layout (location = 1) in vec3 attr_normal_coords;
layout (location = 2) in vec2 attr_tex_coords;

void main() {
    gl_Position = scene.direction_light.cascades[cascade.index] * object.model * vec4(attr_pos_coords, 1.0);
}
