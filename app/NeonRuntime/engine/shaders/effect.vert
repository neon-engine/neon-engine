#version 450

// One triangle that covers what is drawn to, for an effect of a camera: a
// shader a game brings that is run over the finished picture. Its corners
// are made from their number, so it needs no vertex buffer. The effect is
// handed where each pixel is in the picture, see effect.glsl.

layout (location = 0) out vec2 frame_coord;

void main()
{
    vec2 corner = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
    frame_coord = corner;
    gl_Position = vec4(corner * 2.0 - 1.0, 0.0, 1.0);
}
