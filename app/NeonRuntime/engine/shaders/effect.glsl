// What an effect of a camera starts with: a fragment shader a game brings
// that is run over the whole picture the camera drew. It reads the picture
// and writes what is shown in its place.
//
//     #version 450
//     #extension GL_GOOGLE_include_directive : require
//     #include "effect.glsl"
//
//     void main()
//     {
//         frag_color = read_frame(frame_coord + vec2(sin(scene.time.x) * 0.01, 0.0));
//     }
//
// A camera names its effects in two lists. Those of `effects` are run on
// the light of the scene, before the tonemapper: the picture holds linear
// light, with room above white. Those of `screen_effects` are run on the
// colors a screen is given, after the tonemapper and before the user
// interface is drawn. Either way the alpha of the picture is multiplied
// into its colors, and an effect hands on what it does not change.
//
// The layout has to match VK_Effects, field for field.

// the picture as it is before this effect, and the sampler it is read
// through: smooth, and its edge is drawn on past it
layout (set = 0, binding = 0) uniform texture2D frame_image;
layout (set = 0, binding = 1) uniform sampler frame_sampler;

#define SHADER_NUMBER_PLACES 8

// What the shaders of a material read as `scene.time` and `scene.numbers`,
// under the same names: x the seconds the world has run, y how long its
// last frame took; and the numbers of the game, see
// RenderContext::SetShaderNumbers().
layout (std140, set = 0, binding = 2) uniform FrameData {
    vec4 time;
    vec4 numbers[SHADER_NUMBER_PLACES];
} scene;

// where the pixel is in the picture: 0, 0 at the top left and 1, 1 at the
// bottom right
layout (location = 0) in vec2 frame_coord;

layout (location = 0) out vec4 frag_color;

// The picture at a place of it, between its pixels where the place is.
vec4 read_frame(vec2 at)
{
    return texture(sampler2D(frame_image, frame_sampler), at);
}

// The size of the picture in pixels.
vec2 frame_size()
{
    return vec2(textureSize(sampler2D(frame_image, frame_sampler), 0));
}
