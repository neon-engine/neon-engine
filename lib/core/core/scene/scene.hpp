#ifndef SCENE_HPP
#define SCENE_HPP

#include "camera/camera.hpp"
#include "primitives/cube.hpp"
#include "render/render-context.hpp"

namespace core
{
  class Scene
  {
    RenderContext *_render_context;
    Camera _camera;
    Cube _cube;

  public:
    explicit Scene(RenderContext *render_context): _cube(render_context), _render_context(render_context) {}

    void Initialize();

    void DrawFrame();

    void CleanUp();
  };
}


#endif //SCENE_HPP
