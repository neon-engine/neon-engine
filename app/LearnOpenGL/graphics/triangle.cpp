#include "triangle.hpp"

Triangle::Triangle(): Geometry("Triangle")
{
    _vertices = std::vector({
        -0.5f, -0.5f, 0.0f,
        0.5f, -0.5f, 0.0f,
        0.0f, 0.5f, 0.0f
    });
    _indices = std::vector({
        2, 1, 0
    });

    Init();
}

Triangle::~Triangle()
{
    CleanUp();
}

void Triangle::Draw() const
{
    glBindVertexArray(_vao);
    glDrawElements(GL_TRIANGLES, 3, GL_UNSIGNED_INT, nullptr);
}
