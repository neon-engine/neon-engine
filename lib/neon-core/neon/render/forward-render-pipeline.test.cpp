#include "forward-render-pipeline.hpp"

#include <memory>
#include <string>
#include <vector>

#include <glm/gtc/matrix_transform.hpp>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <neon/testing/mock-render-context.hpp>
#include <neon/testing/recording-logger.hpp>

namespace
{
  using neon::CameraInfo;
  using neon::Forward_RenderPipeline;
  using neon::LightSource;
  using neon::RenderInfo;
  using neon::RenderResolution;
  using neon::Transform;
  using neon::testing::LogLevel;
  using neon::testing::MockRenderContext;
  using neon::testing::RecordingLogger;
  using ::testing::_;
  using ::testing::ElementsAre;
  using ::testing::Field;
  using ::testing::InSequence;
  using ::testing::IsEmpty;
  using ::testing::Return;
  using ::testing::ReturnRef;
  using ::testing::SizeIs;
  using ::testing::StrictMock;

  LightSource Light(const std::string &id)
  {
    LightSource light{};
    light.id = id;
    light.light_type = neon::LightType::Point;
    return light;
  }

  Transform At(const float x, const float y, const float z)
  {
    Transform transform;
    transform.position = {x, y, z};
    transform.world_coordinates = translate(glm::mat4(1.0f), transform.position);
    return transform;
  }

  ::testing::Matcher<const Transform &> IsAt(const float x, const float y, const float z)
  {
    return ::testing::AllOf(
      Field("position", &Transform::position, glm::vec3(x, y, z)),
      Field("world_coordinates", &Transform::world_coordinates, translate(glm::mat4(1.0f), glm::vec3(x, y, z))));
  }

  ::testing::Matcher<const LightSource &> HasId(const std::string &id)
  {
    return Field("id", &LightSource::id, id);
  }

  class ForwardRenderPipelineTest : public ::testing::Test
  {
  protected:
    std::shared_ptr<RecordingLogger> _logger = std::make_shared<RecordingLogger>();
    StrictMock<MockRenderContext> _render_context{_logger};
    RenderResolution _resolution{1920, 1080};
    Forward_RenderPipeline _pipeline{&_render_context, 3, _logger};

    void SetUp() override
    {
      EXPECT_CALL(_render_context, GetRenderResolution())
        .Times(::testing::AnyNumber())
        .WillRepeatedly(ReturnRef(_resolution));
    }
  };

  TEST_F(ForwardRenderPipelineTest, TouchesTheRendererNeitherWhenItStartsNorWhenItStops)
  {
    _pipeline.Initialize();
    _pipeline.CleanUp();

    EXPECT_TRUE(_logger->Contains(LogLevel::Info, "Initializing forward rendering pipeline"));
    EXPECT_TRUE(_logger->Contains(LogLevel::Info, "Cleaning up forward rendering pipeline"));
  }

  TEST_F(ForwardRenderPipelineTest, HandsTheCreationOfARenderObjectToTheRenderer)
  {
    RenderInfo render_info{};
    render_info.model_path = "assets://models/cube.obj";

    EXPECT_CALL(_render_context, CreateRenderObject(Field(&RenderInfo::model_path, "assets://models/cube.obj")))
      .WillOnce(Return(7));

    EXPECT_EQ(_pipeline.CreateRenderObject(render_info), 7);
  }

  TEST_F(ForwardRenderPipelineTest, HandsBackThatARenderObjectCouldNotBeCreated)
  {
    EXPECT_CALL(_render_context, CreateRenderObject(_)).WillOnce(Return(-1));

    EXPECT_EQ(_pipeline.CreateRenderObject(RenderInfo{}), -1);
  }

  TEST_F(ForwardRenderPipelineTest, HandsTheDestructionOfARenderObjectToTheRenderer)
  {
    EXPECT_CALL(_render_context, DestroyRenderObject(7));

    _pipeline.DestroyRenderObject(7);
  }

  TEST_F(ForwardRenderPipelineTest, DrawsNothingWhenNothingWasEnqueued)
  {
    _pipeline.RenderFrame();
  }

  TEST_F(ForwardRenderPipelineTest, DrawsNothingBeforeTheFrameIsRendered)
  {
    _pipeline.EnqueueForRendering(1, At(1.0f, 2.0f, 3.0f));
    _pipeline.EnqueueLightSource(Light("lamp"));

    // the render context is strict, so a draw here would fail the test
    ::testing::Mock::VerifyAndClearExpectations(&_render_context);
  }

  TEST_F(ForwardRenderPipelineTest, DrawsWhatWasEnqueuedInTheOrderItWasEnqueuedIn)
  {
    _pipeline.EnqueueForRendering(3, At(3.0f, 0.0f, 0.0f));
    _pipeline.EnqueueForRendering(1, At(1.0f, 0.0f, 0.0f));
    _pipeline.EnqueueForRendering(2, At(2.0f, 0.0f, 0.0f));

    {
      InSequence in_order;
      EXPECT_CALL(_render_context, DrawRenderObject(3, IsAt(3.0f, 0.0f, 0.0f), _, _, _));
      EXPECT_CALL(_render_context, DrawRenderObject(1, IsAt(1.0f, 0.0f, 0.0f), _, _, _));
      EXPECT_CALL(_render_context, DrawRenderObject(2, IsAt(2.0f, 0.0f, 0.0f), _, _, _));
    }

    _pipeline.RenderFrame();
  }

  TEST_F(ForwardRenderPipelineTest, DrawsAnObjectAsOftenAsItWasEnqueued)
  {
    _pipeline.EnqueueForRendering(1, At(1.0f, 0.0f, 0.0f));
    _pipeline.EnqueueForRendering(1, At(5.0f, 0.0f, 0.0f));

    EXPECT_CALL(_render_context, DrawRenderObject(1, IsAt(1.0f, 0.0f, 0.0f), _, _, _));
    EXPECT_CALL(_render_context, DrawRenderObject(1, IsAt(5.0f, 0.0f, 0.0f), _, _, _));

    _pipeline.RenderFrame();
  }

  TEST_F(ForwardRenderPipelineTest, DrawsAnObjectInTheFrameItWasEnqueuedForOnly)
  {
    _pipeline.EnqueueForRendering(1, At(1.0f, 0.0f, 0.0f));

    EXPECT_CALL(_render_context, DrawRenderObject(1, _, _, _, _)).Times(1);
    _pipeline.RenderFrame();
    _pipeline.RenderFrame();
  }

  TEST_F(ForwardRenderPipelineTest, DrawsWithTheTransformAsItWasWhenEnqueued)
  {
    Transform transform = At(1.0f, 2.0f, 3.0f);
    _pipeline.EnqueueForRendering(1, transform);

    transform.position = {9.0f, 9.0f, 9.0f};

    EXPECT_CALL(_render_context, DrawRenderObject(1, IsAt(1.0f, 2.0f, 3.0f), _, _, _));
    _pipeline.RenderFrame();
  }

  TEST_F(ForwardRenderPipelineTest, DrawsWithTheViewOfTheCamera)
  {
    CameraInfo camera;
    camera.view = lookAt(glm::vec3(0.0f, 2.0f, 5.0f), glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    _pipeline.SetCameraInfo(camera);
    _pipeline.EnqueueForRendering(1, At(0.0f, 0.0f, 0.0f));

    EXPECT_CALL(_render_context, DrawRenderObject(1, _, camera.view, _, _));

    _pipeline.RenderFrame();
  }

  TEST_F(ForwardRenderPipelineTest, DrawsWithAProjectionFromTheCameraAndTheResolution)
  {
    CameraInfo camera;
    camera.fov = 60.0f;
    camera.near = 0.5f;
    camera.far = 250.0f;
    _pipeline.SetCameraInfo(camera);
    _pipeline.EnqueueForRendering(1, At(0.0f, 0.0f, 0.0f));

    const auto projection = glm::perspective(glm::radians(60.0f), 1920.0f / 1080.0f, 0.5f, 250.0f);
    EXPECT_CALL(_render_context, DrawRenderObject(1, _, _, projection, _));

    _pipeline.RenderFrame();
  }

  TEST_F(ForwardRenderPipelineTest, DrawsWithTheCameraItWasCreatedWithUntilOneIsSet)
  {
    _pipeline.EnqueueForRendering(1, At(0.0f, 0.0f, 0.0f));

    const auto projection = glm::perspective(glm::radians(45.0f), 1920.0f / 1080.0f, 0.1f, 1000.0f);
    EXPECT_CALL(_render_context, DrawRenderObject(1, _, glm::mat4(1.0f), projection, _));

    _pipeline.RenderFrame();
  }

  TEST_F(ForwardRenderPipelineTest, AsksForTheResolutionInEveryFrame)
  {
    RenderResolution smaller{800, 600};
    ::testing::Mock::VerifyAndClearExpectations(&_render_context);
    EXPECT_CALL(_render_context, GetRenderResolution())
      .WillOnce(ReturnRef(_resolution))
      .WillOnce(ReturnRef(smaller));

    const auto wide = glm::perspective(glm::radians(45.0f), 1920.0f / 1080.0f, 0.1f, 1000.0f);
    const auto narrow = glm::perspective(glm::radians(45.0f), 800.0f / 600.0f, 0.1f, 1000.0f);
    {
      InSequence in_order;
      EXPECT_CALL(_render_context, DrawRenderObject(1, _, _, wide, _));
      EXPECT_CALL(_render_context, DrawRenderObject(1, _, _, narrow, _));
    }

    _pipeline.EnqueueForRendering(1, At(0.0f, 0.0f, 0.0f));
    _pipeline.RenderFrame();
    _pipeline.EnqueueForRendering(1, At(0.0f, 0.0f, 0.0f));
    _pipeline.RenderFrame();
  }

  TEST_F(ForwardRenderPipelineTest, KeepsTheCameraFromFrameToFrame)
  {
    CameraInfo camera;
    camera.view = translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -5.0f));
    _pipeline.SetCameraInfo(camera);

    EXPECT_CALL(_render_context, DrawRenderObject(1, _, camera.view, _, _)).Times(2);

    _pipeline.EnqueueForRendering(1, At(0.0f, 0.0f, 0.0f));
    _pipeline.RenderFrame();
    _pipeline.EnqueueForRendering(1, At(0.0f, 0.0f, 0.0f));
    _pipeline.RenderFrame();
  }

  TEST_F(ForwardRenderPipelineTest, DrawsEveryObjectWithEveryLight)
  {
    _pipeline.EnqueueLightSource(Light("sun"));
    _pipeline.EnqueueLightSource(Light("lamp"));
    _pipeline.EnqueueForRendering(1, At(0.0f, 0.0f, 0.0f));
    _pipeline.EnqueueForRendering(2, At(0.0f, 0.0f, 0.0f));

    EXPECT_CALL(_render_context, DrawRenderObject(1, _, _, _, ElementsAre(HasId("sun"), HasId("lamp"))));
    EXPECT_CALL(_render_context, DrawRenderObject(2, _, _, _, ElementsAre(HasId("sun"), HasId("lamp"))));

    _pipeline.RenderFrame();
  }

  TEST_F(ForwardRenderPipelineTest, DrawsWithALightThatWasEnqueuedAfterTheObject)
  {
    _pipeline.EnqueueForRendering(1, At(0.0f, 0.0f, 0.0f));
    _pipeline.EnqueueLightSource(Light("sun"));

    EXPECT_CALL(_render_context, DrawRenderObject(1, _, _, _, ElementsAre(HasId("sun"))));

    _pipeline.RenderFrame();
  }

  TEST_F(ForwardRenderPipelineTest, DrawsWithoutLightsWhenNoneWereEnqueued)
  {
    _pipeline.EnqueueForRendering(1, At(0.0f, 0.0f, 0.0f));

    EXPECT_CALL(_render_context, DrawRenderObject(1, _, _, _, IsEmpty()));

    _pipeline.RenderFrame();
  }

  TEST_F(ForwardRenderPipelineTest, UsesALightInTheFrameItWasEnqueuedForOnly)
  {
    _pipeline.EnqueueLightSource(Light("sun"));
    _pipeline.EnqueueForRendering(1, At(0.0f, 0.0f, 0.0f));
    EXPECT_CALL(_render_context, DrawRenderObject(1, _, _, _, SizeIs(1)));
    _pipeline.RenderFrame();

    _pipeline.EnqueueForRendering(1, At(0.0f, 0.0f, 0.0f));
    EXPECT_CALL(_render_context, DrawRenderObject(1, _, _, _, IsEmpty()));
    _pipeline.RenderFrame();
  }

  TEST_F(ForwardRenderPipelineTest, ForgetsTheLightsOfAFrameInWhichNothingWasDrawn)
  {
    _pipeline.EnqueueLightSource(Light("sun"));
    _pipeline.RenderFrame();

    _pipeline.EnqueueForRendering(1, At(0.0f, 0.0f, 0.0f));
    EXPECT_CALL(_render_context, DrawRenderObject(1, _, _, _, IsEmpty()));
    _pipeline.RenderFrame();
  }

  TEST_F(ForwardRenderPipelineTest, AcceptsAsManyLightsAsItWasCreatedFor)
  {
    _pipeline.EnqueueLightSource(Light("one"));
    _pipeline.EnqueueLightSource(Light("two"));
    _pipeline.EnqueueLightSource(Light("three"));
    _pipeline.EnqueueForRendering(1, At(0.0f, 0.0f, 0.0f));

    EXPECT_CALL(
      _render_context,
      DrawRenderObject(1, _, _, _, ElementsAre(HasId("one"), HasId("two"), HasId("three"))));

    _pipeline.RenderFrame();

    EXPECT_EQ(_logger->Count(LogLevel::Warn), 0u);
  }

  TEST_F(ForwardRenderPipelineTest, LeavesOutTheLightsBeyondWhatItWasCreatedForAndWarns)
  {
    _pipeline.EnqueueLightSource(Light("one"));
    _pipeline.EnqueueLightSource(Light("two"));
    _pipeline.EnqueueLightSource(Light("three"));
    _pipeline.EnqueueLightSource(Light("four"));
    _pipeline.EnqueueLightSource(Light("five"));
    _pipeline.EnqueueForRendering(1, At(0.0f, 0.0f, 0.0f));

    EXPECT_CALL(
      _render_context,
      DrawRenderObject(1, _, _, _, ElementsAre(HasId("one"), HasId("two"), HasId("three"))));

    _pipeline.RenderFrame();

    EXPECT_EQ(_logger->Count(LogLevel::Warn), 2u);
    EXPECT_TRUE(_logger->Contains(LogLevel::Warn, "Cannot enqueue light source four"));
    EXPECT_TRUE(_logger->Contains(LogLevel::Warn, "Cannot enqueue light source five"));
  }

  TEST_F(ForwardRenderPipelineTest, AcceptsLightsAgainInTheNextFrame)
  {
    for (const char *id : {"one", "two", "three", "four"}) { _pipeline.EnqueueLightSource(Light(id)); }
    _pipeline.RenderFrame();
    _logger->Clear();

    for (const char *id : {"five", "six", "seven"}) { _pipeline.EnqueueLightSource(Light(id)); }
    _pipeline.EnqueueForRendering(1, At(0.0f, 0.0f, 0.0f));

    EXPECT_CALL(
      _render_context,
      DrawRenderObject(1, _, _, _, ElementsAre(HasId("five"), HasId("six"), HasId("seven"))));
    _pipeline.RenderFrame();

    EXPECT_EQ(_logger->Count(LogLevel::Warn), 0u);
  }

  TEST_F(ForwardRenderPipelineTest, AcceptsNoLightWhenCreatedForNone)
  {
    Forward_RenderPipeline pipeline(&_render_context, 0, _logger);

    pipeline.EnqueueLightSource(Light("sun"));
    pipeline.EnqueueForRendering(1, At(0.0f, 0.0f, 0.0f));

    EXPECT_CALL(_render_context, DrawRenderObject(1, _, _, _, IsEmpty()));
    pipeline.RenderFrame();

    EXPECT_EQ(_logger->Count(LogLevel::Warn), 1u);
  }
}
