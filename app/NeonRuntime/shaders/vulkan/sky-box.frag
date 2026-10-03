#version 450
#extension GL_GOOGLE_include_directive : require

// A sky of six images, the faces of a cube seen from its middle.

#include "sky.glsl"

layout (location = 0) in vec2 place;

layout (location = 0) out vec4 frag_color;

// the six faces, and the sampler they are read through, bound apart
layout (set = 0, binding = 0) uniform textureCube sky_texture;
layout (set = 0, binding = 1) uniform sampler sky_sampler;

void main()
{
    vec3 direction = sky_direction(place);

    // A cube counts z the other way round than the world does. Read with z
    // turned round, the faces are seen from the inside as they were
    // painted: `front` along negative z, with `right` to its right.
    vec3 light = textureLod(
        samplerCube(sky_texture, sky_sampler), vec3(direction.x, direction.y, -direction.z), 0.0).rgb;

    frag_color = vec4(light * sky.settings.x, 1.0);
}
