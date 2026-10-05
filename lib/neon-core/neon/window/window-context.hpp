#ifndef SDL_2_WINDOW_CONTEXT_HPP
#define SDL_2_WINDOW_CONTEXT_HPP

#include <cstdint>
#include <string>
#include <vector>

#include "window-metrics.hpp"
#include "window-mode.hpp"

namespace neon
{
  struct WindowSize
  {
    int width;
    int height;
  };

  class WindowContext
  {
  protected:
    ~WindowContext() = default;

  public:
    virtual void SignalToClose() = 0;

    /// Seconds the application advances by in this frame. This is the time
    /// step of the settings when one is set, and the time since the last
    /// call otherwise.
    virtual double GetDeltaTime() = 0;

    virtual void CenterCursor() = 0;

    virtual void SetWindowFocus(bool focus) = 0;

    /// Size in pixels of the surface the renderer draws to. This can differ
    /// from the configured width and height, for example in borderless mode
    /// where the window takes the size of the display.
    virtual WindowSize GetDrawableSize() = 0;

    /// Names of the Vulkan instance extensions the window needs before it can
    /// be drawn to. Empty when there is no window to draw to.
    virtual std::vector<std::string> GetVulkanInstanceExtensions() = 0;

    /// Creates the Vulkan surface of the window. `instance` is a VkInstance
    /// and `surface` points to the VkSurfaceKHR to fill in. They are passed
    /// untyped so that this interface stays free of Vulkan declarations.
    /// A later renderer would add its own pair of hooks next to these.
    /// Returns false when there is no window to draw to, or on failure.
    virtual bool CreateVulkanSurface(void *instance, void *surface) = 0;

    /// The size of the window in points, which is what the platform counts
    /// a window and the pointer in. Without a display of high density it is
    /// the size of what is drawn to.
    virtual WindowSize GetWindowSize()
    {
      return GetDrawableSize();
    }

    /// The window in points and in pixels, from which the density follows.
    virtual WindowMetrics GetMetrics()
    {
      const WindowSize points = GetWindowSize();
      const WindowSize pixels = GetDrawableSize();
      return {points.width, points.height, pixels.width, pixels.height};
    }

    /// Goes up whenever the size of the window or its density changes: when
    /// it is resized, and when it is moved to a display of another density.
    /// What depends on either keeps the number it last saw.
    [[nodiscard]] virtual std::uint64_t GetMetricsRevision()
    {
      return 0;
    }

    /// Shows the window another way while the application runs: as a
    /// window, without borders over the whole display, or as the one thing
    /// on a display that is switched to the size of the window. The
    /// renderer follows the size the window has afterwards by itself.
    /// Returns false, and leaves the window as it is, when it cannot be
    /// shown that way, which is said.
    virtual bool SetWindowMode(WindowMode mode)
    {
      return false;
    }

    /// How the window is shown now.
    [[nodiscard]] virtual WindowMode GetWindowMode()
    {
      return WindowMode::Windowed;
    }

    /// Gives the window another size, in points. A window has that size; a
    /// display that the window takes over is switched to the size it offers
    /// that is nearest; a window without borders covers its display, and
    /// keeps the size for when it is a window again. Returns false for a
    /// size that is not above zero, and for a window that has no size to
    /// change.
    virtual bool SetWindowSize(int width, int height)
    {
      return false;
    }

    /// The sizes the display of the window offers, in points, the largest
    /// first and each once: what a menu lets a player choose from. Empty
    /// for a window without a display.
    [[nodiscard]] virtual std::vector<WindowSize> GetDisplaySizes()
    {
      return {};
    }

    /// Holds the frames to at most that many a second, while the
    /// application runs: a number from 30 to 300, or 0 for as many as can
    /// be drawn. A number beyond them is held to the nearest, see
    /// FrameLimit. It holds with vertical sync on and off, and while the
    /// world is paused.
    virtual void SetFrameLimit(int frames_per_second) {}

    /// The limit that holds, or 0 for none.
    [[nodiscard]] virtual int GetFrameLimit()
    {
      return 0;
    }

    /// Shows the cursor in a shape. A window without a cursor does nothing.
    virtual void SetCursorShape(CursorShape shape) {}
  };
}


#endif //SDL_2_WINDOW_CONTEXT_HPP
