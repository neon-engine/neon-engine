#ifndef GLFW_WINDOW_SYSTEM_HPP
#define GLFW_WINDOW_SYSTEM_HPP

#include <GLFW/glfw3.h>

#include "core/settings-config.hpp"

namespace core
{
  class Glfw_WindowSystem
  {
    void ConfigureWindowForRenderer(const SettingsConfig &settings_config);
  public:
    GLFWwindow *window = nullptr;
    const GLFWwindow *GetWindow();
    void InitializeWindow(const SettingsConfig &settings_config);
    bool IsRunning();
    void HideCursor(GLFWwindow *window);
    void ShowCursor(GLFWwindow *window);
  };
} // core

#endif //GLFW_WINDOW_SYSTEM_HPP
