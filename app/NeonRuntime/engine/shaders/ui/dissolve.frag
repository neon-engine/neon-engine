#version 450
#extension GL_GOOGLE_include_directive : require

// An element that falls apart into grains, with a glowing edge where it is
// about to go.
//
//     shader: engine://shaders/ui/dissolve
//     shader_values: { amount: "{fade}", grain: 6, edge: "#ff8000" }

#include "../ui-shader.glsl"

layout (set = 2, binding = 0) uniform Values
{
    // how much of the element is gone, from 0 to 1
    float amount;

    // the size of a grain in pixels
    float grain;

    // the colour of the edge
    vec4 edge;
} values;

float grain_of(vec2 cell)
{
    return fract(sin(dot(cell, vec2(127.1, 311.7))) * 43758.5453);
}

// Grains that blend into their neighbours, so that the edge between what
// is there and what is gone is a line and not a scatter of dots.
float noise_at(vec2 point)
{
    vec2 cell = floor(point);
    vec2 part = fract(point);
    part = part * part * (3.0 - 2.0 * part);

    float top = mix(grain_of(cell), grain_of(cell + vec2(1.0, 0.0)), part.x);
    float bottom = mix(grain_of(cell + vec2(0.0, 1.0)), grain_of(cell + vec2(1.0, 1.0)), part.x);
    return mix(top, bottom, part.y);
}

void main()
{
    vec4 base = ui_base();

    vec2 point = ui_element_uv() * ui_element_size() / max(values.grain, 1.0);
    float grain = 0.65 * noise_at(point) + 0.35 * noise_at(point * 2.7 + 13.0);

    // a little past both ends, so that 0 shows everything and 1 nothing
    float gone = values.amount * 1.2 - 0.1;

    float kept = smoothstep(gone, gone + 0.02, grain);
    float glow = kept * (1.0 - smoothstep(gone + 0.02, gone + 0.12, grain));

    vec3 edge = values.edge.rgb * values.edge.a * glow * base.a;
    frag_color = vec4(base.rgb * kept + edge, base.a * kept);
}
