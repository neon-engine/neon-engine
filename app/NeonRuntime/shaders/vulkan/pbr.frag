#version 450
#extension GL_GOOGLE_include_directive : require

#include "scene-data.glsl"

// Physically based shading, with the metallic-roughness model of glTF:
// Lambert for the diffuse light and Cook-Torrance for the specular, with the
// GGX distribution, the Smith-GGX geometry term as Schlick approximates it,
// and Schlick's Fresnel. The light's `diffuse` is its radiance: the colour a
// white, rough, non-metal surface facing it shows. Its `ambient` lights the
// diffuse colour evenly. Its `specular` is not read; a surface's highlight
// follows from its roughness and its metalness, not from the light.

layout (location = 0) in vec3 frag_coord;
layout (location = 1) in vec3 normal_coord;
layout (location = 2) in vec2 tex_coord;

layout (location = 0) out vec4 frag_color;

// the base colour, in sRGB as an image keeps it, read as linear light
layout (set = 0, binding = 2) uniform sampler2D base_color_texture;

// as glTF lays it out: green is roughness, blue is metallic, both multiply
// the material's numbers. Read as numbers, not colours.
layout (set = 0, binding = 3) uniform sampler2D metallic_roughness_texture;

const float PI = 3.14159265359;

// What a dielectric reflects when looked at straight on, the same for nearly
// all of them: about 4 percent.
const vec3 DIELECTRIC_F0 = vec3(0.04);

vec3 BaseColor()
{
    vec3 texture_color = texture(base_color_texture, tex_coord).rgb;
    return object.color.rgb * mix(vec3(1.0), texture_color, object.material.y);
}

vec2 MetallicRoughness()
{
    vec2 from_texture = texture(metallic_roughness_texture, tex_coord).bg;
    vec2 factors = mix(vec2(1.0), from_texture, object.material.y);
    float metallic = clamp(object.surface.x * factors.x, 0.0, 1.0);

    // A roughness of 0 makes the distribution a spike that no pixel hits
    // and divides by zero at the end. The floor is what a polished mirror
    // comes to in this model.
    float roughness = clamp(object.surface.y * factors.y, 0.045, 1.0);
    return vec2(metallic, roughness);
}

// GGX, Trowbridge-Reitz: how many of the surface's microfacets face the
// half vector, with alpha the roughness squared as Disney and glTF have it.
float DistributionGGX(float n_dot_h, float roughness)
{
    float alpha = roughness * roughness;
    float alpha2 = alpha * alpha;
    float denominator = n_dot_h * n_dot_h * (alpha2 - 1.0) + 1.0;
    return alpha2 / (PI * denominator * denominator);
}

// Smith's geometry term with Schlick's approximation of GGX: how many
// microfacets are shadowed or masked, from the light and from the eye.
float GeometrySchlickGGX(float n_dot_x, float roughness)
{
    float alpha = roughness * roughness;
    float k = alpha / 2.0;
    return n_dot_x / (n_dot_x * (1.0 - k) + k);
}

float GeometrySmith(float n_dot_v, float n_dot_l, float roughness)
{
    return GeometrySchlickGGX(n_dot_v, roughness) * GeometrySchlickGGX(n_dot_l, roughness);
}

// Schlick's Fresnel: more light is reflected the more grazing the view
vec3 FresnelSchlick(float v_dot_h, vec3 f0)
{
    return f0 + (1.0 - f0) * pow(clamp(1.0 - v_dot_h, 0.0, 1.0), 5.0);
}

// The light one source adds, given the direction towards it and its
// radiance at this point.
vec3 Shade(vec3 light_dir, vec3 normal, vec3 view_dir, vec3 radiance, vec3 base_color, float metallic, float roughness)
{
    float n_dot_l = max(dot(normal, light_dir), 0.0);
    if (n_dot_l <= 0.0) { return vec3(0.0); }

    vec3 half_dir = normalize(view_dir + light_dir);
    float n_dot_v = max(dot(normal, view_dir), 0.0001);
    float n_dot_h = max(dot(normal, half_dir), 0.0);
    float v_dot_h = max(dot(view_dir, half_dir), 0.0);

    // a metal reflects in its own colour, a dielectric in white
    vec3 f0 = mix(DIELECTRIC_F0, base_color, metallic);
    vec3 fresnel = FresnelSchlick(v_dot_h, f0);

    float distribution = DistributionGGX(n_dot_h, roughness);
    float geometry = GeometrySmith(n_dot_v, n_dot_l, roughness);
    vec3 specular = (distribution * geometry * fresnel) / (4.0 * n_dot_v * n_dot_l);

    // what is reflected is not scattered, and a metal scatters nothing
    vec3 diffuse = (vec3(1.0) - fresnel) * (1.0 - metallic) * base_color;

    return (diffuse + specular) * radiance * n_dot_l;
}

float Attenuation(vec4 attenuation, vec3 light_position)
{
    float distance = length(light_position - frag_coord);
    return 1.0 / (attenuation.x + attenuation.y * distance + attenuation.z * (distance * distance));
}

void main()
{
    vec3 normal = normalize(normal_coord);
    vec3 view_dir = normalize(scene.view_position.xyz - frag_coord);

    vec3 base_color = BaseColor();
    vec2 metallic_roughness = MetallicRoughness();
    float metallic = metallic_roughness.x;
    float roughness = metallic_roughness.y;

    // the ambient light reaches the diffuse colour alone, from every side
    vec3 result = scene.direction_light.ambient.rgb * base_color * (1.0 - metallic);

    result += Shade(
        normalize(-scene.direction_light.direction.xyz),
        normal,
        view_dir,
        scene.direction_light.diffuse.rgb,
        base_color,
        metallic,
        roughness);

    for (int i = 0; i < scene.light_counts.x; i++) {
        PointLight light = scene.point_lights[i];
        float strength = Attenuation(light.attenuation, light.position.xyz);
        result += light.ambient.rgb * base_color * (1.0 - metallic) * strength;
        result += Shade(
            normalize(light.position.xyz - frag_coord),
            normal,
            view_dir,
            light.diffuse.rgb * strength,
            base_color,
            metallic,
            roughness);
    }

    for (int i = 0; i < scene.light_counts.y; i++) {
        SpotLight light = scene.spot_lights[i];
        vec3 light_dir = normalize(light.position.xyz - frag_coord);

        float theta = dot(light_dir, normalize(-light.direction.xyz));
        float epsilon = light.cutoff.x - light.cutoff.y;
        float intensity = clamp((theta - light.cutoff.y) / epsilon, 0.0, 1.0);
        float strength = Attenuation(light.attenuation, light.position.xyz) * intensity;

        result += light.ambient.rgb * base_color * (1.0 - metallic) * strength;
        result += Shade(
            light_dir,
            normal,
            view_dir,
            light.diffuse.rgb * strength,
            base_color,
            metallic,
            roughness);
    }

    frag_color = vec4(result, object_alpha(object.color.a));
}
