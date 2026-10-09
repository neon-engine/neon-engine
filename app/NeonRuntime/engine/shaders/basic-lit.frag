#version 450
#extension GL_GOOGLE_include_directive : require

#define object objects[object_index]
#include "scene-data.glsl"
#include "shadows.glsl"

layout (location = 0) in vec3 frag_coord;
layout (location = 1) in vec3 normal_coord;
layout (location = 2) in vec2 tex_coord;
// the color painted on the vertices, in linear light; white where a
// model has none
layout (location = 3) in vec4 vertex_color;
layout (location = 4) flat in uint object_index;
layout (location = 5) in vec2 lightmap_coord;

layout (location = 0) out vec4 frag_color;

// Every texture is read through a sampler bound next to it, three bindings
// on: the one the texture asks for, shared with all that are read alike.
// The third texture is what the surface gives off, plain white when the
// material has none.
layout (set = 0, binding = 2) uniform texture2D diffuse_texture;
layout (set = 0, binding = 3) uniform texture2D specular_texture;
layout (set = 0, binding = 4) uniform texture2D emissive_texture;
layout (set = 0, binding = 5) uniform sampler diffuse_sampler;
layout (set = 0, binding = 6) uniform sampler specular_sampler;
layout (set = 0, binding = 7) uniform sampler emissive_sampler;

#include "lightmap.glsl"

vec3 GetDiffuseColor()
{
    vec3 texture_color = texture(sampler2D(diffuse_texture, diffuse_sampler), tex_coord).rgb;
    return vertex_color.rgb * mix(object.color.rgb, texture_color, object.material.y);
}

vec3 GetSpecularColor()
{
    vec3 texture_color = texture(sampler2D(specular_texture, specular_sampler), tex_coord).rgb;
    return mix(vec3(0.0), texture_color, object.material.y);
}

// The light the surface gives off itself: the emissive color, with its
// strength multiplied in, times the emissive texture where there is one.
// Added after lighting, so that it shows in the dark.
vec3 GetEmissive()
{
    vec3 texture_color = texture(sampler2D(emissive_texture, emissive_sampler), tex_coord).rgb;
    return object.emissive.rgb * mix(vec3(1.0), texture_color, object.emissive.w);
}

// The part every kind of light shares, given the direction towards the
// light. `visibility` is how much of the light reaches the point, which a
// shadow takes away from the diffuse and the specular light, never from
// the ambient.
vec3 Shade(vec3 light_dir, vec3 normal, vec3 view_dir, vec3 ambient, vec3 diffuse, vec3 specular, float strength, float visibility)
{
    float diff = max(dot(normal, light_dir), 0.0);

    // A material without shininess has no highlight. It also must not reach
    // pow(), since 0 to the power of 0 is not defined and turns the whole
    // fragment black on some graphics cards.
    float spec = 0.0;
    if (object.material.x > 0.0) {
        vec3 reflect_dir = reflect(-light_dir, normal);
        spec = pow(max(dot(view_dir, reflect_dir), 0.0), object.material.x);
    }

    vec3 diffuse_sample = GetDiffuseColor();
    vec3 specular_sample = GetSpecularColor();

    return strength * (
        ambient * diffuse_sample +
        visibility * diffuse * diff * diffuse_sample +
        visibility * specular * spec * specular_sample);
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

    vec3 result = Shade(
        normalize(-scene.direction_light.direction.xyz),
        normal,
        view_dir,
        scene.direction_light.ambient.rgb,
        scene.direction_light.diffuse.rgb,
        scene.direction_light.specular.rgb,
        1.0,
        direction_light_visibility(frag_coord));

    for (int i = 0; i < scene.light_counts.x; i++) {
        PointLight light = scene.point_lights[i];
        result += Shade(
            normalize(light.position.xyz - frag_coord),
            normal,
            view_dir,
            light.ambient.rgb,
            light.diffuse.rgb,
            light.specular.rgb,
            Attenuation(light.attenuation, light.position.xyz),
            1.0);
    }

    for (int i = 0; i < scene.light_counts.y; i++) {
        SpotLight light = scene.spot_lights[i];
        vec3 light_dir = normalize(light.position.xyz - frag_coord);

        float theta = dot(light_dir, normalize(-light.direction.xyz));
        float epsilon = light.cutoff.x - light.cutoff.y;
        float intensity = clamp((theta - light.cutoff.y) / epsilon, 0.0, 1.0);

        result += Shade(
            light_dir,
            normal,
            view_dir,
            light.ambient.rgb,
            light.diffuse.rgb,
            light.specular.rgb,
            Attenuation(light.attenuation, light.position.xyz) * intensity,
            1.0);
    }

    // light that was worked out ahead falls on the surface next to the light
    // of the lights
    if (has_lightmap(object.lightmap)) { result += GetDiffuseColor() * baked_light(object.lightmap, lightmap_coord); }

    frag_color = vec4(object.color.rgb * result + GetEmissive(), object_alpha(object.material, object.color.a));
}
