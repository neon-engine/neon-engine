// How much of the direction light reaches a point, read from its shadow
// map. For the fragment half of a lit shader, after scene-data.glsl: the
// map and the sampler it is compared through are bound after the textures
// of the material, apart, as every texture is. The sampler compares: a
// depth is lit when it is no further than what the map holds, blended
// between the four texels around the place, and lit past the edge of the
// map.

layout (set = 0, binding = 6) uniform texture2D shadow_map;
layout (set = 0, binding = 7) uniform samplerShadow shadow_sampler;

// 1 where the direction light reaches `world_position`, 0 where something
// stands between, and in between along the edge of a shadow: the nine
// texels around the place are compared and averaged, each of which the
// sampler blends with its neighbours already.
float direction_light_visibility(vec3 world_position)
{
    if (scene.direction_light.shadow.x < 0.5) { return 1.0; }

    vec4 in_light = scene.direction_light.light_view_projection * vec4(world_position, 1.0);
    vec3 place = in_light.xyz / in_light.w;

    // what is further from the light than the map reaches is lit
    if (place.z > 1.0) { return 1.0; }

    // Across the map from -1 to 1 to 0 to 1, with the rows turned round,
    // since the map was drawn with the viewport of the scene, which turns
    // the picture the right way up. The depth is moved a little towards
    // the light, so that a surface does not shadow itself.
    vec2 texel = vec2(scene.direction_light.shadow.y);
    vec3 compared = vec3(0.5 + 0.5 * place.x, 0.5 - 0.5 * place.y, place.z - scene.direction_light.shadow.z);

    float lit = 0.0;
    for (int x = -1; x <= 1; x++) {
        for (int y = -1; y <= 1; y++) {
            lit += texture(sampler2DShadow(shadow_map, shadow_sampler), compared + vec3(vec2(x, y) * texel, 0.0));
        }
    }
    return lit / 9.0;
}
