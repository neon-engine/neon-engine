#include "cube.hpp"

#include <iostream>

namespace core
{
  Cube::Cube(
    RenderContext *render_context,
    const std::vector<std::string> &texture_paths,
    const std::string &shader_path,
    const Color &color)
    : _render_object_id(-1)
  {
    _render_context = render_context;
    _texture_paths = texture_paths;
    _shader_path = shader_path;
    _color = color;
  }

  void Cube::Initialize()
  {
    std::cout << "Initializing Cube" << std::endl;
    _render_object_id = _render_context->CreateRenderObject(
      "assets/models/cube.obj",
      _shader_path,
      _texture_paths,
      _color);
  }

  void Cube::Draw(const glm::mat4 &view, const glm::mat4 &projection) const
  {
    _render_context->DrawRenderObject(_render_object_id, _transform, view, projection);
  }

  void Cube::CleanUp()
  {
    if (_destroyed)
    {
      return;
    }

    std::cout << "Cleaning up Cube" << std::endl;
    _render_context->DestroyRenderObject(_render_object_id);
    _destroyed = true;
  }
} // core
