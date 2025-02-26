#ifndef OPEN_GL_MESH_HPP
#define OPEN_GL_MESH_HPP
#include "mesh.hpp"
#include "glad/gl.h"

namespace core
{
  // ReSharper disable once CppInconsistentNaming
  class OpenGL_Mesh final : public Mesh
  {
    GLuint _vao = 0;
    GLuint _vbo = 0;
    GLuint _ebo = 0;

  public:
    OpenGL_Mesh(const std::vector<Vertex> &vertices,
                const std::vector<unsigned int> &indices,
                const std::vector<Texture> &textures)
      : Mesh(vertices, indices, textures) {}

  protected:
    void SetupMesh() override;

  public:
    void Draw() override;
  };
} // core

#endif //OPEN_GL_MESH_HPP
