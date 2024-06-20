#ifndef GLFW_WINDOW_MANAGER_HPP
#define GLFW_WINDOW_MANAGER_HPP

#include <private/glfw-input-manager.hpp>
#include "renderer.hpp"
#include "window-info.hpp"
#include "window-manager.hpp"
#include "GLFW/glfw3.h"


namespace core
{
  class Glfw_WindowManager final : public WindowManager
  {
    GLFWwindow *_window = nullptr;
    Renderer _renderer;
    Glfw_InputManager _input_manager;

    void ConfigureWindowForRenderer() const;

  public:
    explicit Glfw_WindowManager(const Window_Info &window_info);

    ~Glfw_WindowManager();

    void Initialize() override;

    void CleanUp() override;

    void RenderLoop() override;

    void HideCursor() override;

    void ShowCursor() override;

    void InitializeInputManager() override;

    void InitializeRenderer() override;

    InputManager GetInputManager() override;

    Renderer GetRenderer() override;
  };
} // core

#endif //GLFW_WINDOW_MANAGER_HPP
