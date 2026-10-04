#ifndef MOCK_RENDER_CONTEXT_HPP
#define MOCK_RENDER_CONTEXT_HPP

#include <memory>
#include <vector>

#include <gmock/gmock.h>

#include <neon/render/render-context.hpp>

namespace neon::testing
{
  class MockRenderContext : public RenderContext
  {
  public:
    explicit MockRenderContext(const std::shared_ptr<Logger> &logger, const int max_render_objects = 16)
      : RenderContext(max_render_objects, logger) {}

    MOCK_METHOD(int, CreateRenderObject, (const RenderInfo &render_info), (override));

    MOCK_METHOD(
      void,
      DrawRenderObject,
      (int render_object_id,
        const Transform &transform,
        const glm::mat4 &view,
        const glm::mat4 &projection,
        const std::vector<LightSource> &lights),
      (override));

    MOCK_METHOD(void, DestroyRenderObject, (int render_object_id), (override));

    MOCK_METHOD(void, UpdateRenderObjectMesh, (int render_object_id, const MeshData &mesh), (override));

    MOCK_METHOD(bool, SetImage, (const std::string &name, const ImagePixels &pixels), (override));

    MOCK_METHOD(const RenderResolution &, GetRenderResolution, (), (override));
  };
} // neon::testing

#endif //MOCK_RENDER_CONTEXT_HPP
