#ifndef SCENE_HPP
#define SCENE_HPP


#include "camera.hpp"
#include "primitives/cube.hpp"


namespace core
{
  class Scene
  {
    RenderContext *_render_context;
    Camera _camera;
    Cube _cube;

  public:
    explicit Scene(RenderContext *render_context)
      : _render_context(render_context),
        _cube(render_context,
          {
            "assets/textures/dark.png"
          },
          "assets/shaders/unlit") {}

    void Initialize();

    void Draw();

    void CleanUp();
  };
}


#endif //SCENE_HPP
