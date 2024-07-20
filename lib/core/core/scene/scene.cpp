#include "scene.hpp"

#include <iostream>

void core::Scene::Initialize()
{
  std::cout << "Initializing the scene" << std::endl;
  _cube.Initialize();
  std::cout << "Initialized scene!" << std::endl;
}

void core::Scene::Draw()
{
  _camera.Update(1.0f, {0.0f, 0.0f, 0.0f});
  _cube.Draw();
}

void core::Scene::CleanUp()
{
  std::cout << "Cleaning up the scene" << std::endl;
  _cube.CleanUp();
}
