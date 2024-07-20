#include "cube.hpp"

#include <iostream>

namespace core
{
  Cube::Cube(RenderContext *render_context, const std::string &texture_path, const std::string &shader_path)
    : _geometry_id(0),
      _material_id(0),
      _shader_id(0)
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
    _geometry_id = _render_context->InitGeometry(_vertices, _normals, _tex_coordinates, _indices);
    std::string textures[] = {_texture_path};
    _material_id = _render_context->InitMaterial(_shader_path, textures);
  }

  void Cube::Draw() const
  {
    _render_context->UseMaterial(_material_id);
    _render_context->DrawGeometry(_geometry_id);
  }

  void Cube::CleanUp()
  {
    if (_destroyed)
    {
      return;
    }

    std::cout << "Cleaning up Cube" << std::endl;
    _render_context->DestroyMaterial(_material_id);
    _render_context->DestroyGeometry(_geometry_id);
    _destroyed = true;
  }
} // core
