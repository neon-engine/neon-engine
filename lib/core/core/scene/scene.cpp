#include "scene.hpp"

#include <iostream>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

void core::Scene::Initialize()
{
  std::cout << "Initializing the scene" << std::endl;
  _cube.Initialize();
  std::cout << "Initialized scene!" << std::endl;
}

void core::Scene::Draw()
{
  _camera.Update(1.0f, {0.0f, 0.0f, 0.0f});
  const glm::mat4 view = _camera.GetView();
  const auto [width, height] = _render_context->GetRenderResolution();
  const glm::mat4 projection = glm::perspective(
    glm::radians(_camera.GetFov()),
    static_cast<float>(width) / static_cast<float>(height),
    0.1f,
    1000.0f
    );

  _cube.Draw(view, projection);
}

void core::Scene::CleanUp()
{
  std::cout << "Cleaning up the scene" << std::endl;
  _cube.CleanUp();
}
