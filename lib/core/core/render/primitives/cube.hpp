#ifndef CUBE_HPP
#define CUBE_HPP

#include <glm/glm.hpp>

#include "render-context.hpp"


namespace core
{
  class Cube
  {
    glm::vec3 _position = glm::vec3(0.0f, 0.0f, -5.0f);
    glm::mat4 _model = glm::mat4(1.0f);

    RenderContext *_render_context;

    std::vector<std::string> _texture_paths;
    std::string _shader_path;

    int _geometry_id;
    int _material_id;

    bool _destroyed = false;

  public:
    explicit Cube(
      RenderContext *render_context,
      const std::vector<std::string> &texture_paths,
      const std::string &shader_path);

    ~Cube();

    void Initialize();

    void CleanUp();

    void Draw(const glm::mat4 &view, const glm::mat4 &projection) const;

  private:
    std::vector<float> _vertices = {
      0.5f, -0.5f, 0.5f,
      -0.5f, -0.5f, 0.5f,
      -0.5f, -0.5f, -0.5f,
      0.5f, 0.5f, -0.5f,
      -0.5f, 0.5f, -0.5f,
      -0.5f, 0.5f, 0.5f,
      0.5f, 0.5f, -0.5f,
      0.5f, 0.5f, 0.5f,
      0.5f, -0.5f, 0.5f,
      0.5f, -0.5f, 0.5f,
      0.5f, 0.5f, 0.5f,
      -0.5f, 0.5f, 0.5f,
      -0.5f, 0.5f, 0.5f,
      -0.5f, 0.5f, -0.5f,
      -0.5f, -0.5f, -0.5f,
      0.5f, 0.5f, -0.5f,
      0.5f, -0.5f, -0.5f,
      -0.5f, -0.5f, -0.5f,
      0.5f, -0.5f, -0.5f,
      0.5f, -0.5f, 0.5f,
      -0.5f, -0.5f, -0.5f,
      0.5f, 0.5f, 0.5f,
      0.5f, 0.5f, -0.5f,
      -0.5f, 0.5f, 0.5f,
      0.5f, -0.5f, -0.5f,
      0.5f, 0.5f, -0.5f,
      0.5f, -0.5f, 0.5f,
      -0.5f, -0.5f, 0.5f,
      0.5f, -0.5f, 0.5f,
      -0.5f, 0.5f, 0.5f,
      -0.5f, -0.5f, 0.5f,
      -0.5f, 0.5f, 0.5f,
      -0.5f, -0.5f, -0.5f,
      -0.5f, 0.5f, -0.5f,
      0.5f, 0.5f, -0.5f,
      -0.5f, -0.5f, -0.5f
    };

    std::vector<float> _normals = {
      0.f, -1.f, 0.f,
      0.f, -1.f, 0.f,
      0.f, -1.f, 0.f,
      0.f, 1.f, 0.f,
      0.f, 1.f, 0.f,
      0.f, 1.f, 0.f,
      1.f, 0.f, 0.f,
      1.f, 0.f, 0.f,
      1.f, 0.f, 0.f,
      -0.f, -0.f, 1.f,
      -0.f, -0.f, 1.f,
      -0.f, -0.f, 1.f,
      -1.f, -0.f, -0.f,
      -1.f, -0.f, -0.f,
      -1.f, -0.f, -0.f,
      0.f, 0.f, -1.f,
      0.f, 0.f, -1.f,
      0.f, 0.f, -1.f,
      0.f, -1.f, 0.f,
      0.f, -1.f, 0.f,
      0.f, -1.f, 0.f,
      0.f, 1.f, 0.f,
      0.f, 1.f, 0.f,
      0.f, 1.f, 0.f,
      1.f, -0.f, 0.f,
      1.f, -0.f, 0.f,
      1.f, -0.f, 0.f,
      -0.f, -0.f, 1.f,
      -0.f, -0.f, 1.f,
      -0.f, -0.f, 1.f,
      -1.f, -0.f, -0.f,
      -1.f, -0.f, -0.f,
      -1.f, -0.f, -0.f,
      0.f, 0.f, -1.f,
      0.f, 0.f, -1.f,
      0.f, 0.f, -1.f,
    };

    std::vector<float> _tex_coordinates = {
      0.f, -0.f,
      1.f, -0.f,
      1.f, -1.f,
      1.f, -1.f,
      0.f, -1.f,
      0.f, -0.f,
      1.f, -1.f,
      0.f, -1.f,
      0.f, -0.f,
      1.f, -0.f,
      1.f, -1.f,
      0.f, -1.f,
      1.f, -1.f,
      0.f, -1.f,
      0.f, -0.f,
      0.f, -1.f,
      0.f, -0.f,
      1.f, -0.f,
      0.f, -1.f,
      0.f, -0.f,
      1.f, -1.f,
      1.f, -0.f,
      1.f, -1.f,
      0.f, -0.f,
      1.f, -0.f,
      1.f, -1.f,
      0.f, -0.f,
      0.f, -0.f,
      1.f, -0.f,
      0.f, -1.f,
      1.f, -0.f,
      1.f, -1.f,
      0.f, -0.f,
      1.f, -1.f,
      0.f, -1.f,
      1.f, -0.f
    };

    std::vector<int> _indices = {
      // Front
      0, 1, 2,
      2, 3, 0,

      // Right
      0, 3, 7,
      7, 4, 0,

      // Bottom
      2, 6, 7,
      7, 3, 2,

      // Left
      1, 5, 6,
      6, 2, 1,

      // Back
      4, 7, 6,
      6, 5, 4,

      // Top
      5, 1, 0,
      0, 4, 5,
    };
  };
} // core

#endif //CUBE_HPP
