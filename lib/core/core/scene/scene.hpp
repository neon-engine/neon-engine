#ifndef SCENE_HPP
#define SCENE_HPP

#include "input-context.hpp"
#include "window-context.hpp"
#include "controllers/fps-player-controller.hpp"
#include "primitives/cube.hpp"

namespace core
{
  class Scene
  {
    RenderContext *_render_context;
    InputContext *_input_context;
    WindowContext *_window_context;
    FPS_PlayerController _player;
    Cube _cube;

  public:
    explicit Scene(
      RenderContext *render_context,
      InputContext *input_context,
      WindowContext *window_context)
      : _render_context(render_context),
        _input_context(input_context),
        _window_context(window_context),
        _player(input_context),
        _cube(render_context,
              {
                "assets/textures/dark.png"
              },
              "assets/shaders/unlit") {}

    void Initialize();

    void Update();

    void RenderFrame() const;

    void CleanUp();
  };
}


#endif //SCENE_HPP
