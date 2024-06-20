#include "glfw-input-system.hpp"

#include "glfw-window-system.hpp"
#include "GLFW/glfw3.h"

namespace core
{
  void Glfw_InputSystem::ProcessInputWrapper()
  {
  }
  void Glfw_InputSystem::ProcessInputCallback(const int key, const int scancode, const int action, const int mods) {}
  void Glfw_InputSystem::MouseInputCallback(const double x_pos, const double y_pos) {}
  void Glfw_InputSystem::ScrollCallback(const double scroll_x_offset, const double scroll_y_offset) {}
} // core
