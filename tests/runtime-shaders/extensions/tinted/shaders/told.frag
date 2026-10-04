#version 450
#extension GL_GOOGLE_include_directive : require

// An effect that shows the picture as it is when it is told what the
// shaders of a material are told: that time runs, and the numbers the
// extension set at place 3. When not, the picture is black.

#include "effect.glsl"

void main()
{
    bool told = scene.time.x > 0.0 && scene.time.y > 0.0 && scene.numbers[3].r == 1.0 && scene.numbers[3].g == 0.5;
    bool sized = frame_size() == vec2(1280.0, 720.0);
    frag_color = told && sized ? read_frame(frame_coord) : vec4(0.0, 0.0, 0.0, 1.0);
}
