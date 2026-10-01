#version 450

// The resolve step: the one place where the linear light of the scene
// becomes the colours of the image that is shown. Light that bleeds around
// what is bright is added to the scene image before this, and a curve for
// light brighter than white replaces shown() below.

layout (set = 0, binding = 0) uniform sampler2D scene_image;

layout (location = 0) out vec4 frag_color;

// What a screen can show of the light. For now what is brighter than white
// is white.
vec3 shown(vec3 light)
{
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
    vec4 light = texelFetch(scene_image, ivec2(gl_FragCoord.xy), 0);

    // Alpha is multiplied into the light. It is taken out before the curve
    // and put back after it, so that what is drawn on top, and what shows
    // a render target, find the colours with alpha multiplied in.
    float alpha = clamp(light.a, 0.0, 1.0);
    vec3 straight = alpha > 0.0 ? light.rgb / alpha : vec3(0.0);

    frag_color = vec4(encode_srgb(shown(straight)) * alpha, alpha);
}
