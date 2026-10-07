#version 450
#extension GL_GOOGLE_include_directive : require

// An effect on the light of a scene: the picture gets darker towards its
// corners, as through a lens. It belongs in `effects` of a camera, before
// the tonemapper, since it takes light away.

#include "../effect.glsl"

void main()
{
    vec4 light = read_frame(frame_coord);

    // how far from the middle, 1 in a corner
    float far = length(frame_coord - vec2(0.5)) / 0.7071;
    float kept = 1.0 - 0.75 * smoothstep(0.45, 1.0, far);

    frag_color = vec4(light.rgb * kept, light.a);
}
