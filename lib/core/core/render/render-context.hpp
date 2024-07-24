#ifndef RENDER_CONTEXT_HPP
#define RENDER_CONTEXT_HPP

#include <string>
#include <vector>

#include "data-buffer.hpp"
#include "data-types.hpp"

namespace core
{
  class RenderContext
  {
  protected:
    DataBuffer<RenderObject> _render_object_buffer;

    ~RenderContext() = default;

  public:
    explicit RenderContext(const int max_render_objects): _render_object_buffer(max_render_objects) {}

    virtual int CreateRenderObject(
      std::string model_path,
      std::string shader_path,
      std::vector<std::string> texture_paths) = 0;

    virtual int CreateRenderObject(
      const std::vector<float> &vertices,
      const std::vector<float> &normals,
      const std::vector<float> &tex_coordinates,
      const std::vector<unsigned int> &indices,
      std::string shader_path,
      std::vector<std::string> texture_paths) = 0;

    virtual void DrawRenderObject(
      int render_object_id,
      const Transform &transform,
      const glm::mat4 &view,
      const glm::mat4 &projection) = 0;

    virtual void DestroyRenderObject(int render_object_id) = 0;

    virtual const RenderResolution& GetRenderResolution() = 0;
  };
} // core

#endif //RENDER_CONTEXT_HPP
