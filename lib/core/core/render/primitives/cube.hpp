#ifndef CUBE_HPP
#define CUBE_HPP

#include <glm/glm.hpp>

#include "render-context.hpp"


namespace core
{
  class Cube
  {
    glm::vec3 _position = glm::vec3(0.0f, 0.0f, 0.0f);
    glm::mat4 _model = glm::mat4(1.0f);

    RenderContext *_render_context;

    std::string _texture_path;
    std::string _shader_path;

    int _geometry_id;
    int _material_id;

    bool _destroyed = false;

  public:
    explicit Cube(RenderContext *render_context, const std::string &texture_path, const std::string &shader_path);

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
      1, 2, 3,
4, 7, 6,
4, 5, 1,
1, 5, 6,
6, 7, 3,
4, 0, 3,
0, 1, 3,
5, 4, 6,
0, 4, 1,
2, 1, 6,
2, 6, 3,
7, 4, 3
    };
  };
} // core

#endif //CUBE_HPP
