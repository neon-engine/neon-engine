#version 450
#extension GL_GOOGLE_include_directive : require

#include "scene-data.glsl"

// The pass that draws the shadow map: every caster as the direction light
// sees it, its depth alone. The pipeline writes the depth, and the
// fragment half has nothing to do.

layout (location = 0) in vec3 attr_pos_coords;
layout (location = 1) in vec3 attr_normal_coords;
layout (location = 2) in vec2 attr_tex_coords;

void main() {
    gl_Position = scene.direction_light.light_view_projection * object.model * vec4(attr_pos_coords, 1.0);
}
