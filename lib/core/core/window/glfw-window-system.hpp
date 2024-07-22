#ifndef GLFW_WINDOW_SYSTEM_HPP
#define GLFW_WINDOW_SYSTEM_HPP

// gl3w must be first
// ReSharper disable once CppUnusedIncludeDirective
#include <GL/gl3w.h>

#include <GLFW/glfw3.h>

#include "settings-config.hpp"
#include "window-system.hpp"

namespace core
{
  // ReSharper disable once CppInconsistentNaming
  class GLFW_WindowSystem final : public WindowSystem
  {
  public:
    explicit GLFW_WindowSystem(const SettingsConfig &settings_config)
      : WindowSystem(settings_config) {}

  private:
    GLFWwindow *_window = nullptr;

  public:
    void Initialize() override;

    [[nodiscard]] bool IsRunning() const override;

    void CleanUp() override;

    void HideCursor() override;

    void ShowCursor() override;

    [[nodiscard]] GLFWwindow *GetWindow() const;

    void Update() override;

  protected:
    void ConfigureWindowForRenderer() override;

  public:
    void SignalToClose() override;

    double GetDeltaTime() override;
  };
} // core

#endif //GLFW_WINDOW_SYSTEM_HPP
