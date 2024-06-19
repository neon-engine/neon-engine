#include "plane.hpp"

Plane::Plane(): Geometry("Plane")
{
    _vertices = std::vector({
        // positions         // colors            // texture coords
        // (x, y, z)         // (r, g, b)         // (s, t)
        00.5f, 00.5f, 00.0f, 01.0f, 00.0f, 00.0f, 01.0f, 01.0f, // top right
        00.5f, -0.5f, 00.0f, 00.0f, 01.0f, 00.0f, 01.0f, 00.0f, // bottom right
        -0.5f, -0.5f, 00.0f, 00.0f, 00.0f, 01.0f, 00.0f, 00.0f, // bottom left
        -0.5f, 00.5f, 00.0f, 01.0f, 01.0f, 00.0f, 00.0f, 01.0f  // top left
    });
    _indices = std::vector({
        0, 1, 3,
        1, 2, 3
    });

    Init();
}

Plane::~Plane()
{
    CleanUp();
}

void Plane::Draw() const
{
    glBindVertexArray(_vao);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
}
