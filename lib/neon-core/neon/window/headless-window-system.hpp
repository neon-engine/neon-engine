#ifndef HEADLESS_WINDOW_SYSTEM_HPP
#define HEADLESS_WINDOW_SYSTEM_HPP

#include <memory>
#include <neon/runtime/settings-config.hpp>
#include <neon/logging/logger.hpp>
#include <neon/window/window-system.hpp>

namespace neon
{
  /// A window system without a window. It needs no display, so the engine can
  /// render on a build server or from a script.
  ///
  /// Time advances by the same amount every frame. A run therefore produces
  /// the same frames every time, whatever the speed of the machine. The
  /// amount is the time step of the settings, or frame_time without one.
  // ReSharper disable once CppInconsistentNaming
  class Headless_WindowSystem final : public WindowSystem
  {
    bool _should_close = false;

  protected:
    void ConfigureWindowForRenderer() override;

  public:
    /// Seconds that pass between two frames when the settings name no time
    /// step.
    static constexpr double frame_time = 1.0 / 60.0;

    explicit Headless_WindowSystem(const SettingsConfig &settings_config, const std::shared_ptr<Logger> &logger)
      : WindowSystem(settings_config, logger) {}

    void Initialize() override;

    [[nodiscard]] bool IsRunning() const override;

    void CleanUp() override;

    void Update() override;

    void SignalToClose() override;

    double GetDeltaTime() override;

    void CenterCursor() override;

    void SetWindowFocus(bool focus) override;

    WindowSize GetDrawableSize() override;

    std::vector<std::string> GetVulkanInstanceExtensions() override;

    bool CreateVulkanSurface(void *instance, void *surface) override;
  };
} // neon

#endif //HEADLESS_WINDOW_SYSTEM_HPP
