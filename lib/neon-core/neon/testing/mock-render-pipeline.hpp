#ifndef MOCK_RENDER_PIPELINE_HPP
#define MOCK_RENDER_PIPELINE_HPP

#include <memory>

#include <gmock/gmock.h>

#include <neon/render/render-pipeline.hpp>

namespace neon::testing
{
  /// A render pipeline that draws nothing. Creating and destroying render
  /// objects is mocked as well, so it needs no render context.
  class MockRenderPipeline : public RenderPipeline
  {
  public:
    explicit MockRenderPipeline(const std::shared_ptr<Logger> &logger, RenderContext *render_context = nullptr)
      : RenderPipeline(render_context, logger) {}

    MOCK_METHOD(int, CreateRenderObject, (const RenderInfo &render_info), (override));

    MOCK_METHOD(void, DestroyRenderObject, (int render_object_id), (override));

    MOCK_METHOD(void, SetCameraInfo, (const CameraInfo &camera_info), (override));

    MOCK_METHOD(void, EnqueueForRendering, (int render_object_id, const Transform &local_transform), (override));

    MOCK_METHOD(void, EnqueueLightSource, (const LightSource &light_source), (override));

    MOCK_METHOD(void, SetSky, (const SkyInfo &sky), (override));

    MOCK_METHOD(void, Initialize, (), (override));

    MOCK_METHOD(void, RenderFrame, (), (override));

    MOCK_METHOD(void, CleanUp, (), (override));
  };
} // neon::testing

#endif //MOCK_RENDER_PIPELINE_HPP
