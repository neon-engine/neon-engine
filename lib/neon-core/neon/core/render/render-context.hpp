#ifndef RENDER_CONTEXT_HPP
#define RENDER_CONTEXT_HPP

#include <string>
#include <vector>
#include <neon/core/common/data-buffer.hpp>
#include <neon/core/common/transform.hpp>
#include "render-info.hpp"
#include "render-object-ref.hpp"


namespace core
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

    ~RenderContext() = default;

  public:
    explicit RenderContext(const int max_render_objects): _render_object_buffer(max_render_objects) {}

    virtual int CreateRenderObject(const RenderInfo &render_info) = 0;

    virtual int CreateRenderObject(
      const std::vector<float> &vertices,
      const std::vector<float> &normals,
      const std::vector<float> &tex_coordinates,
      const std::vector<unsigned int> &indices,
      const std::string &shader_path,
      const std::vector<std::string> &texture_paths,
      const Color &color) = 0;

    virtual void DrawRenderObject(
      int render_object_id,
      const Transform &transform,
      const glm::mat4 &view,
      const glm::mat4 &projection) = 0;

    virtual void DrawRenderObject(
      int render_object_id,
      const glm::mat4 &to_world,
      const glm::mat4 &view,
      const glm::mat4 &projection) = 0;

    virtual void DestroyRenderObject(int render_object_id) = 0;

    virtual const RenderResolution& GetRenderResolution() = 0;
  };
} // core

#endif //RENDER_CONTEXT_HPP
