#version 450

// The shadow map holds depth alone, which the pipeline writes, so there is
// nothing to do here. The half exists because a shader is loaded in two
// halves, and every target of the sources takes a pair.

void main() {}
