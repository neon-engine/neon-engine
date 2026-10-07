#version 450
#extension GL_GOOGLE_include_directive : require

#define object objects[object_index]
#include "scene-data.glsl"

layout (location = 0) flat in uint object_index;

layout (location = 0) out vec4 frag_color;

void main()
{
    frag_color = vec4(object.color.rgb, object_alpha(object.material, object.color.a));
}
