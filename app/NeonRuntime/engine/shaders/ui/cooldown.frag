#version 450
#extension GL_GOOGLE_include_directive : require

// What is left of a time of waiting, as a shade that is wiped off an
// element clockwise from the top, as over the icon of an ability.
//
//     shader: engine://shaders/ui/cooldown
//     shader_values: { progress: "{charge}", shade: "#000000b0" }

#include "../ui-shader.glsl"

layout (set = 2, binding = 0) uniform Values
{
    // how much of the time has passed, from 0 to 1
    float progress;

    // what lies over the part that is still waiting
    vec4 shade;
} values;

void main()
{
    vec4 base = ui_base();

    // The angle of the pixel around the middle, from 0 at the top and
    // clockwise to 1. Counted in pixels, so that the hand of the clock
    // moves as evenly over a box that is wider than high.
    vec2 from_middle = (ui_element_uv() - 0.5) * ui_element_size();
    float angle = atan(from_middle.x, -from_middle.y) / 6.28318530718;
    if (angle < 0.0) { angle += 1.0; }

    // smoothed over a pixel along the hand, but not where the hand is at
    // the top, which is where the time starts and ends
    float pixel = 1.0 / max(6.28318530718 * length(from_middle), 1.0);
    float progress = clamp(values.progress, 0.0, 1.0);
    float waiting = progress >= 1.0 ? 0.0 : smoothstep(progress - pixel, progress + pixel, angle);
    if (progress <= 0.0) { waiting = 1.0; }

    float cover = values.shade.a * waiting;
    frag_color = vec4(mix(base.rgb, values.shade.rgb * base.a, cover), base.a);
}
