#version 450
#extension GL_GOOGLE_include_directive : require

// Draws what a user interface is made of: rectangles with a colour or a
// texture, boxes with round corners, their borders and shadows, gradients,
// and text. What each of them looks like is in ui-shader.glsl, which the
// shaders of elements share.

#include "ui-shader.glsl"

void main()
{
    frag_color = ui_base();
}
