#ifndef RENDER_PIPELINE_HPP
#define RENDER_PIPELINE_HPP
#include <memory>

#include "render-context.hpp"
#include "neon/core/logging/logger.hpp"

namespace core
{
  class RenderPipeline
  {
  protected:
    RenderContext *_render_context;
    std::shared_ptr<Logger> _logger;

    ~RenderPipeline() = default;

  public:
    explicit RenderPipeline(RenderContext *render_context, const std::shared_ptr<Logger> &logger)
    {
      _render_context = render_context;
      _logger = logger;
    }

    virtual int CreateRenderObject(const RenderInfo &render_info)
    {
      return _render_context->CreateRenderObject(render_info);
    }

    virtual void DestroyRenderObject(const int render_object_id)
    {
      _render_context->DestroyRenderObject(render_object_id);
    }

    virtual void SetCameraInfo(const CameraInfo& camera_info) = 0;

    virtual void EnqueueForRendering(int render_object_id, const glm::mat4& to_world) = 0;

    virtual void Initialize() = 0;

    virtual void RenderFrame() = 0;

    virtual void CleanUp() = 0;
  };
} // core

#endif //RENDER_PIPELINE_HPP
