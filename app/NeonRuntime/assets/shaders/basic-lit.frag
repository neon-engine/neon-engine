#version 330 core

out vec4 FragColor;

struct Material {
    sampler2D diffuse;
    sampler2D specular;
    float shininess;
};

struct DirectionLight {
    vec3 direction;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

struct PointLight {
    vec3 position;

    float constant;
    float linear;
    float quadratic;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

struct SpotLight {
    vec3 position;
    vec3 direction;
    float cutoff;
    float outer_cutoff;

    float constant;
    float linear;
    float quadratic;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

#define NUM_POINT_LIGHTS 128
#define NUM_SPOT_LIGHTS 128

in vec3 frag_coord;
in vec3 normal_coord;
in vec2 tex_coord;

uniform vec3 view_position;
uniform DirectionLight direction_light;
uniform PointLight point_lights[NUM_POINT_LIGHTS];
uniform int num_point_lights;
uniform SpotLight spot_lights[NUM_SPOT_LIGHTS];
uniform int num_spot_lights;
uniform Material material;
uniform vec4 color;
uniform bool use_tex_coords;

float is_color_mode = float(!use_tex_coords);

vec3 GetDiffuseColor();
vec3 GetSpecularColor();
vec3 CalcDirectionLight(DirectionLight light, vec3 normal, vec3 view_direction);
vec3 CalcPointLight(PointLight light, vec3 normal, vec3 frag_pos, vec3 view_dir);
vec3 CalcSpotLight(SpotLight light, vec3 normal, vec3 frag_pos, vec3 view_dir);

void main()
{
    vec3 norm = normalize(normal_coord);
    vec3 view_dir = normalize(view_position - frag_coord);

    vec3 result = CalcDirectionLight(direction_light, norm, view_dir);

    for (int i = 0; i < num_point_lights; i++) {
        result += CalcPointLight(point_lights[i], norm, frag_coord, view_dir);
    }

    for (int i = 0; i < num_spot_lights; i++) {
        result += CalcSpotLight(spot_lights[i], norm, frag_coord, view_dir);
    }

    FragColor = color * vec4(result, 1.0);
}

vec3 GetDiffuseColor()
{
    vec3 texture_color = texture(material.diffuse, tex_coord * vec2(float(use_tex_coords))).rgb;
    return mix(texture_color, color.rgb, is_color_mode);
}

vec3 GetSpecularColor()
{
    vec3 texture_color = texture(material.specular, tex_coord * vec2(float(use_tex_coords))).rgb;
    return mix(texture_color, vec3(0.0), is_color_mode);
}

vec3 CalcDirectionLight(DirectionLight light, vec3 normal, vec3 viewDir)
{
    vec3 lightDir = normalize(-light.direction);
    // diffuse shading
    float diff = max(dot(normal, lightDir), 0.0);
    // specular shading
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);

    vec3 diffuse_sample = GetDiffuseColor();
    vec3 specular_sample = GetSpecularColor();

    // combine results
    vec3 ambient = light.ambient * diffuse_sample;
    vec3 diffuse = light.diffuse * diff * diffuse_sample;
    vec3 specular = light.specular * spec * specular_sample;
    return (ambient + diffuse + specular);
}

// calculates the color when using a point light.
vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir)
{
    vec3 lightDir = normalize(light.position - fragPos);
    // diffuse shading
    float diff = max(dot(normal, lightDir), 0.0);
    // specular shading
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    // attenuation
    float distance = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));

    vec3 diffuse_sample = GetDiffuseColor();
    vec3 specular_sample = GetSpecularColor();

    // combine results
    vec3 ambient = light.ambient * diffuse_sample;
    vec3 diffuse = light.diffuse * diff * diffuse_sample;
    vec3 specular = light.specular * spec * specular_sample;
    ambient *= attenuation;
    diffuse *= attenuation;
    specular *= attenuation;
    return (ambient + diffuse + specular);
}

// calculates the color when using a spot light.
vec3 CalcSpotLight(SpotLight light, vec3 normal, vec3 fragPos, vec3 viewDir)
{
    vec3 lightDir = normalize(light.position - fragPos);
    // diffuse shading
    float diff = max(dot(normal, lightDir), 0.0);
    // specular shading
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    // attenuation
    float distance = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));
    // spotlight intensity
    float theta = dot(lightDir, normalize(-light.direction));
    float epsilon = light.cutoff - light.outer_cutoff;
    float intensity = clamp((theta - light.outer_cutoff) / epsilon, 0.0, 1.0);

    vec3 diffuse_sample = GetDiffuseColor();
    vec3 specular_sample = GetSpecularColor();

    // combine results
    vec3 ambient = light.ambient * diffuse_sample;
    vec3 diffuse = light.diffuse * diff * diffuse_sample;
    vec3 specular = light.specular * spec * specular_sample;
    ambient *= attenuation * intensity;
    diffuse *= attenuation * intensity;
    specular *= attenuation * intensity;
    return (ambient + diffuse + specular);
}