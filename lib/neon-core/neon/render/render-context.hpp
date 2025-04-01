#ifndef RENDER_CONTEXT_HPP
#define RENDER_CONTEXT_HPP

#include <vector>
#include <neon/common/data-buffer.hpp>
#include <neon/common/transform.hpp>
#include <neon/logging/logger.hpp>

#include "light-source.hpp"
#include "render-info.hpp"
#include "render-object-ref.hpp"


namespace neon
{
  struct RenderResolution
  {
    RenderResolution(const int width, const int height)
      : width(width),
        height(height) {}

    const int width;
    const int height;
  };

  class RenderContext
  {
  protected:
    DataBuffer<RenderObjectRef> _render_object_buffer;
    std::shared_ptr<Logger> _logger;

    ~RenderContext() = default;

  public:
    explicit RenderContext(const int max_render_objects, const std::shared_ptr<Logger> &logger):
      _render_object_buffer(max_render_objects)
    {
      _logger = logger;
    }

    virtual int CreateRenderObject(const RenderInfo &render_info) = 0;

    virtual void DrawRenderObject(
      int render_object_id,
      const Transform &transform,
      const glm::mat4 &view,
      const glm::mat4 &projection,
      const std::vector<LightSource> &lights) = 0;

    virtual void DestroyRenderObject(int render_object_id) = 0;

    virtual const RenderResolution& GetRenderResolution() = 0;
  };
} // neon

#endif //RENDER_CONTEXT_HPP
