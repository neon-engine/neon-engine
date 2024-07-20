#ifndef CUBE_HPP
#define CUBE_HPP
#include "render-context.hpp"
#include "glm/vec3.hpp"

namespace core
{
  class Cube
  {
    glm::vec3 _position = glm::vec3(0.0f, 0.0f, 0.0f);

    std::vector<float> _vertices = {
      // Front face
      -1.0, -1.0, 1.0,
      1.0, -1.0, 1.0,
      1.0, 1.0, 1.0,
      -1.0, 1.0, 1.0,
      // Back face
      -1.0, -1.0, -1.0,
      -1.0, 1.0, -1.0,
      1.0, 1.0, -1.0,
      1.0, -1.0, -1.0,
      // Top face
      -1.0, 1.0, -1.0,
      -1.0, 1.0, 1.0,
      1.0, 1.0, 1.0,
      1.0, 1.0, -1.0,
      // Bottom face
      -1.0, -1.0, -1.0,
      1.0, -1.0, -1.0,
      1.0, -1.0, 1.0,
      -1.0, -1.0, 1.0,
      // Right face
      1.0, -1.0, -1.0,
      1.0, 1.0, -1.0,
      1.0, 1.0, 1.0,
      1.0, -1.0, 1.0,
      // Left face
      -1.0, -1.0, -1.0,
      -1.0, -1.0, 1.0,
      -1.0, 1.0, 1.0,
      -1.0, 1.0, -1.0
    };

    std::vector<float> _normals = {
      // Front face
      0.0, 0.0, 1.0,
      0.0, 0.0, 1.0,
      0.0, 0.0, 1.0,
      0.0, 0.0, 1.0,
      // Back face
      0.0, 0.0, -1.0,
      0.0, 0.0, -1.0,
      0.0, 0.0, -1.0,
      0.0, 0.0, -1.0,
      // Top face
      0.0, 1.0, 0.0,
      0.0, 1.0, 0.0,
      0.0, 1.0, 0.0,
      0.0, 1.0, 0.0,
      // Bottom face
      0.0, -1.0, 0.0,
      0.0, -1.0, 0.0,
      0.0, -1.0, 0.0,
      0.0, -1.0, 0.0,
      // Right face
      1.0, 0.0, 0.0,
      1.0, 0.0, 0.0,
      1.0, 0.0, 0.0,
      1.0, 0.0, 0.0,
      // Left face
      -1.0, 0.0, 0.0,
      -1.0, 0.0, 0.0,
      -1.0, 0.0, 0.0,
      -1.0, 0.0, 0.0
    };

    std::vector<float> _tex_coordinates = {
      // Front face
      0.0, 0.0,
      1.0, 0.0,
      1.0, 1.0,
      0.0, 1.0,
      // Back face
      0.0, 0.0,
      1.0, 1.0,
      1.0, 0.0,
      0.0, 1.0,
      // Top face
      0.0, 1.0,
      0.0, 0.0,
      1.0, 0.0,
      1.0, 1.0,
      // Bottom face
      1.0, 1.0,
      0.0, 1.0,
      0.0, 0.0,
      1.0, 0.0,
      // Right face
      1.0, 0.0,
      1.0, 1.0,
      0.0, 1.0,
      0.0, 0.0,
      // Left face
      0.0, 0.0,
      1.0, 0.0,
      1.0, 1.0,
      0.0, 1.0
    };

    std::vector<int> _indices = {
      // Front face
      0, 1, 2,
      2, 3, 0,
      // Back face
      4, 5, 6,
      6, 7, 4,
      // Top face
      8, 9, 10,
      10, 11, 8,
      // Bottom face
      12, 13, 14,
      14, 15, 12,
      // Right face
      16, 17, 18,
      18, 19, 16,
      // Left face
      20, 21, 22,
      22, 23, 20
    };

    RenderContext *_render_context;

    int _geometry_id;
    int _texture_id;
    int _shader_id;

  public:
    explicit Cube(RenderContext *render_context, const std::string &texture_path, const std::string &shader_path);

    ~Cube();

    void Draw() const;
  };
} // core

#endif //CUBE_HPP
