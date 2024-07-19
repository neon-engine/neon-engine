#include "scene.hpp"


core::Scene::Scene(RenderContext *render_context)
{
  _render_context = render_context;
}

void core::Scene::Initialize()
{
}

void core::Scene::DrawFrame() {}

void core::Scene::CleanUp()
{
}
