// How much of the direction light reaches a point, read from its shadow
// map. For the fragment half of a lit shader, after scene-data.glsl: the
// map and the sampler it is compared through are bound after the textures
// of the material, apart, as every texture is. The sampler compares: a
// depth is lit when it is no further than what the map holds, blended
// between the four texels around the place, and lit past the edge of the
// map.
//
// The map is cascaded: one layer for each slice of the camera's view, the
// nearest slice drawn the finest. A point picks the first cascade whose
// reach it is within, by its distance along the camera's view, so that
// the shadows near the camera come from the finest layer.

layout (set = 0, binding = 8) uniform texture2DArray shadow_map;
layout (set = 0, binding = 9) uniform samplerShadow shadow_sampler;

// 1 where the direction light reaches `world_position`, 0 where something
// stands between, and in between along the edge of a shadow: with the
// filter the nine texels around the place are compared and averaged, each
// of which the sampler blends with its neighbors already; without it the
// place alone is compared, which the sampler blends the same.
float direction_light_visibility(vec3 world_position)
{
    if (scene.direction_light.shadow.x < 0.5) { return 1.0; }
    int taps = int(scene.direction_light.shadow.x);
    int reach = taps / 2;

    // how far along the view the point is, which picks the cascade; past
    // the last one the point is lit
    float depth = -(scene.view * vec4(world_position, 1.0)).z;
    int count = int(scene.direction_light.shadow.w);
    int cascade = 0;
    while (cascade < count - 1 && depth > scene.direction_light.splits[cascade]) { cascade++; }
    if (depth > scene.direction_light.splits[count - 1]) { return 1.0; }

    vec4 in_light = scene.direction_light.cascades[cascade] * vec4(world_position, 1.0);
    vec3 place = in_light.xyz / in_light.w;

    // what is further from the light than the cascade reaches is lit
    if (place.z > 1.0) { return 1.0; }

    // Across the map from -1 to 1 to 0 to 1, with the rows turned round,
    // since the map was drawn with the viewport of the scene, which turns
    // the picture the right way up. The depth is moved a little towards
    // the light, so that a surface does not shadow itself.
    vec2 texel = vec2(scene.direction_light.shadow.y);
    vec3 compared = vec3(0.5 + 0.5 * place.x, 0.5 - 0.5 * place.y, place.z - scene.direction_light.shadow.z);

    float lit = 0.0;
    for (int x = -reach; x <= reach; x++) {
        for (int y = -reach; y <= reach; y++) {
            vec2 at = compared.xy + vec2(x, y) * texel;
            lit += texture(sampler2DArrayShadow(shadow_map, shadow_sampler), vec4(at, float(cascade), compared.z));
        }
    }
    return lit / float(taps * taps);
}
