#ifndef SCENE_HPP
#define SCENE_HPP

#include <functional>

#include "neon/input/input-context.hpp"
#include "neon/render/render-pipeline.hpp"
#include "neon/window/window-context.hpp"
#include "node/node.hpp"


namespace neon
{
  class SceneManager
  {
    WindowContext *_window_context;
    InputContext *_input_context;
    RenderPipeline *_render_pipeline;
    Node* _root;
    std::shared_ptr<Logger> _logger;

  public:
    explicit SceneManager(
      RenderPipeline *render_pipeline,
      InputContext *input_context,
      WindowContext *window_context,
      const std::shared_ptr<Logger> &logger);

    void Initialize() const;

    void Update() const;

    void CleanUp() const;

  private:
    void PreOrderTraversal(const std::function<void(Node *)> & func) const;

    void PostOrderTraversal(const std::function<void(Node *)> & func) const;
  };
}


#endif //SCENE_HPP
