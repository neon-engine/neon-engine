#include "cube.hpp"

namespace core {
  Cube::Cube(RenderContext *render_context)
  {
    _render_context = render_context;
    _geometry_id = _render_context->InitGeometry(_vertices, _normals, _tex_coordinates, _indices);
  }
  Cube::~Cube()
  {
    _render_context->DestroyGeometry(_geometry_id);
  }

  void Cube::Draw()
  {
    _render_context->DrawGeometry(_geometry_id);
  }
} // core