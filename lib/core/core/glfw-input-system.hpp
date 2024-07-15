#ifndef GLFW_INPUT_SYSTEM_HPP
#define GLFW_INPUT_SYSTEM_HPP

#include <GLFW/glfw3.h>
#include "input-system.hpp"

namespace core
{
  class Glfw_InputSystem final : public InputSystem
  {
    GLFWwindow *_window = nullptr;

  public:
    explicit Glfw_InputSystem(
      const SettingsConfig &settings_config,
      GLFWwindow *window)
      : InputSystem(settings_config)
    {
      _window = window;
    }

    void Initialize() override;

    void ProcessInput() override;

    void CleanUp() override;

    static void ProcessInputWrapper();

    void ProcessInputCallback(
      const int key,
      const int scancode,
      const int action,
      const int mods);

    void MouseInputCallback(
      const double x_pos,
      const double y_pos);

    void ScrollCallback(
      const double scroll_x_offset,
      const double scroll_y_offset);
  };
} // core

#endif //GLFW_INPUT_SYSTEM_HPP
