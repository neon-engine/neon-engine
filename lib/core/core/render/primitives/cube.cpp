#include "cube.hpp"

#include <iostream>

#include <glm/gtc/matrix_transform.hpp>

namespace core
{
  Cube::Cube(RenderContext *render_context, const std::vector<std::string> &texture_paths, const std::string &shader_path)
    : _geometry_id(0),
      _material_id(0)
  {
    _render_context = render_context;
    _texture_paths = texture_paths;
    _shader_path = shader_path;
  }

  Cube::~Cube()
  {
    CleanUp();
  }

  void Cube::Initialize()
  {
    std::cout << "Initializing Cube" << std::endl;
    _material_id = _render_context->InitMaterial(_shader_path, _texture_paths);
    _geometry_id = _render_context->InitMesh("assets/models/bear.obj", _normal_matrix);
  }

  void Cube::Draw(const glm::mat4 &view, const glm::mat4 &projection) const
  {
    _render_context->UseMaterial(_material_id, translate(_model, _position) * _normal_matrix, view, projection);
    _render_context->DrawMesh(_geometry_id);
  }

  void Cube::CleanUp()
  {
    if (_destroyed)
    {
      return;
    }

    std::cout << "Cleaning up Cube" << std::endl;
    _render_context->DestroyMaterial(_material_id);
    _render_context->DestroyMesh(_geometry_id);
    _destroyed = true;
  }
} // core
