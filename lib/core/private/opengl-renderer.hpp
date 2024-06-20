#ifndef GL_RENDERER_HPP
#define GL_RENDERER_HPP
#include "core/renderer.hpp"
#include "core/window-info.hpp"

namespace core
{
  class OpenGl_Renderer final : public Renderer
  {
  public:
    explicit OpenGl_Renderer(const Window_Info &window_info);

    virtual ~OpenGl_Renderer() = default;
    void Initialize() override;
  };
} // core

#endif //GL_RENDERER_HPP
