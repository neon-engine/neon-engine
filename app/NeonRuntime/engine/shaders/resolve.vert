#version 450

// One triangle that covers what is drawn to, for the resolve step. Its
// corners are made from their number, so it needs no vertex buffer.

void main()
{
    vec2 corner = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
    gl_Position = vec4(corner * 2.0 - 1.0, 0.0, 1.0);
}
