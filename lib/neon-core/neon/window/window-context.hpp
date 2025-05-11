#ifndef SDL_2_WINDOW_CONTEXT_HPP
#define SDL_2_WINDOW_CONTEXT_HPP

namespace neon
{
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
  };
}


#endif //SDL_2_WINDOW_CONTEXT_HPP
