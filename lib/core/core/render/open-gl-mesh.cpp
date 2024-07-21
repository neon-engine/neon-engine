#include "open-gl-mesh.hpp"

#include <iostream>

namespace core {
  OpenGL_Mesh::OpenGL_Mesh() = default;

  OpenGL_Mesh::OpenGL_Mesh(const std::vector<float> &vertices,
                           const std::vector<float> &normals,
                           const std::vector<float> &uvs,
                           const std::vector<int> &indices)
  {
    _vertices = vertices;
    _normals = normals;
    _uvs = uvs;
    _indices = indices;
  }

  OpenGL_Mesh::~OpenGL_Mesh()
  {
    if (!_initialized) { return; }

    glDeleteBuffers(1, &_vao);
    glDeleteBuffers(1, &_vbo);
    glDeleteBuffers(1, &_nvbo);
    glDeleteBuffers(1, &_uvbo);
    glDeleteBuffers(1, &_ebo);
    _initialized = false;
  }

  // ReSharper disable once CppDFAConstantFunctionResult
  bool OpenGL_Mesh::Initialize()
  {
    if (_initialized)
    {
      std::cerr << "Mesh with opengl id " << _vao << " already initialized" << std::endl;
      return true;
    }

    _vao = 0;
    _vbo = 0;
    _nvbo = 0;
    _uvbo = 0;
    _ebo = 0;

    glGenVertexArrays(1, &_vao);
    glGenBuffers(1, &_vbo);
    glGenBuffers(1, &_nvbo);
    glGenBuffers(1, &_uvbo);
    glGenBuffers(1, &_ebo);

    glBindVertexArray(_vao);

    // vbo - vertices
    glBindBuffer(GL_ARRAY_BUFFER, _vbo);
    glBufferData(
      GL_ARRAY_BUFFER,
      static_cast<GLsizeiptr>(_vertices.size() * sizeof(float)),
      &_vertices[0],
      GL_STATIC_DRAW);

    glVertexAttribPointer(
      0,
      3,
      GL_FLOAT,
      GL_FALSE,
      3 * sizeof(GLfloat),
      static_cast<void *>(nullptr));
    glEnableVertexAttribArray(0);

    // nvbo - normals
    glBindBuffer(GL_ARRAY_BUFFER, _nvbo);
    glBufferData(GL_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(_normals.size() * sizeof(float)),
                 &_normals[0],
                 GL_STATIC_DRAW);

    glVertexAttribPointer(
      1,
      3,
      GL_FLOAT,
      GL_FALSE,
      3 * sizeof(GLfloat),
      static_cast<void *>(nullptr));
    glEnableVertexAttribArray(1);

    // uvbo - texture coordinates
    glBindBuffer(GL_ARRAY_BUFFER, _uvbo);
    glBufferData(GL_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(_uvs.size() * sizeof(float)),
                 &_uvs[0],
                 GL_STATIC_DRAW);

    glVertexAttribPointer(
      2,
      2,
      GL_FLOAT,
      GL_FALSE,
      2 * sizeof(GLfloat),
      static_cast<void *>(nullptr));
    glEnableVertexAttribArray(2);


    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(_indices.size() * sizeof(int)),
                 &_indices[0],
                 GL_STATIC_DRAW);

    // Unbind the currently bound buffer so that we don't accidentally make unwanted changes to it.
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // Unbind the VAO now, so we don't accidentally tamper with it.
    // NOTE: You must NEVER unbind the element array buffer associated with a VAO!
    glBindVertexArray(0);

    _initialized = true;
    return true;
  }

  void OpenGL_Mesh::Use() const
  {
    glBindVertexArray(_vao);
    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(_indices.size()), GL_UNSIGNED_INT, nullptr);
  }

  void OpenGL_Mesh::CleanUp()
  {
    if (!_initialized) { return; }

    glDeleteBuffers(1, &_vao);
    glDeleteBuffers(1, &_vbo);
    glDeleteBuffers(1, &_nvbo);
    glDeleteBuffers(1, &_uvbo);
    glDeleteBuffers(1, &_ebo);

    _initialized = false;
  }
} // core