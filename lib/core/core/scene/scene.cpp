#include "scene.hpp"


core::Scene::Scene(RenderContext *render_context)
{
  _render_context = render_context;
}

void core::Scene::Initialize()
{
  _render_context->InitGeometry();
  _render_context->InitGeometry();
  _render_context->InitShader();
  _render_context->InitTexture();
  _render_context->InitShader();
  _render_context->InitTexture();
  _render_context->InitGeometry();
}

void core::Scene::Draw() {}
