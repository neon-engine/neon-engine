#ifndef OPEN_GL_MESH_HPP
#define OPEN_GL_MESH_HPP

#include <glad/gl.h>

#include "mesh.hpp"

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
                const std::vector<Texture> &textures,
                const std::shared_ptr<Logger> &logger)
      : Mesh(vertices, indices, textures, logger) {}

    bool Initialize() override;

    void Use() override;

    void CleanUp() override;
  };
} // core

#endif //OPEN_GL_MESH_HPP
