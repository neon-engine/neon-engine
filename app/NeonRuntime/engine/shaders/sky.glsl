// What the shaders of a sky are told about a draw, and how a pixel becomes
// the direction it is seen in. Field for field what VK_SkyValues holds.

layout (push_constant) uniform Sky
{
    // a place at the far end of the screen to the direction it is seen in,
    // in the space of the sky
    mat4 to_sky;
    // x: what the light of the images is multiplied by
    vec4 settings;
} sky;

// The direction a place on the screen is seen in, from -1 to 1 across and
// upward. Worked out for every pixel, since a direction that is made a
// length of 1 at the corners does not blend into one in between.
vec3 sky_direction(vec2 place)
{
    vec4 far = sky.to_sky * vec4(place, 1.0, 1.0);
    return normalize(far.xyz / far.w);
}
