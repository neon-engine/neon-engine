#include "scene.hpp"

#include <iostream>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

void core::Scene::Initialize()
{
  std::cout << "Initializing the scene" << std::endl;
  _cube.Initialize();
  std::cout << "Initialized scene!" << std::endl;
  _input_context->SnapCursorToWindow();
  _input_context->HideCursor();
}

void core::Scene::Draw()
{
  const auto delta_time = _window_context->GetDeltaTime();
  _player.Update(delta_time, glm::mat4{1.0f});
  const auto camera = _player.GetCamera();
  const glm::mat4 view = camera.GetView();
  const auto [width, height] = _render_context->GetRenderResolution();
  const glm::mat4 projection = glm::perspective(
    glm::radians(camera.GetFov()),
    static_cast<float>(width) / static_cast<float>(height),
    0.1f,
    100.0f
    );

  _cube.Draw(view, projection);
}

void core::Scene::CleanUp()
{
  std::cout << "Cleaning up the scene" << std::endl;
  _cube.CleanUp();
}
