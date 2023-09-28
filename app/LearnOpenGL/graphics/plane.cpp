#include "plane.hpp"

Plane::Plane() {
  _vertices = std::vector({
                              0.5f, 0.5f, 0.0f,  // top right
                              0.5f, -0.5f, 0.0f,  // bottom right
                              -0.5f, -0.5f, 0.0f,  // bottom left
                              -0.5f, 0.5f, 0.0f   // top left
                          });
  _indices = std::vector({
                             0, 1, 3,
                             1, 2, 3
                         });

  Init();
}

Plane::~Plane() {
  CleanUp();
}

void Plane::Draw() const {
  glBindVertexArray(_vao);
  glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
}
