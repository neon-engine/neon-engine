#ifndef GLFW_INPUT_SYSTEM_HPP
#define GLFW_INPUT_SYSTEM_HPP

#include "input-system.hpp"

namespace core
{
  class Glfw_InputSystem final : public InputSystem
  {
  public:
    explicit Glfw_InputSystem(const SettingsConfig &settings_config)
      : InputSystem(settings_config) {}

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
