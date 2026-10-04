#version 450
#extension GL_GOOGLE_include_directive : require

// An effect on the light of a scene: half the light. It is run before the
// tonemapper, so half of white is 188 on a screen, and not 128.

#include "effect.glsl"

void main()
{
    vec4 light = read_frame(frame_coord);
    frag_color = vec4(light.rgb * 0.5, light.a);
}
