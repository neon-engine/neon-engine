#include "cube.hpp"

#include <iostream>

namespace core
{
  Cube::Cube(RenderContext *render_context, const std::string &texture_path, const std::string &shader_path)
    : _geometry_id(0),
      _material_id(0)
  {
    _render_context = render_context;
    _texture_path = texture_path;
    _shader_path = shader_path;
  }

  Cube::~Cube()
  {
    CleanUp();
  }

  void Cube::Initialize()
  {
    std::cout << "Initializing Cube" << std::endl;
    _geometry_id = _render_context->InitMesh(_vertices, _normals, _tex_coordinates, _indices);
    _material_id = _render_context->InitMaterial(_shader_path, {_texture_path});
  }

  void Cube::Draw(const glm::mat4 &view, const glm::mat4 &projection) const
  {
    _render_context->UseMaterial(_material_id, _model, view, projection);
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
