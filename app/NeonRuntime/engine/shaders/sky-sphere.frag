#version 450
#extension GL_GOOGLE_include_directive : require

// A sky of one panorama around a sphere seen from its middle: an image
// twice as wide as high in which a place across is a direction around,
// and a place down is an angle from straight up.

#include "sky.glsl"

layout (location = 0) in vec2 place;

layout (location = 0) out vec4 frag_color;

// the panorama, and the sampler it is read through, bound apart
layout (set = 0, binding = 0) uniform texture2D sky_texture;
layout (set = 0, binding = 1) uniform sampler sky_sampler;

const float pi = 3.14159265358979;

void main()
{
    vec3 direction = sky_direction(place);

    // the middle of the image is seen along negative z, and what is right
    // of it towards positive x
    float across = atan(direction.x, -direction.z) / (2.0 * pi) + 0.5;
    float down = acos(clamp(direction.y, -1.0, 1.0)) / pi;

    // The image starts again past its left and right edge, but not past
    // its top and bottom: half a pixel is kept from them, so that straight
    // up is not blended with straight down.
    float rows = float(textureSize(sampler2D(sky_texture, sky_sampler), 0).y);
    down = clamp(down, 0.5 / rows, 1.0 - 0.5 / rows);

    // Read without smaller copies, which is what an explicit level says:
    // where the image starts again `across` jumps from 1 to 0 between two
    // pixels, and a level picked from that jump would draw a seam.
    vec3 light = textureLod(sampler2D(sky_texture, sky_sampler), vec2(across, down), 0.0).rgb;

    frag_color = vec4(light * sky.settings.x, 1.0);
}
