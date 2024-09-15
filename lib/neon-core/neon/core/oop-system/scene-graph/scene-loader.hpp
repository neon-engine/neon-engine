#ifndef SCENE_HPP
#define SCENE_HPP

#include "node.hpp"
#include "neon/core/input/input-context.hpp"
#include "neon/core/oop-system/controllers/fps-player-controller.hpp"
#include "neon/core/render/render-pipeline.hpp"
#include "neon/core/window/window-context.hpp"


namespace core
{
  class SceneLoader
  {
    WindowContext *_window_context;
    InputContext *_input_context;
    RenderPipeline *_render_pipeline;
    FPS_PlayerController _player;
    Node _scene_graph;

  public:
    explicit SceneLoader(
      RenderPipeline *render_pipeline,
      InputContext *input_context,
      WindowContext *window_context);

    void Initialize();

    void Update();

    void RenderFrame() const;

    void CleanUp();
  };
}


#endif //SCENE_HPP
