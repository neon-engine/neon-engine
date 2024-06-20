#ifndef WINDOW_MANAGER_HPP
#define WINDOW_MANAGER_HPP

#include "window-info.hpp"

namespace core
{
  class WindowManager
  {
  protected:
    Window_Info _window_info;
    bool _created_window = false;
    bool _created_input_manager = false;
    bool _created_renderer = false;

    explicit WindowManager(const Window_Info &window_info)
    {
      _window_info = window_info;
    }

    ~WindowManager() = default;

  public:
    virtual void Initialize() = 0;

    virtual void CleanUp() = 0;

    virtual void RenderLoop() = 0;

    virtual void HideCursor() = 0;

    virtual void ShowCursor() = 0;

    virtual void InitializeInputManager() = 0;

    virtual void InitializeRenderer() = 0;

    virtual InputManager GetInputManager();

    virtual Renderer GetRenderer();
  };
} // core

#endif //WINDOW_MANAGER_HPP
