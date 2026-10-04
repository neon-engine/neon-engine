#version 450
#extension GL_GOOGLE_include_directive : require

// An effect on the colours a screen is given: red and blue change places.

#include "effect.glsl"

void main()
{
    frag_color = read_frame(frame_coord).bgra;
}
