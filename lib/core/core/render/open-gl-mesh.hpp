#ifndef OPEN_GL_MESH_HPP
#define OPEN_GL_MESH_HPP

#include <GL/gl3w.h>

#include <vector>

namespace core
{
  // ReSharper disable once CppInconsistentNaming
  class OpenGL_Mesh
  {
    GLuint _vao = 0;
    GLuint _vbo = 0;
    GLuint _nvbo = 0;
    GLuint _uvbo = 0;
    GLuint _ebo = 0;
    bool _initialized = false;
    std::vector<float> _vertices{};
    std::vector<float> _normals{};
    std::vector<float> _uvs{};
    std::vector<int> _indices{};
  public:
    OpenGL_Mesh();
    explicit OpenGL_Mesh(
      const std::vector<float> &vertices,
      const std::vector<float> &normals,
      const std::vector<float> &uvs,
      const std::vector<int> &indices);

    ~OpenGL_Mesh();

    bool Initialize();

    void Use() const;

    void CleanUp();
  };
} // core

#endif //OPEN_GL_MESH_HPP
