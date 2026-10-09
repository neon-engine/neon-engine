#version 450
#extension GL_GOOGLE_include_directive : require

// Shows what a shader is told that no object carries. The left half of the
// square is the color of the numbers at place 3, as the extension set
// them. The right half says whether the time runs: green when the seconds
// the world has run and the length of its last frame are both above zero,
// red when not.

#include "scene-data.glsl"

layout (location = 0) in vec2 tex_coord;

layout (location = 0) out vec4 frag_color;

void main()
{
    if (tex_coord.x < 0.5)
    {
        frag_color = vec4(scene.numbers[3].rgb, 1.0);
        return;
    }

    bool runs = scene.time.x > 0.0 && scene.time.y > 0.0;
    frag_color = runs ? vec4(0.0, 1.0, 0.0, 1.0) : vec4(1.0, 0.0, 0.0, 1.0);
}
