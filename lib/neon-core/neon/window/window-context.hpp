#ifndef SDL_2_WINDOW_CONTEXT_HPP
#define SDL_2_WINDOW_CONTEXT_HPP

#include <string>
#include <vector>

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
  };
}


#endif //SDL_2_WINDOW_CONTEXT_HPP
