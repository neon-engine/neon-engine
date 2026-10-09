#version 450

// The resolve step: the one place where the linear light of the scene
// becomes the colors of the image that is shown. Light that bleeds around
// what is bright is added to the scene image before this. Here the
// exposure is multiplied in, the tonemapper maps what is brighter than
// white into what a screen can show, and the result is encoded in sRGB.

// The scene image, and the sampler it is read through, bound apart so
// that every target of the shaders binds what the source says. The read
// below is pixel by pixel, so the sampler does nothing but has to be
// there.
layout (set = 0, binding = 0) uniform texture2D scene_image;
layout (set = 0, binding = 1) uniform sampler scene_sampler;

// The settings of the run, which have to match VK_Resolve::Data: x the
// tonemapper as the setting numbers them, y the exposure.
layout (std140, set = 0, binding = 2) uniform ResolveData {
    vec4 settings;
} resolve;

layout (location = 0) out vec4 frag_color;

const float TONEMAPPER_NONE = 0.0;
const float TONEMAPPER_ACES = 1.0;
const float TONEMAPPER_AGX = 2.0;

// The ACES filmic curve as Krzysztof Narkowicz fits it (2016): one
// rational function of the light, which rolls off towards white in place
// of cutting it flat. White itself comes out at 0.80 of white.
vec3 tonemap_aces(vec3 light)
{
    const float a = 2.51;
    const float b = 0.03;
    const float c = 2.43;
    const float d = 0.59;
    const float e = 0.14;
    return clamp((light * (a * light + b)) / (light * (c * light + d) + e), 0.0, 1.0);
}

// AgX in the minimal form of Benjamin Wrensch (2023), after Troy Sobotka:
// the light is mixed a little between its channels, encoded as a log over
// about sixteen and a half stops, and put through a sigmoid that is fitted
// by a polynomial. The mix is undone at the end, and the output is read
// as sRGB encoded and linearized with the 2.2 power, so that the sRGB
// curve below gives the colors of the screen.
vec3 agx_contrast(vec3 x)
{
    vec3 x2 = x * x;
    vec3 x4 = x2 * x2;
    return 15.5 * x4 * x2
         - 40.14 * x4 * x
         + 31.96 * x4
         - 6.868 * x2 * x
         + 0.4298 * x2
         + 0.1191 * x
         - 0.00232;
}

vec3 tonemap_agx(vec3 light)
{
    // the mix in, by columns, as GLSL lays a matrix out
    const mat3 inset = mat3(
        0.842479062253094, 0.0423282422610123, 0.0423756549057051,
        0.0784335999999992, 0.878468636469772, 0.0784336,
        0.0792237451477643, 0.0791661274605434, 0.879142973793104);
    const mat3 outset = mat3(
        1.19687900512017, -0.0528968517574562, -0.0529716355144438,
        -0.0980208811401368, 1.15190312990417, -0.0980434501171241,
        -0.0990297440797205, -0.0989611768448433, 1.15107367264116);
    const float min_ev = -12.47393;
    const float max_ev = 4.026069;

    vec3 value = inset * light;
    // black has no logarithm, so the floor keeps it at the bottom of the range
    value = clamp(log2(max(value, vec3(1e-10))), min_ev, max_ev);
    value = (value - min_ev) / (max_ev - min_ev);
    value = agx_contrast(value);
    value = outset * value;
    return pow(clamp(value, 0.0, 1.0), vec3(2.2));
}

// What a screen can show of the light: the exposure multiplied in, then
// the curve the settings ask for. Without one, what is brighter than
// white is white.
vec3 shown(vec3 light)
{
    light *= resolve.settings.y;
    if (resolve.settings.x == TONEMAPPER_ACES) { return tonemap_aces(light); }
    if (resolve.settings.x == TONEMAPPER_AGX) { return tonemap_agx(light); }
    return clamp(light, 0.0, 1.0);
}

// The curve of the sRGB standard, which the image that is shown holds.
vec3 encode_srgb(vec3 linear)
{
    vec3 low = linear * 12.92;
    vec3 high = 1.055 * pow(linear, vec3(1.0 / 2.4)) - 0.055;
    return mix(low, high, step(vec3(0.0031308), linear));
}

void main()
{
    vec4 light = texelFetch(sampler2D(scene_image, scene_sampler), ivec2(gl_FragCoord.xy), 0);

    // Alpha is multiplied into the light. It is taken out before the curve
    // and put back after it, so that what is drawn on top, and what shows
    // a render target, find the colors with alpha multiplied in.
    float alpha = clamp(light.a, 0.0, 1.0);
    vec3 straight = alpha > 0.0 ? light.rgb / alpha : vec3(0.0);

    frag_color = vec4(encode_srgb(shown(straight)) * alpha, alpha);
}
