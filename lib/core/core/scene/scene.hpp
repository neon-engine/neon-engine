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
    Cube _light_cube;

  public:
    explicit Scene(
      RenderContext *render_context,
      InputContext *input_context,
      WindowContext *window_context);

    void Initialize();

    void Update();

    void RenderFrame() const;

    void CleanUp();
  };
}


#endif //SCENE_HPP
