#version 450
#extension GL_GOOGLE_include_directive : require

// An effect on the light of a scene: what is on the left is shown on the
// right.

#include "effect.glsl"

void main()
{
    frag_color = read_frame(vec2(1.0 - frame_coord.x, frame_coord.y));
}
