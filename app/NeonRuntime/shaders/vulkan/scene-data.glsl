// Data every lit shader receives. The layout has to match the structures in
// lib/neon-vulkan/neon/render/vk-shader-data.hpp field for field.

#define MAX_POINT_LIGHTS 64
#define MAX_SPOT_LIGHTS 64

struct DirectionLight {
    vec4 direction;
    vec4 ambient;
    vec4 diffuse;
    vec4 specular;
};

struct PointLight {
    vec4 position;
    vec4 ambient;
    vec4 diffuse;
    vec4 specular;
    // x constant, y linear, z quadratic
    vec4 attenuation;
};

struct SpotLight {
    vec4 position;
    vec4 direction;
    vec4 ambient;
    vec4 diffuse;
    vec4 specular;
    // x constant, y linear, z quadratic
    vec4 attenuation;
    // x cutoff, y outer cutoff
    vec4 cutoff;
};

// what is the same for everything drawn with one camera and one set of lights
layout (std140, set = 0, binding = 0) uniform SceneData {
    mat4 view;
    mat4 projection;
    vec4 view_position;
    DirectionLight direction_light;
    // x number of point lights, y number of spot lights
    ivec4 light_counts;
    PointLight point_lights[MAX_POINT_LIGHTS];
    SpotLight spot_lights[MAX_SPOT_LIGHTS];
} scene;

// what differs from one object to the next
layout (std140, set = 0, binding = 1) uniform ObjectData {
    mat4 model;
    mat4 normal_matrix;
    vec4 color;
    // x and y scale the texture coordinates
    vec4 texture_scale;
    // x shininess, y is 1 when textures are used and 0 when only the color is,
    // z is 1 when the object is see-through and 0 when it is opaque
    vec4 material;
} object;

// The alpha an object writes. An opaque one covers what is behind it
// whatever its alpha says, and writes 1.
float object_alpha(float alpha)
{
    return object.material.z > 0.5 ? alpha : 1.0;
}
