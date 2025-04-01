#ifndef GL_MESH_HPP
#define GL_MESH_HPP

#include <glad/gl.h>
#include <neon/render/mesh.hpp>

namespace neon
{
  // ReSharper disable once CppInconsistentNaming
  class GL_Mesh final : public Mesh
  {
    GLuint _vao = 0;
    GLuint _vbo = 0;
    GLuint _ebo = 0;

  public:
    GL_Mesh(const std::vector<Vertex> &vertices,
                const std::vector<unsigned int> &indices,
                const std::vector<TextureInfo> &textures,
                const std::shared_ptr<Logger> &logger)
      : Mesh(vertices, indices, textures, logger) {}

    bool Initialize() override;

    void Use() const override;

    void CleanUp() override;
  };
} // neon

#endif //GL_MESH_HPP
