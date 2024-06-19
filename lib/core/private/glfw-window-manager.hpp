#ifndef GLFW_WINDOW_MANAGER_HPP
#define GLFW_WINDOW_MANAGER_HPP
#include <core/window-info.hpp>
#include <core/window-manager.hpp>
#include <GLFW/glfw3.h>

namespace core
{
  class Glfw_WindowManager final : public WindowManager
  {
    static void ConfigureForOpenGl();

    GLFWwindow *_window = nullptr;

  public:
    explicit Glfw_WindowManager(const Window_Info &window_info);

    void RenderLoop() override;

    void HideCursor() override;

    void ShowCursor() override;
  };
} // core

#endif //GLFW_WINDOW_MANAGER_HPP
