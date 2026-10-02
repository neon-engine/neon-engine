#ifndef MOCK_WORLD_SYSTEM_HPP
#define MOCK_WORLD_SYSTEM_HPP

#include <memory>

#include <gmock/gmock.h>

#include <neon/world-system/world-system.hpp>

namespace neon::testing
{
  /// A world that holds nothing. It needs none of the systems a world is
  /// given, all of them may be nullptr.
  class MockWorldSystem : public WorldSystem
  {
  public:
    explicit MockWorldSystem(
      const std::shared_ptr<Logger> &logger,
      RenderPipeline *render_pipeline = nullptr,
      InputContext *input_context = nullptr,
      WindowContext *window_context = nullptr)
      : WorldSystem(render_pipeline, input_context, window_context, logger) {}

    MOCK_METHOD(void, Initialize, (), (override));

    MOCK_METHOD(void, Update, (), (override));

    MOCK_METHOD(void, SetPaused, (bool paused), (override));

    MOCK_METHOD(void, LoadScene, (const std::string &file_path), (override));

    MOCK_METHOD(void, CleanUp, (), (override));
  };
} // neon::testing

#endif //MOCK_WORLD_SYSTEM_HPP
