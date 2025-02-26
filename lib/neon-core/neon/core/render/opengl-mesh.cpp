#include "opengl-mesh.hpp"

#include <string>

namespace core {
  void OpenGL_Mesh::Initialize()
  {
    glGenVertexArrays(1, &_vao);
    glGenBuffers(1, &_vbo);
    glGenBuffers(1, &_ebo);

    glBindVertexArray(_vao);
    glBindBuffer(GL_ARRAY_BUFFER, _vbo);

    glBufferData(
      GL_ARRAY_BUFFER,
      static_cast<GLsizeiptr>(_vertices.size() * sizeof(Vertex)),
      &_vertices[0],
      GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
      static_cast<GLsizeiptr>(_indices.size() * sizeof(unsigned int)),
      &_indices[0],
      GL_STATIC_DRAW);

    // vertex positions
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), static_cast<void *>(nullptr));

    // vertex normals
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void *>(offsetof(Vertex, normal)));

    // vertex texture coords
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void *>(offsetof(Vertex, tex_coords)));

    glBindVertexArray(0);
  }

  void OpenGL_Mesh::Draw()
  {

  }

  void OpenGL_Mesh::CleanUp() {}
} // core