#ifndef GLFW_INPUT_MANAGER_HPP
#define GLFW_INPUT_MANAGER_HPP
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include "core/input-manager.hpp"

namespace core
{
  class Glfw_InputManager final : public InputManager
  {
  public:

    void ProcessInput(GLFWwindow *window);

    void ProcessInputCallback(
      GLFWwindow *window,
      const int key,
      const int scancode,
      const int action,
      const int mods);

    void MouseInputCallback(
      GLFWwindow *window,
      const double x_pos,
      const double y_pos);

    void ScrollCallback(
      GLFWwindow *window,
      const double scroll_x_offset,
      const double scroll_y_offset);

    void CleanUp() override;
  };
} // core

#endif //GLFW_INPUT_MANAGER_HPP
