#ifndef SDL_2_WINDOW_SYSTEM_HPP
#define SDL_2_WINDOW_SYSTEM_HPP

#include <array>
#include <memory>
#include <SDL.h>
#include <neon/runtime/settings-config.hpp>
#include <neon/logging/logger.hpp>
#include <neon/window/window-system.hpp>

namespace neon
{
  // ReSharper disable once CppInconsistentNaming
  class SDL2_WindowSystem final : public WindowSystem {
    SDL_Window *_window = nullptr;
    int _window_flags = 0;
    bool _should_close = false;
    uint64_t _last_frame;

    // seconds the frame before took
    double _delta_time = 0.0;

    void MeasureFrame();

    // what the window measured when it was last looked at
    WindowMetrics _metrics;
    std::uint64_t _metrics_revision = 0;

    // the cursors of the system, each created when it is first shown
    std::array<SDL_Cursor *, 16> _cursors{};
    CursorShape _cursor_shape = CursorShape::Default;

    void LoadVulkanLibrary() const;

    void FollowMetrics();

  protected:
    void ConfigureWindowForRenderer() override;

  public:
    explicit SDL2_WindowSystem(const SettingsConfig &settings_config, const std::shared_ptr<Logger> &logger)
      : WindowSystem(settings_config, logger)
    {
      _last_frame = SDL_GetPerformanceCounter();
    }

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

    WindowSize GetWindowSize() override;

    WindowMetrics GetMetrics() override;

    [[nodiscard]] std::uint64_t GetMetricsRevision() override;

    void SetCursorShape(CursorShape shape) override;

    /// Says to SDL what has to be said before it starts: that the process
    /// knows of displays of high density, so that no platform draws the
    /// window small and stretches it. It does nothing else, and is called
    /// by Initialize().
    static void SetHints();

    /// The flags a window of these settings is created with, less what the
    /// renderer asks for.
    [[nodiscard]] static int WindowFlagsOf(const SettingsConfig &settings_config);

    /// The window of SDL in points and in pixels. `pixel_width` and
    /// `pixel_height` are the size of what is drawn to, as the renderer
    /// reports it.
    [[nodiscard]] static WindowMetrics MetricsOf(SDL_Window *window, int pixel_width, int pixel_height);

    /// The cursor of the system for a shape of CSS, as SDL names it, or -1
    /// for no cursor.
    [[nodiscard]] static int SystemCursorOf(CursorShape shape);
  };
} // neon

#endif //SDL_2_WINDOW_SYSTEM_HPP
