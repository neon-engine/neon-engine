#version 450
#extension GL_GOOGLE_include_directive : require

// An effect on the colours a screen is given: every other pair of rows is
// darker, and a band of light runs down the picture, as on a monitor with a
// tube. It belongs in `screen_effects` of a camera, after the tonemapper,
// since it is about the rows of the picture that is shown.

#include "../effect.glsl"

void main()
{
    vec4 shown = read_frame(frame_coord);

    float row = floor(frame_coord.y * frame_size().y);
    float lines = mod(row, 4.0) < 2.0 ? 1.0 : 0.72;

    // once down the picture every four seconds
    float band = fract(frame_coord.y - scene.time.x * 0.25);
    float glow = 1.0 + 0.12 * smoothstep(0.92, 1.0, band);

    frag_color = vec4(min(shown.rgb * lines * glow, vec3(shown.a)), shown.a);
}
