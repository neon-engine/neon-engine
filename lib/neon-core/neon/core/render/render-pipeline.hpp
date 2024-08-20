#ifndef RENDER_PIPELINE_HPP
#define RENDER_PIPELINE_HPP
#include "render-context.hpp"

namespace core
{
  class RenderPipeline
  {
  protected:
    RenderContext *_render_context;

    ~RenderPipeline() = default;

  public:
    explicit RenderPipeline(RenderContext *render_context)
    {
      _render_context = render_context;
    }

    virtual int CreateRenderObject(const RenderInfo &render_info)
    {
      return _render_context->CreateRenderObject(render_info);
    }

    virtual void DestroyRenderObject(const int render_object_id)
    {
      _render_context->DestroyRenderObject(render_object_id);
    }

    virtual RenderResolution GetRenderResolution()
    {
      return _render_context->GetRenderResolution();
    }

    virtual void EnqueueForRendering(int render_object_id, const glm::mat4& to_world) = 0;

    virtual void Initialize() = 0;

    virtual void RenderFrame() = 0;

    virtual void CleanUp() = 0;
  };
} // core

#endif //RENDER_PIPELINE_HPP
