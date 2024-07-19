#include "glfw-input-system.hpp"

#include <iostream>

namespace core
{
  void GLFW_InputSystem::ProcessInputWrapper() {}
  void GLFW_InputSystem::ProcessInputCallback(const int key, const int scancode, const int action, const int mods) {}
  void GLFW_InputSystem::MouseInputCallback(const double x_pos, const double y_pos) {}
  void GLFW_InputSystem::ScrollCallback(const double scroll_x_offset, const double scroll_y_offset) {}

  void GLFW_InputSystem::Initialize()
  {
    std::cout << "Initializing GLFW input system" << std::endl;
  }

  void GLFW_InputSystem::ProcessInput() {}

  void GLFW_InputSystem::CleanUp()
  {
    std::cout << "Cleaning up GLFW input system" << std::endl;
  }
} // core
