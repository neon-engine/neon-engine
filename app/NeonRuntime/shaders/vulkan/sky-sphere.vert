#version 450

// One triangle that covers what is drawn to, at the far end of the depth,
// so that it shows wherever no model was drawn. Its corners are made from
// their number, so it needs no vertex buffer.

// where the pixel is on the screen, from -1 to 1 across and upward
layout (location = 0) out vec2 place;

void main()
{
    vec2 corner = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
    place = corner * 2.0 - 1.0;
    gl_Position = vec4(place, 1.0, 1.0);
}
