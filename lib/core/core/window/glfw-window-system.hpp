#ifndef GLFW_WINDOW_SYSTEM_HPP
#define GLFW_WINDOW_SYSTEM_HPP

#include "core/private/neon.hpp"

#include "window-system.hpp"
#include "core/settings-config.hpp"

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

    void ConfigureWindowForRenderer(const SettingsConfig &settings_config) const;

  public:
    void Initialize() override;

    [[nodiscard]] bool IsRunning() const override;

    void CleanUp() override;

    void HideCursor() override;

    void ShowCursor() override;

    [[nodiscard]] GLFWwindow *GetWindow() const;

    void Update() override;
  };
} // core

#endif //GLFW_WINDOW_SYSTEM_HPP
