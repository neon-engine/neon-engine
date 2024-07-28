#include "open-gl-mesh.hpp"

#include <algorithm>
#include <iostream>
#include <glm/gtc/matrix_transform.hpp>

namespace core {
  OpenGL_Mesh::OpenGL_Mesh() = default;

  OpenGL_Mesh::OpenGL_Mesh(const std::vector<float> &vertices,
                           const std::vector<float> &normals,
                           const std::vector<float> &uvs,
                           const std::vector<unsigned int> &indices)
  {
    _vertices = vertices;
    _normals = normals;
    _uvs = uvs;
    _indices = indices;
  }

  // ReSharper disable once CppDFAConstantFunctionResult
  bool OpenGL_Mesh::Initialize()
  {
    if (_initialized)
    {
      std::cerr << "Mesh with opengl id " << _vao << " already initialized" << std::endl;
      return true;
    }

    GenerateNormalizationMatrix();

    _vao = 0;
    _vbo = 0;
    _nvbo = 0;
    _uvbo = 0;
    _ebo = 0;

    glGenVertexArrays(1, &_vao);
    glGenBuffers(1, &_vbo);
    glGenBuffers(1, &_nvbo);
    glGenBuffers(1, &_uvbo);

    if (!_indices.empty())
    {
      glGenBuffers(1, &_ebo);
    }

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


    if (!_indices.empty())
    {
      glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _ebo);
      glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                   static_cast<GLsizeiptr>(_indices.size() * sizeof(unsigned int)),
                   &_indices[0],
                   GL_STATIC_DRAW);
    }

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
    if (!_indices.empty())
    {
      glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(_indices.size()), GL_UNSIGNED_INT, nullptr);
    } else
    {
      glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(_vertices.size() / 3));
    }
  }

  void OpenGL_Mesh::CleanUp()
  {
    if (!_initialized) { return; }

    glDeleteBuffers(1, &_vao);
    glDeleteBuffers(1, &_vbo);
    glDeleteBuffers(1, &_nvbo);
    glDeleteBuffers(1, &_uvbo);

    if (!_indices.empty())
    {
      glDeleteBuffers(1, &_ebo);
    }

    _initialized = false;
  }

  glm::mat4 OpenGL_Mesh::GetModelMatrix() const
  {
    return _model_matrix;
  }

  void OpenGL_Mesh::GenerateNormalizationMatrix()
  {
    std::cout << "Generating normalizing matrix using the loaded vertices" << std::endl;
    auto min_x = _vertices[0];
    auto min_y = _vertices[1];
    auto min_z = _vertices[2];

    auto max_x = _vertices[0];
    auto max_y = _vertices[1];
    auto max_z = _vertices[2];

    for (int i = 3; i < _vertices.size(); i += 3) {
      const auto x = _vertices[i];
      const auto y = _vertices[i + 1];
      const auto z = _vertices[i + 2];

      if (x < min_x) {min_x = x;}
      if (y < min_y) {min_y = y;}
      if (z < min_z) {min_z = z;}

      if (x > max_x) {max_x = x;}
      if (y > max_y) {max_y = y;}
      if (z > max_z) {max_z = z;}
    }

    const auto x_center = (min_x + max_x) / 2.0f;
    const auto y_center = (min_y + max_y) / 2.0f;
    const auto z_center = (min_z + max_z) / 2.0f;

    const auto range_x = max_x - min_x;
    const auto range_y = max_y - min_y;
    const auto range_z = max_z - min_z;

    const auto max_range = std::max(std::max(range_x, range_y), range_z);

    const auto translation = translate(glm::mat4(1.0f), {-x_center, -y_center, -z_center});
    _model_matrix =
      scale(glm::mat4(1.0f), {1.0f / max_range, 1.0f / max_range, 1.0f / max_range}) * translation;
  }
} // core