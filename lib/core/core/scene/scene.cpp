#include "scene.hpp"

#include <iostream>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

void core::Scene::Initialize()
{
  std::cout << "Initializing the scene" << std::endl;
  _cube.Initialize();
  std::cout << "Initialized scene!" << std::endl;
  _input_context->CenterAndHideCursor();
}

void core::Scene::Update()
{
  const auto delta_time = _window_context->GetDeltaTime();
  _player.Update(delta_time, glm::mat4{1.0f});
}

void core::Scene::RenderFrame() const
{
  const auto camera = _player.GetCamera();
  const glm::mat4 view = camera.GetView();
  const auto [width, height] = _render_context->GetRenderResolution();
  const glm::mat4 projection = camera.GetProjection(width, height);
  _cube.Draw(view, projection);
}

void core::Scene::CleanUp()
{
  std::cout << "Cleaning up the scene" << std::endl;
  _cube.CleanUp();
}
