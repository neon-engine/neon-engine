#ifndef HEADLESS_WINDOW_SYSTEM_HPP
#define HEADLESS_WINDOW_SYSTEM_HPP

#include <memory>
#include <neon/runtime/settings-config.hpp>
#include <neon/logging/logger.hpp>
#include <neon/window/window-system.hpp>

namespace neon
{
  /// A window system without a window. It needs no display, so the engine can
  /// render on a build server or from a script. It is what --headless-renderer
  /// creates: the renderer draws off-screen. A headless runtime, the dedicated
  /// server of #144, has no window system at all, so the name stays.
  ///
  /// Time advances by the same amount every frame. A run therefore produces
  /// the same frames every time, whatever the speed of the machine. The
  /// amount is the time step of the settings, or frame_time without one.
  // ReSharper disable once CppInconsistentNaming
  class Headless_WindowSystem final : public WindowSystem
  {
    bool _should_close = false;
    CursorShape _cursor_shape = CursorShape::Default;

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

    /// The size of the settings times their render scale, which stands in
    /// for the density of a display.
    WindowSize GetDrawableSize() override;

    /// The size of the settings.
    WindowSize GetWindowSize() override;

    /// The shape that was asked for last. There is no cursor to show, so
    /// this is for tests and for tools.
    void SetCursorShape(CursorShape shape) override;

    [[nodiscard]] CursorShape GetCursorShape() const;

    std::vector<std::string> GetVulkanInstanceExtensions() override;

    bool CreateVulkanSurface(void *instance, void *surface) override;
  };
} // neon

#endif //HEADLESS_WINDOW_SYSTEM_HPP
