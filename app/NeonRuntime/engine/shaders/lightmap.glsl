// The lightmap of a material: light that was worked out ahead of time and
// kept in a texture, which lies over a mesh by the second set of coordinates
// of its vertices. A fragment shader that reads it includes this after
// scene-data.glsl, and has `object` and `lightmap_coord`.

layout (set = 0, binding = 10) uniform texture2D lightmap_texture;
layout (set = 0, binding = 11) uniform sampler lightmap_sampler;

// Whether the material of the object has a lightmap.
bool has_lightmap(vec4 lightmap)
{
    return lightmap.y > 0.5;
}

// The light of the lightmap at a place, in linear light with its strength
// multiplied in.
vec3 baked_light(vec4 lightmap, vec2 coord)
{
    return texture(sampler2D(lightmap_texture, lightmap_sampler), coord).rgb * lightmap.x;
}
