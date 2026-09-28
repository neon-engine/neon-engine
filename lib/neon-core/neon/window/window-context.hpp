#ifndef SDL_2_WINDOW_CONTEXT_HPP
#define SDL_2_WINDOW_CONTEXT_HPP

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

    virtual void* GetGlProcAddress() = 0;

    /// Size in pixels of the surface the renderer draws to. This can differ
    /// from the configured width and height, for example in borderless mode
    /// where the window takes the size of the display.
    virtual WindowSize GetDrawableSize() = 0;
  };
}


#endif //SDL_2_WINDOW_CONTEXT_HPP
