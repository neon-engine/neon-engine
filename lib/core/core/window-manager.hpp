#ifndef WINDOW_MANAGER_HPP
#define WINDOW_MANAGER_HPP

namespace core
{
  class WindowManager
  {
  protected:
    Window_Info _window_info;

    explicit WindowManager(const Window_Info &window_info)
    {
      _window_info = window_info;
    }

    ~WindowManager() = default;

  public:
    virtual void RenderLoop() = 0;
    virtual void HideCursor() = 0;
    virtual void ShowCursor() = 0;
  };
} // core

#endif //WINDOW_MANAGER_HPP
