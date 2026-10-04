// Data every lit shader receives. The layout has to match the structures in
// lib/neon-vulkan/neon/render/vk-shader-data.hpp field for field.

#define MAX_POINT_LIGHTS 64
#define MAX_SPOT_LIGHTS 64
#define SHADER_NUMBER_PLACES 8
#define MAX_SHADOW_CASCADES 4

struct DirectionLight {
    vec4 direction;
    vec4 ambient;
    vec4 diffuse;
    vec4 specular;
    // where the shadow map looks, one matrix a cascade: the world as the
    // light sees it, with the depth from 0 to 1 as Vulkan has it
    mat4 cascades[MAX_SHADOW_CASCADES];
    // how far from the camera each cascade reaches, along its view
    vec4 splits;
    // x is 1 when the shadow map is to be compared against and 0 when the
    // light casts no shadow, y the size of a texel of the map across it,
    // z the bias a depth is moved towards the light by before it is
    // compared, w how many cascades there are, see shadows.glsl
    vec4 shadow;
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
    // x the seconds the world has run, y how long its last frame took. The
    // time stands still while the world does.
    vec4 time;
    // Numbers of a game, which the shaders it brings read: how thick its
    // fog is, a colour, whatever they are written for. The shaders of the
    // engine read none. Set with RenderContext::SetShaderNumbers(), and by
    // an extension with set_shader_numbers.
    vec4 numbers[SHADER_NUMBER_PLACES];
} scene;

// what differs from one object to the next
struct ObjectData {
    mat4 model;
    mat4 normal_matrix;
    vec4 color;
    // x and y scale the texture coordinates
    vec4 texture_scale;
    // x shininess, y is 1 when textures are used and 0 when only the color is,
    // z is 1 when the object is see-through and 0 when it is opaque
    vec4 material;
    // x metallic, y roughness, for the pbr shader
    vec4 surface;
    // rgb the light the object gives off itself, in linear light with its
    // strength multiplied in; w is 1 when an emissive texture is bound
    // and 0 when the colour alone glows
    vec4 emissive;
    // x what the lightmap is multiplied by, y is 1 when a lightmap is bound
    // and 0 when the material has none
    vec4 lightmap;
};

// Every object of the frame, side by side. A draw names its first with
// gl_InstanceIndex, and objects drawn as one call follow it. A shader
// says which one is `object` before it includes this: the vertex half
// by gl_InstanceIndex, the fragment half by the index the vertex half
// hands it.
layout (std430, set = 0, binding = 1) readonly buffer ObjectBuffer {
    ObjectData objects[];
};

// The alpha an object writes. An opaque one covers what is behind it
// whatever its alpha says, and writes 1. The pipeline of an opaque material
// keeps the alpha of the scene image at 1 as well, over an opaque clear.
// This is for a texture a camera clears to a see-through colour, where the
// pipeline cannot, so a shader of its own is well advised to use it.
float object_alpha(vec4 material, float alpha)
{
    return material.z > 0.5 ? alpha : 1.0;
}
