#ifndef GLFW_WINDOW_SYSTEM_HPP
#define GLFW_WINDOW_SYSTEM_HPP

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

    void Update() override;

  protected:
    void ConfigureWindowForRenderer() override;

  public:
    void SignalToClose() override;

    double GetDeltaTime() override;
  };
} // core

#endif //GLFW_WINDOW_SYSTEM_HPP
