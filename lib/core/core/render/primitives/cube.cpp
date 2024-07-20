#include "cube.hpp"

namespace core {
  Cube::Cube(RenderContext *render_context, const std::string &texture_path, const std::string &shader_path)
  {
    _render_context = render_context;
    _geometry_id = _render_context->InitGeometry(_vertices, _normals, _tex_coordinates, _indices);
    _texture_id = _render_context->InitTexture(texture_path);
    _shader_id = _render_context->InitShader(shader_path);
  }
  Cube::~Cube()
  {
    _render_context->DestroyShader(_shader_id);
    _render_context->DestroyTexture(_texture_id);
    _render_context->DestroyGeometry(_geometry_id);
  }

  void Cube::Draw() const
  {
    _render_context->UseShader(_shader_id);
    _render_context->UseTexture(_texture_id);
    _render_context->DrawGeometry(_geometry_id);
  }
} // core