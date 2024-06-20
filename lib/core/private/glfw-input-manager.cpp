#include "glfw-input-manager.hpp"

namespace core
{

  void Glfw_InputManager::ProcessInput(GLFWwindow *window) {}

  void Glfw_InputManager::ProcessInputCallback(
    GLFWwindow *window,
    const int key,
    const int scancode,
    const int action,
    const int mods) {}

  void Glfw_InputManager::MouseInputCallback(
    GLFWwindow *window,
    const double x_pos,
    const double y_pos) {}

  void Glfw_InputManager::ScrollCallback(
    GLFWwindow *window,
    const double scroll_x_offset,
    const double scroll_y_offset) {}
} // core
