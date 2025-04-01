#ifndef WORLD_SYSTEM_HPP
#define WORLD_SYSTEM_HPP

#include <neon/input/input-context.hpp>
#include <neon/logging/logger.hpp>
#include <neon/render/render-pipeline.hpp>
#include <neon/window/window-context.hpp>

#include <ryml.hpp>

namespace neon
{
  class WorldSystem
  {
  protected:
    WindowContext *_window_context;
    InputContext *_input_context;
    RenderPipeline *_render_pipeline;
    std::shared_ptr<Logger> _logger;

    ~WorldSystem() = default;

  public:
    WorldSystem(
      RenderPipeline *render_pipeline,
      InputContext *input_context,
      WindowContext *window_context,
      const std::shared_ptr<Logger> &logger)
    {
      _render_pipeline = render_pipeline;
      _input_context = input_context;
      _window_context = window_context;
      _logger = logger;
    }

    virtual void Initialize() = 0;

    virtual void Update() = 0;

    virtual void CleanUp() = 0;

    virtual void LoadScene(const std::string &file_path)
    {
    }

    // virtual void LoadSceneImpl() = 0;
  };
}

#endif // WORLD_SYSTEM_HPP
