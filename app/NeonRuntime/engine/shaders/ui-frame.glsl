// What both halves of a shader of the user interface are told about the
// call they draw. It has to match VK_Renderer2D::Frame.

layout (push_constant) uniform Frame
{
    // the size in pixels of what is drawn to
    vec2 size;

    // added to the place of every corner, in pixels
    vec2 translation;

    // the box with round corners nothing is drawn outside of: its middle
    // and half its size in pixels, and the radius of its corners, left top,
    // right top, right bottom, left bottom
    vec4 clip_box;
    vec4 clip_radii;

    // seconds since the user interface was started, the place of the first
    // shape of the call among all shapes of the frame, 1 when `clip_box`
    // applies, and nothing
    vec4 state;

    // the box of the element a shader of its own is drawn for: its left
    // top corner and its size, in pixels
    vec4 element;
} frame;
