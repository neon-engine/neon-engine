#include "scene.hpp"


core::Scene::Scene(RenderContext *render_context)
{
  _render_context = render_context;
}

void core::Scene::Initialize()
{
  auto vertices = std::vector({
    // positions         // colors            // texture coords
    // (x, y, z)         // (r, g, b)         // (s, t)
    00.5f, 00.5f, 00.0f, 01.0f, 00.0f, 00.0f, 01.0f, 01.0f, // top right
    00.5f, -0.5f, 00.0f, 00.0f, 01.0f, 00.0f, 01.0f, 00.0f, // bottom right
    -0.5f, -0.5f, 00.0f, 00.0f, 00.0f, 01.0f, 00.0f, 00.0f, // bottom left
    -0.5f, 00.5f, 00.0f, 01.0f, 01.0f, 00.0f, 00.0f, 01.0f  // top left
  });
  auto indices = std::vector({
    0, 1, 3,
    1, 2, 3
  });

  auto geo1 = _render_context->InitGeometry(vertices, indices);
  auto geo2 = _render_context->InitGeometry(vertices, indices);
  auto shader1 = _render_context->InitShader();
  auto texture1 = _render_context->InitTexture();
  auto shader2 = _render_context->InitShader();
  auto texture2 = _render_context->InitTexture();
  auto geo3 = _render_context->InitGeometry(vertices, indices);

  _render_context->DestroyGeometry(geo1);
  _render_context->DestroyGeometry(geo2);
  _render_context->DestroyGeometry(geo3);

  _render_context->DestroyShader(shader1);
  _render_context->DestroyShader(shader2);

  _render_context->DestroyTexture(texture1);
  _render_context->DestroyTexture(texture2);
}

void core::Scene::Draw() {}

void core::Scene::CleanUp()
{
}
