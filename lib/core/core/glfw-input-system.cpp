#include "glfw-input-system.hpp"

#include <iostream>

namespace core
{
  void Glfw_InputSystem::ProcessInputWrapper() {}
  void Glfw_InputSystem::ProcessInputCallback(const int key, const int scancode, const int action, const int mods) {}
  void Glfw_InputSystem::MouseInputCallback(const double x_pos, const double y_pos) {}
  void Glfw_InputSystem::ScrollCallback(const double scroll_x_offset, const double scroll_y_offset) {}

  void Glfw_InputSystem::Initialize()
  {
    std::cout << "Initializing GLFW input system" << std::endl;
  }

  void Glfw_InputSystem::ProcessInput() {}

  void Glfw_InputSystem::CleanUp()
  {
    std::cout << "Cleaning up GLFW input system" << std::endl;
  }
} // core
