#ifndef RENDERER_HPP
#define RENDERER_HPP
#include <stdexcept>

#include "opengl-renderer.hpp"
#include "window-info.hpp"

namespace core
{
  class Renderer final
  {
    Window_Info _window_info;
  public:
    explicit Renderer(const Window_Info &window_info)
    {
      _window_info = window_info;
    }

    void Initialize();

    void CleanUp();
  };
} // core

#endif //RENDERER_HPP
