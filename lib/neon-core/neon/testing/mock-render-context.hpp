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

    MOCK_METHOD(void, UpdateRenderObject, (int render_object_id, const RenderInfo &render_info), (override));
    MOCK_METHOD(void, FreeUnused, (), (override));
    MOCK_METHOD(bool, SetVerticalSync, (bool enabled), (override));
    MOCK_METHOD(bool, GetVerticalSync, (), (override));
    MOCK_METHOD(bool, SetAnisotropy, (int level), (override));
    MOCK_METHOD(int, GetAnisotropy, (), (override));
    MOCK_METHOD(bool, SetTextureScale, (double scale), (override));
    MOCK_METHOD(double, GetTextureScale, (), (override));
    MOCK_METHOD(bool, SetTargetScale, (double scale), (override));
    MOCK_METHOD(double, GetTargetScale, (), (override));
    MOCK_METHOD(bool, SetTargetMipmaps, (int mipmaps), (override));
    MOCK_METHOD(int, GetTargetMipmaps, (), (override));
    MOCK_METHOD(void, UpdateRenderObjectMesh, (int render_object_id, const MeshData &mesh), (override));

    MOCK_METHOD(bool, SetImage, (const std::string &name, const ImagePixels &pixels), (override));

    MOCK_METHOD(void, SetShaderTime, (double seconds, double delta), (override));
    MOCK_METHOD(
      void,
      SetEffects,
      (const std::vector<std::string> &effects, const std::vector<std::string> &screen_effects),
      (override));

    MOCK_METHOD(bool, SetShaderNumbers, (int place, const glm::vec4 &numbers), (override));

    MOCK_METHOD(const RenderResolution &, GetRenderResolution, (), (override));
  };
} // neon::testing

#endif //MOCK_RENDER_CONTEXT_HPP
