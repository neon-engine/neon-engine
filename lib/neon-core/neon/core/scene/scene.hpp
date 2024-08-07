#ifndef SCENE_HPP
#define SCENE_HPP

#include "neon/core/input/input-context.hpp"
#include "neon/core/oop-system/controllers/fps-player-controller.hpp"
#include "neon/core/oop-system/scene-graph/node.hpp"
#include "neon/core/render/render-context.hpp"
#include "neon/core/window/window-context.hpp"


namespace core
{
  class Scene
  {
    RenderContext *_render_context;
    InputContext *_input_context;
    WindowContext *_window_context;
    FPS_PlayerController _player;
    Node _scene_graph;

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
