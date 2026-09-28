#version 450
#extension GL_GOOGLE_include_directive : require

#include "scene-data.glsl"

layout (location = 0) in vec3 frag_coord;
layout (location = 1) in vec3 normal_coord;
layout (location = 2) in vec2 tex_coord;

layout (location = 0) out vec4 frag_color;

layout (set = 0, binding = 2) uniform sampler2D diffuse_texture;
layout (set = 0, binding = 3) uniform sampler2D specular_texture;

vec3 GetDiffuseColor()
{
    vec3 texture_color = texture(diffuse_texture, tex_coord).rgb;
    return mix(object.color.rgb, texture_color, object.material.y);
}

vec3 GetSpecularColor()
{
    vec3 texture_color = texture(specular_texture, tex_coord).rgb;
    return mix(vec3(0.0), texture_color, object.material.y);
}

// the part every kind of light shares, given the direction towards the light
vec3 Shade(vec3 light_dir, vec3 normal, vec3 view_dir, vec3 ambient, vec3 diffuse, vec3 specular, float strength)
{
    float diff = max(dot(normal, light_dir), 0.0);

    vec3 reflect_dir = reflect(-light_dir, normal);
    float spec = pow(max(dot(view_dir, reflect_dir), 0.0), object.material.x);

    vec3 diffuse_sample = GetDiffuseColor();
    vec3 specular_sample = GetSpecularColor();

    return strength * (
        ambient * diffuse_sample +
        diffuse * diff * diffuse_sample +
        specular * spec * specular_sample);
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
        1.0);

    for (int i = 0; i < scene.light_counts.x; i++) {
        PointLight light = scene.point_lights[i];
        result += Shade(
            normalize(light.position.xyz - frag_coord),
            normal,
            view_dir,
            light.ambient.rgb,
            light.diffuse.rgb,
            light.specular.rgb,
            Attenuation(light.attenuation, light.position.xyz));
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
            Attenuation(light.attenuation, light.position.xyz) * intensity);
    }

    frag_color = object.color * vec4(result, 1.0);
}
