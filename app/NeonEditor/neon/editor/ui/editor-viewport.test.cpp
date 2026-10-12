#include "editor-viewport.hpp"

#include <memory>
#include <string>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <neon/input/key.hpp>
#include <neon/testing/mock-input-system.hpp>
#include <neon/testing/mock-render-context.hpp>
#include <neon/testing/mock-render-pipeline.hpp>
#include <neon/testing/mock-window-system.hpp>
#include <neon/testing/recording-logger.hpp>
#include <neon/testing/recording-tool-ui.hpp>

namespace
{
  using neon::EditorCamera;
  using neon::EditorViewport;
  using neon::Key;
  using neon::MouseButton;
  using neon::No_Render_Target;
  using neon::RenderTargetOptions;
  using neon::ToolUiSize;
  using neon::WindowMetrics;
  using neon::testing::FakeInputContext;
  using neon::testing::MockRenderContext;
  using neon::testing::MockRenderPipeline;
  using neon::testing::MockWindowContext;
  using neon::testing::RecordingLogger;
  using neon::testing::RecordingToolUi;
  using ::testing::_;
  using ::testing::Contains;
  using ::testing::Field;
  using ::testing::NiceMock;
  using ::testing::Not;
  using ::testing::Return;

  /// A window of one pixel a point, and a viewport on it. The density is
  /// worked out by PixelsOf(), which has a test of its own.
  class EditorViewportTest : public ::testing::Test
  {
  protected:
    std::shared_ptr<RecordingLogger> _logger = std::make_shared<RecordingLogger>();
    NiceMock<MockRenderContext> _renderer{_logger};
    NiceMock<MockRenderPipeline> _pipeline{_logger, &_renderer};
    NiceMock<MockWindowContext> _window;
    FakeInputContext _input{_logger};
    EditorCamera _camera;
    RecordingToolUi _ui;
    EditorViewport _viewport{&_renderer, &_pipeline, &_window, &_input, &_camera, _logger};

    void SetUp() override
    {
      ON_CALL(_window, GetDrawableSize()).WillByDefault(Return(neon::WindowSize{1280, 720}));
      ON_CALL(_window, GetDeltaTime()).WillByDefault(Return(1.0));
      _camera.StartAt({0.0f, 0.0f, 0.0f}, neon::Rotation{});
    }
  };

  TEST(EditorViewportPixels, AreWholePixelsOfTheRoomAndNeverBelowOne)
  {
    const WindowMetrics retina{1280, 720, 2560, 1440};
    const auto [width, height] = EditorViewport::PixelsOf({640.5f, 360.25f}, retina);
    EXPECT_EQ(width, 1281);
    EXPECT_EQ(height, 721);

    const auto [least_width, least_height] = EditorViewport::PixelsOf({0.0f, 0.0f}, retina);
    EXPECT_EQ(least_width, 1);
    EXPECT_EQ(least_height, 1);
  }

  TEST_F(EditorViewportTest, DrawsIntoTheFrameUntilTheWindowHadRoom)
  {
    EXPECT_CALL(_pipeline, SetFrameTarget(No_Render_Target));
    EXPECT_CALL(_renderer, CreateRenderTarget(_, _, _, _)).Times(0);

    _viewport.Prepare();

    EXPECT_EQ(_viewport.GetTarget(), No_Render_Target);
  }

  TEST_F(EditorViewportTest, MakesTheTargetTheSizeOfTheRoomInPixelsAndShowsIt)
  {
    _ui.available = {640.0f, 360.0f};
    _viewport.Draw(_ui);

    // the window is drawn without an image in the first frame, and the
    // room is measured
    EXPECT_THAT(_ui.calls, Contains("begin-window Viewport"));
    EXPECT_THAT(_ui.calls, Not(Contains(::testing::StartsWith("image"))));

    EXPECT_CALL(
      _renderer,
      CreateRenderTarget(
        std::string(EditorViewport::name), 640, 360, Field(&RenderTargetOptions::scales, false)))
      .WillOnce(Return(3));
    EXPECT_CALL(_pipeline, SetFrameTarget(3));
    _viewport.Prepare();
    EXPECT_EQ(_viewport.GetTarget(), 3);

    _ui.calls.clear();
    _viewport.Draw(_ui);
    EXPECT_THAT(_ui.calls, Contains("image 3 640x360"));
  }

  TEST_F(EditorViewportTest, MakesTheTargetAgainWhenTheRoomChanges)
  {
    ON_CALL(_renderer, CreateRenderTarget(_, _, _, _)).WillByDefault(Return(3));
    _viewport.Draw(_ui);
    _viewport.Prepare();

    // the same room keeps the target
    EXPECT_CALL(_renderer, DestroyRenderTarget(_)).Times(0);
    _viewport.Draw(_ui);
    _viewport.Prepare();
    ::testing::Mock::VerifyAndClearExpectations(&_renderer);

    _ui.available = {320.0f, 200.0f};
    _viewport.Draw(_ui);

    EXPECT_CALL(_renderer, DestroyRenderTarget(3));
    EXPECT_CALL(_renderer, CreateRenderTarget(_, 320, 200, _)).WillOnce(Return(4));
    EXPECT_CALL(_pipeline, SetFrameTarget(4));
    _viewport.Prepare();
    EXPECT_EQ(_viewport.GetTarget(), 4);
  }

  TEST_F(EditorViewportTest, ACollapsedWindowLeavesTheFrameToThePipeline)
  {
    ON_CALL(_renderer, CreateRenderTarget(_, _, _, _)).WillByDefault(Return(3));
    _viewport.Draw(_ui);
    _viewport.Prepare();

    _ui.windows_open = false;
    _viewport.Draw(_ui);

    EXPECT_CALL(_pipeline, SetFrameTarget(No_Render_Target));
    _viewport.Prepare();
  }

  TEST_F(EditorViewportTest, FreesTheTargetWhenCleanedUp)
  {
    ON_CALL(_renderer, CreateRenderTarget(_, _, _, _)).WillByDefault(Return(3));
    _viewport.Draw(_ui);
    _viewport.Prepare();

    EXPECT_CALL(_renderer, DestroyRenderTarget(3));
    EXPECT_CALL(_pipeline, SetFrameTarget(No_Render_Target));
    _viewport.CleanUp();
    EXPECT_EQ(_viewport.GetTarget(), No_Render_Target);
  }

  TEST_F(EditorViewportTest, TheKeysFlyTheCameraWhileThePointerIsOverTheWindow)
  {
    _input.state.SetKeyDown(Key::W);

    // over another window, nothing moves
    _viewport.Draw(_ui);
    EXPECT_EQ(_camera.GetPosition().z, 0.0f);

    _ui.hovered = true;
    _viewport.Draw(_ui);
    EXPECT_NEAR(_camera.GetPosition().z, -_camera.GetSpeed(), 1e-4f);

    // a field of the tools takes the keys
    _ui.wants_keyboard = true;
    const float before = _camera.GetPosition().z;
    _viewport.Draw(_ui);
    EXPECT_EQ(_camera.GetPosition().z, before);
  }

  TEST_F(EditorViewportTest, TheRightButtonHoldsTheViewWithThePointerHiddenUntilItIsReleased)
  {
    _ui.hovered = true;
    _input.state.SetMouseButtonDown(MouseButton::Right);
    _input.state.AddMouseMotion(10.0, 0.0);

    EXPECT_CALL(_input, CenterAndHideCursor()).Times(1);
    _viewport.Draw(_ui);
    EXPECT_TRUE(_viewport.IsLooking());
    EXPECT_LT(_camera.GetRotation().yaw, 0.0f);

    // held while the pointer leaves the window, and not taken twice
    _ui.hovered = false;
    _viewport.Draw(_ui);
    EXPECT_TRUE(_viewport.IsLooking());

    _input.state.Reset();
    EXPECT_CALL(_input, ShowCursor()).Times(1);
    _viewport.Draw(_ui);
    EXPECT_FALSE(_viewport.IsLooking());
  }

  TEST_F(EditorViewportTest, TheRightButtonOverAnotherWindowDoesNotTakeTheView)
  {
    _input.state.SetMouseButtonDown(MouseButton::Right);
    EXPECT_CALL(_input, CenterAndHideCursor()).Times(0);

    _viewport.Draw(_ui);

    EXPECT_FALSE(_viewport.IsLooking());
  }

  TEST_F(EditorViewportTest, TheWheelOverTheWindowChangesTheSpeed)
  {
    const float before = _camera.GetSpeed();
    _input.state.AddWheel(0.0, -1.0, false);

    _viewport.Draw(_ui);
    EXPECT_EQ(_camera.GetSpeed(), before);

    _ui.hovered = true;
    _viewport.Draw(_ui);
    EXPECT_GT(_camera.GetSpeed(), before);
  }

  TEST_F(EditorViewportTest, AMotionTurnsTheViewOnceAndAPointerHeldStillTurnsNothing)
  {
    _ui.hovered = true;
    _input.state.SetMouseButtonDown(MouseButton::Right);
    _input.state.AddMouseMotion(20.0, 0.0);
    EXPECT_CALL(_input, CenterAndHideCursor()).Times(1);

    _viewport.Draw(_ui);
    const float turned = _camera.GetRotation().yaw;
    EXPECT_NEAR(turned, -20.0f * EditorCamera::look_degrees_per_pixel, 1e-4f);

    // two frames with the button held and the mouse still, as a frame
    // begins: the motion is forgotten with the frame, the button stays
    for (int frame = 0; frame < 2; frame++)
    {
      _input.state.Reset();
      _input.state.SetMouseButtonDown(MouseButton::Right);
      _viewport.Draw(_ui);
      EXPECT_EQ(_camera.GetRotation().yaw, turned);
    }
  }
}
