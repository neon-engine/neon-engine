#ifndef SCENE_HPP
#define SCENE_HPP
#include "render/render-context.hpp"

namespace core
{
  class Scene
  {
    RenderContext *_render_context;
  public:
    explicit Scene(RenderContext *render_context);
    void Initialize();
    void DrawFrame();
    void CleanUp();
  };
}


#endif //SCENE_HPP
