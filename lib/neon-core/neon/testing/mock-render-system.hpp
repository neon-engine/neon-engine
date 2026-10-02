#ifndef MOCK_RENDER_SYSTEM_HPP
#define MOCK_RENDER_SYSTEM_HPP

#include <memory>
#include <string>
#include <vector>

#include <gmock/gmock.h>

#include <neon/render/render-system.hpp>

namespace neon::testing
{
  /// A render system that draws nothing. It needs no window and no file
  /// system, both may be nullptr.
  class MockRenderSystem : public RenderSystem
  {
  public:
    explicit MockRenderSystem(
      const std::shared_ptr<Logger> &logger,
      const SettingsConfig &settings_config = {},
      WindowContext *window_context = nullptr,
      FileSystemContext *file_system_context = nullptr,
      const int max_render_objects = 16)
      : RenderSystem(window_context, file_system_context, settings_config, max_render_objects, logger) {}

    MOCK_METHOD(void, Initialize, (), (override));

    MOCK_METHOD(void, CleanUp, (), (override));

    MOCK_METHOD(void, PrepareFrame, (), (override));

    MOCK_METHOD(void, FinishFrame, (), (override));

    MOCK_METHOD(bool, CaptureFrame, (const std::string &path), (override));

    MOCK_METHOD(const RenderCapabilities &, GetCapabilities, (), (const, override));

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

    MOCK_METHOD(const RenderResolution &, GetRenderResolution, (), (override));
  };
} // neon::testing

#endif //MOCK_RENDER_SYSTEM_HPP
