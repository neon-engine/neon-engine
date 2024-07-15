#ifndef GLFW_WINDOW_SYSTEM_HPP
#define GLFW_WINDOW_SYSTEM_HPP

#include <GLFW/glfw3.h>

#include "settings-config.hpp"
#include "window-system.hpp"

namespace core
{
  class Glfw_WindowSystem final : public WindowSystem
  {
  public:
    explicit Glfw_WindowSystem(const SettingsConfig &settings_config)
      : WindowSystem(settings_config) {}

  private:
    void ConfigureWindowForRenderer(const SettingsConfig &settings_config);

  public:
    GLFWwindow *window = nullptr;
    const GLFWwindow *GetWindow();
    void InitializeWindow(const SettingsConfig &settings_config);
    bool IsRunning();
    void HideCursor(GLFWwindow *window);
    void ShowCursor(GLFWwindow *window);

    void Initialize() override;

    [[nodiscard]] bool IsRunning() const override;

    void CleanUp() override;
  };
} // core

#endif //GLFW_WINDOW_SYSTEM_HPP
