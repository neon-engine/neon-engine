#version 450
#extension GL_GOOGLE_include_directive : require

// A band of light that moves over an element from left to right, again and
// again, as over a button that asks to be pressed.
//
//     shader: engine://shaders/ui/shine
//     shader_values: { speed: 0.5, width: 0.2, tint: "#ffffff80" }

#include "../ui-shader.glsl"

layout (set = 2, binding = 0) uniform Values
{
    // how often in a second the band crosses the element
    float speed;

    // how wide the band is, in parts of the width of the element
    float width;

    // how far the band leans: 0 stands upright, 1 leans by the height of
    // the element
    float lean;

    // the color of the band, whose alpha says how bright it is
    vec4 tint;
} values;

void main()
{
    vec4 base = ui_base();
    vec2 place = ui_element_uv();

    // from before the left edge to behind the right one, so that the band
    // is gone for a moment between two crossings
    float reach = 1.0 + 2.0 * values.width + abs(values.lean);
    float middle = fract(ui_time() * values.speed) * reach - values.width - max(values.lean, 0.0);

    float along = place.x - place.y * values.lean;
    float band = 1.0 - smoothstep(0.0, max(values.width, 0.0001), abs(along - middle));

    // light is added where the element is, and as much as it covers
    vec3 light = values.tint.rgb * values.tint.a * band * base.a;
    frag_color = vec4(base.rgb + light, base.a);
}
