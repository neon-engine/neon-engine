#include "runtime.hpp"

#include <memory>
#include <string>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <neon/input/input-state.hpp>
#include <neon/testing/mock-input-system.hpp>
#include <neon/testing/mock-render-pipeline.hpp>
#include <neon/testing/mock-render-system.hpp>
#include <neon/testing/mock-ui-system.hpp>
#include <neon/testing/mock-window-system.hpp>
#include <neon/testing/mock-world-system.hpp>
#include <neon/testing/recording-logger.hpp>

namespace
{
  using neon::Action;
  using neon::InputState;
  using neon::testing::FakeInputContext;
  using neon::testing::LogLevel;
  using neon::testing::MockInputSystem;
  using neon::testing::MockRenderPipeline;
  using neon::testing::MockRenderSystem;
  using neon::testing::MockUiSystem;
  using neon::testing::MockWindowSystem;
  using neon::testing::MockWorldSystem;
  using neon::testing::RecordingLogger;
  using ::testing::_;
  using ::testing::AnyNumber;
  using ::testing::InSequence;
  using ::testing::Invoke;
  using ::testing::NiceMock;
  using ::testing::Return;
  using ::testing::ReturnRef;
  using ::testing::StrictMock;

  /// Runtime is meant to be derived from, its constructor is protected.
  class TestRuntime final : public neon::Runtime
  {
  public:
    TestRuntime(
      const SettingsConfig &settings_config,
      neon::WindowSystem *window_system,
      neon::InputSystem *input_system,
      neon::RenderSystem *render_system,
      neon::RenderPipeline *render_pipeline,
      neon::WorldSystem *world_system,
      const std::shared_ptr<neon::Logger> &logger)
      : Runtime(
        settings_config,
        window_system,
        input_system,
        render_system,
        render_pipeline,
        // the runtime keeps the logging system and never calls it
        nullptr,
        world_system,
        logger) {}
  };

  class RuntimeTest : public ::testing::Test
  {
  protected:
    std::shared_ptr<RecordingLogger> _logger = std::make_shared<RecordingLogger>();
    StrictMock<MockWindowSystem> _window_system{_logger};
    StrictMock<MockInputSystem> _input_system{_logger};
    StrictMock<MockRenderSystem> _render_system{_logger};
    StrictMock<MockRenderPipeline> _render_pipeline{_logger};
    StrictMock<MockWorldSystem> _world_system{_logger};
    StrictMock<MockUiSystem> _ui_system;

    // what the window was told, so that IsRunning can answer like a window
    bool _closed = false;

    std::unique_ptr<TestRuntime> Create(const SettingsConfig &settings)
    {
      return std::make_unique<TestRuntime>(
        settings,
        &_window_system,
        &_input_system,
        &_render_system,
        &_render_pipeline,
        &_world_system,
        _logger);
    }

    void ExpectInitialize()
    {
      InSequence in_order;
      EXPECT_CALL(_window_system, Initialize());
      EXPECT_CALL(_input_system, Initialize());
      EXPECT_CALL(_render_system, Initialize());
      EXPECT_CALL(_render_pipeline, Initialize());
      EXPECT_CALL(_world_system, Initialize());
    }

    void ExpectCleanUp()
    {
      InSequence in_order;
      EXPECT_CALL(_world_system, CleanUp());
      EXPECT_CALL(_render_pipeline, CleanUp());
      EXPECT_CALL(_render_system, CleanUp());
      EXPECT_CALL(_input_system, CleanUp());
      EXPECT_CALL(_window_system, CleanUp());
    }

    /// Lets the window behave like one: it runs until it is told to close.
    void LetTheWindowRunUntilItIsClosed()
    {
      EXPECT_CALL(_window_system, IsRunning()).WillRepeatedly(Invoke([this] { return !_closed; }));
      EXPECT_CALL(_window_system, SignalToClose()).Times(AnyNumber()).WillRepeatedly(
        Invoke([this] { _closed = true; }));
    }

    /// Expects the calls of a number of frames, and nothing beyond them.
    void ExpectFrames(const int frames)
    {
      EXPECT_CALL(_input_system, ProcessInput()).Times(frames);
      EXPECT_CALL(_render_system, PrepareFrame()).Times(frames);
      EXPECT_CALL(_world_system, Update()).Times(frames);
      EXPECT_CALL(_render_system, FinishFrame()).Times(frames);
      EXPECT_CALL(_window_system, Update()).Times(frames);
    }
  };

  // with a user interface

  TEST_F(RuntimeTest, TouchesNoUserInterfaceWhenItIsGivenOne)
  {
    const auto runtime = Create({});
    runtime->SetUiSystem(&_ui_system);

    {
      InSequence in_order;
      EXPECT_CALL(_world_system, CleanUp());
      EXPECT_CALL(_ui_system, CleanUp());
      EXPECT_CALL(_render_pipeline, CleanUp());
    }
    EXPECT_CALL(_render_system, CleanUp());
    EXPECT_CALL(_input_system, CleanUp());
    EXPECT_CALL(_window_system, CleanUp());
  }

  TEST_F(RuntimeTest, InitializesTheUserInterfaceBetweenTheRendererAndTheWorld)
  {
    const auto runtime = Create({});
    runtime->SetUiSystem(&_ui_system);

    {
      InSequence in_order;
      EXPECT_CALL(_window_system, Initialize());
      EXPECT_CALL(_input_system, Initialize());
      EXPECT_CALL(_render_system, Initialize());
      EXPECT_CALL(_render_pipeline, Initialize());
      // the renderer is there for its textures, and the scene of the world
      // may name a user interface to show
      EXPECT_CALL(_ui_system, Initialize());
      EXPECT_CALL(_world_system, Initialize());
    }
    runtime->Initialize();

    {
      InSequence in_order;
      EXPECT_CALL(_world_system, CleanUp());
      EXPECT_CALL(_ui_system, CleanUp());
      EXPECT_CALL(_render_pipeline, CleanUp());
      EXPECT_CALL(_render_system, CleanUp());
      EXPECT_CALL(_input_system, CleanUp());
      EXPECT_CALL(_window_system, CleanUp());
    }
  }

  TEST_F(RuntimeTest, UpdatesTheUserInterfaceBeforeTheWorldAndDrawsItAfterwards)
  {
    const auto runtime = Create({.max_frames = 2});
    runtime->SetUiSystem(&_ui_system);

    ExpectInitialize();
    EXPECT_CALL(_ui_system, Initialize());
    LetTheWindowRunUntilItIsClosed();
    {
      InSequence in_order;
      for (int frame = 0; frame < 2; frame++)
      {
        EXPECT_CALL(_input_system, ProcessInput());
        // it sees the input first, and takes what it uses from the world
        EXPECT_CALL(_ui_system, Update());
        EXPECT_CALL(_render_system, PrepareFrame());
        EXPECT_CALL(_world_system, Update());
        // on top of the world, and into the frame that is saved
        EXPECT_CALL(_ui_system, Draw());
        EXPECT_CALL(_render_system, FinishFrame());
        EXPECT_CALL(_window_system, Update());
      }
    }

    runtime->Run();

    ExpectCleanUp();
    EXPECT_CALL(_ui_system, CleanUp());
  }

  TEST_F(RuntimeTest, DrawsTheUserInterfaceIntoTheFrameThatIsSaved)
  {
    const auto runtime = Create({.max_frames = 1, .screenshot_path = "output://frame.png"});
    runtime->SetUiSystem(&_ui_system);

    ExpectInitialize();
    EXPECT_CALL(_ui_system, Initialize());
    LetTheWindowRunUntilItIsClosed();
    EXPECT_CALL(_input_system, ProcessInput());
    EXPECT_CALL(_ui_system, Update());
    EXPECT_CALL(_render_system, PrepareFrame());
    EXPECT_CALL(_world_system, Update());
    EXPECT_CALL(_window_system, Update());
    {
      InSequence in_order;
      EXPECT_CALL(_ui_system, Draw());
      EXPECT_CALL(_render_system, FinishFrame());
      EXPECT_CALL(_render_system, CaptureFrame("output://frame.png")).WillOnce(Return(true));
    }

    runtime->Run();

    EXPECT_FALSE(runtime->HasFailed());
    ExpectCleanUp();
    EXPECT_CALL(_ui_system, CleanUp());
  }

  TEST_F(RuntimeTest, RunsWithoutAUserInterfaceOnceItIsTakenAway)
  {
    const auto runtime = Create({.max_frames = 1});
    runtime->SetUiSystem(&_ui_system);
    runtime->SetUiSystem(nullptr);

    // the mock is strict, a call to it fails the test
    ExpectInitialize();
    LetTheWindowRunUntilItIsClosed();
    ExpectFrames(1);

    runtime->Run();

    ExpectCleanUp();
  }

  TEST_F(RuntimeTest, TouchesNoSystemWhenItIsCreated)
  {
    const auto runtime = Create({});

    EXPECT_FALSE(runtime->HasFailed());

    ExpectCleanUp();
  }

  TEST_F(RuntimeTest, InitializesTheWindowFirstAndTheWorldLast)
  {
    const auto runtime = Create({});

    ExpectInitialize();
    runtime->Initialize();

    ExpectCleanUp();
  }

  TEST_F(RuntimeTest, CleansUpInTheOppositeOrder)
  {
    const auto runtime = Create({});

    ExpectCleanUp();
    runtime->CleanUp();
  }

  TEST_F(RuntimeTest, CleansUpOnceWhenAskedTwice)
  {
    const auto runtime = Create({});

    ExpectCleanUp();
    runtime->CleanUp();
    runtime->CleanUp();
  }

  TEST_F(RuntimeTest, CleansUpWhenItIsDestroyed)
  {
    auto runtime = Create({});

    ExpectCleanUp();
    runtime.reset();

    ::testing::Mock::VerifyAndClearExpectations(&_world_system);
    ::testing::Mock::VerifyAndClearExpectations(&_window_system);
  }

  TEST_F(RuntimeTest, DoesNotCleanUpAgainWhenItIsDestroyed)
  {
    auto runtime = Create({});

    ExpectCleanUp();
    runtime->CleanUp();
    ::testing::Mock::VerifyAndClearExpectations(&_world_system);
    ::testing::Mock::VerifyAndClearExpectations(&_render_pipeline);
    ::testing::Mock::VerifyAndClearExpectations(&_render_system);
    ::testing::Mock::VerifyAndClearExpectations(&_input_system);
    ::testing::Mock::VerifyAndClearExpectations(&_window_system);

    // the mocks are strict, so a call from here on fails the test
    runtime.reset();
  }

  TEST_F(RuntimeTest, RunsNoFrameWhenTheWindowIsClosedFromTheStart)
  {
    const auto runtime = Create({});

    ExpectInitialize();
    EXPECT_CALL(_window_system, IsRunning()).WillOnce(Return(false));
    ExpectFrames(0);

    runtime->Run();

    EXPECT_FALSE(runtime->HasFailed());
    ExpectCleanUp();
  }

  TEST_F(RuntimeTest, InitializesBeforeTheFirstFrame)
  {
    const auto runtime = Create({.max_frames = 1});

    {
      InSequence in_order;
      EXPECT_CALL(_world_system, Initialize());
      EXPECT_CALL(_input_system, ProcessInput());
    }
    EXPECT_CALL(_window_system, Initialize());
    EXPECT_CALL(_input_system, Initialize());
    EXPECT_CALL(_render_system, Initialize());
    EXPECT_CALL(_render_pipeline, Initialize());
    EXPECT_CALL(_render_system, PrepareFrame());
    EXPECT_CALL(_world_system, Update());
    EXPECT_CALL(_render_system, FinishFrame());
    EXPECT_CALL(_window_system, Update());
    LetTheWindowRunUntilItIsClosed();

    runtime->Run();

    ExpectCleanUp();
  }

  TEST_F(RuntimeTest, RunsAFrameInOrder)
  {
    const auto runtime = Create({.max_frames = 1});

    ExpectInitialize();
    LetTheWindowRunUntilItIsClosed();
    {
      InSequence in_order;
      EXPECT_CALL(_input_system, ProcessInput());
      EXPECT_CALL(_render_system, PrepareFrame());
      EXPECT_CALL(_world_system, Update());
      EXPECT_CALL(_render_system, FinishFrame());
      EXPECT_CALL(_window_system, Update());
    }

    runtime->Run();

    ExpectCleanUp();
  }

  TEST_F(RuntimeTest, StopsAfterTheFramesItWasGiven)
  {
    const auto runtime = Create({.max_frames = 5});

    ExpectInitialize();
    LetTheWindowRunUntilItIsClosed();
    ExpectFrames(5);

    runtime->Run();

    EXPECT_TRUE(_closed);
    EXPECT_FALSE(runtime->HasFailed());
    EXPECT_TRUE(_logger->Contains(LogLevel::Info, "Rendered 5 frames, stopping"));
    ExpectCleanUp();
  }

  TEST_F(RuntimeTest, TellsTheWindowToCloseOnceAfterTheLastFrame)
  {
    const auto runtime = Create({.max_frames = 3});

    ExpectInitialize();
    ExpectFrames(3);
    EXPECT_CALL(_window_system, IsRunning()).WillRepeatedly(Invoke([this] { return !_closed; }));
    EXPECT_CALL(_window_system, SignalToClose()).WillOnce(Invoke([this] { _closed = true; }));

    runtime->Run();

    ExpectCleanUp();
  }

  TEST_F(RuntimeTest, RunsUntilTheWindowIsClosedWithoutANumberOfFrames)
  {
    const auto runtime = Create({});

    ExpectInitialize();
    EXPECT_CALL(_window_system, IsRunning())
      .WillOnce(Return(true))
      .WillOnce(Return(true))
      .WillOnce(Return(true))
      .WillOnce(Return(false));
    // the runtime itself never asks the window to close
    EXPECT_CALL(_window_system, SignalToClose()).Times(0);
    ExpectFrames(3);

    runtime->Run();

    EXPECT_FALSE(runtime->HasFailed());
    ExpectCleanUp();
  }

  TEST_F(RuntimeTest, StopsEarlierWhenTheWindowIsClosedBeforeTheLastFrame)
  {
    const auto runtime = Create({.max_frames = 10});

    ExpectInitialize();
    EXPECT_CALL(_window_system, IsRunning()).WillOnce(Return(true)).WillOnce(Return(true)).WillOnce(Return(false));
    ExpectFrames(2);

    runtime->Run();

    ExpectCleanUp();
  }

  TEST_F(RuntimeTest, DoesNotCleanUpAtTheEndOfTheRun)
  {
    const auto runtime = Create({.max_frames = 1});

    ExpectInitialize();
    LetTheWindowRunUntilItIsClosed();
    ExpectFrames(1);

    // the mocks are strict, a CleanUp during the run fails the test
    runtime->Run();

    ExpectCleanUp();
  }

  TEST_F(RuntimeTest, SavesTheLastFrameAfterItWasFinished)
  {
    const auto runtime = Create({.max_frames = 2, .screenshot_path = "output://frame.png"});

    ExpectInitialize();
    LetTheWindowRunUntilItIsClosed();
    EXPECT_CALL(_input_system, ProcessInput()).Times(2);
    EXPECT_CALL(_render_system, PrepareFrame()).Times(2);
    EXPECT_CALL(_world_system, Update()).Times(2);
    EXPECT_CALL(_window_system, Update()).Times(2);
    {
      InSequence in_order;
      EXPECT_CALL(_render_system, FinishFrame()).Times(2);
      EXPECT_CALL(_render_system, CaptureFrame("output://frame.png")).WillOnce(Return(true));
    }

    runtime->Run();

    EXPECT_FALSE(runtime->HasFailed());
    ExpectCleanUp();
  }

  TEST_F(RuntimeTest, SavesTheListedFrames)
  {
    const auto runtime = Create({
      .max_frames = 4,
      .screenshot_path = "output://frame.png",
      .screenshot_frames = {1, 3}
    });

    ExpectInitialize();
    LetTheWindowRunUntilItIsClosed();
    ExpectFrames(4);
    EXPECT_CALL(_render_system, CaptureFrame("output://frame-0001.png")).WillOnce(Return(true));
    EXPECT_CALL(_render_system, CaptureFrame("output://frame-0003.png")).WillOnce(Return(true));

    runtime->Run();

    EXPECT_FALSE(runtime->HasFailed());
    ExpectCleanUp();
  }

  TEST_F(RuntimeTest, HasFailedWhenAFrameCouldNotBeSaved)
  {
    const auto runtime = Create({.max_frames = 2, .screenshot_path = "output://frame.png"});

    ExpectInitialize();
    LetTheWindowRunUntilItIsClosed();
    ExpectFrames(2);
    EXPECT_CALL(_render_system, CaptureFrame("output://frame.png")).WillOnce(Return(false));

    runtime->Run();

    EXPECT_TRUE(runtime->HasFailed());
    ExpectCleanUp();
  }

  TEST_F(RuntimeTest, KeepsRunningAndStaysFailedAfterAFrameCouldNotBeSaved)
  {
    const auto runtime = Create({
      .max_frames = 3,
      .screenshot_path = "output://frame.png",
      .screenshot_frames = {1, 2}
    });

    ExpectInitialize();
    LetTheWindowRunUntilItIsClosed();
    ExpectFrames(3);
    EXPECT_CALL(_render_system, CaptureFrame("output://frame-0001.png")).WillOnce(Return(false));
    EXPECT_CALL(_render_system, CaptureFrame("output://frame-0002.png")).WillOnce(Return(true));

    runtime->Run();

    EXPECT_TRUE(runtime->HasFailed());
    ExpectCleanUp();
  }

  // the pause menu

  class PauseMenuTest : public RuntimeTest
  {
  protected:
    static constexpr const char *menu = "assets://ui/pause.ui.yml";
    static constexpr const char *settings = "assets://ui/settings.ui.yml";

    // what the devices say, and what the user interface leaves of it
    InputState _raw{_logger};
    NiceMock<FakeInputContext> _game{_logger};
    int _frame = 0;

    /// What each frame presses: the pause key as the devices see it, and
    /// whether the user interface let it through.
    std::function<void(int frame)> _press;

    std::unique_ptr<TestRuntime> CreateWithMenu(const int frames)
    {
      auto runtime = Create({
        .pause_menu = menu,
        .settings_menu = settings,
        .max_frames = static_cast<std::size_t>(frames)
      });
      runtime->SetUiSystem(&_ui_system);

      ExpectInitialize();
      EXPECT_CALL(_ui_system, Initialize());
      EXPECT_CALL(_ui_system, CleanUp());
      LetTheWindowRunUntilItIsClosed();

      EXPECT_CALL(_input_system, ProcessInput()).WillRepeatedly(Invoke([this]
      {
        _frame++;
        _raw.Reset();
        _game.state.Reset();
        if (_press) { _press(_frame); }
      }));
      EXPECT_CALL(_input_system, GetInputState()).WillRepeatedly(ReturnRef(_raw));
      EXPECT_CALL(_ui_system, GetGameInput()).WillRepeatedly(Return(&_game));
      EXPECT_CALL(_ui_system, Update()).Times(AnyNumber());
      EXPECT_CALL(_ui_system, Draw()).Times(AnyNumber());
      EXPECT_CALL(_ui_system, WasClicked(_)).WillRepeatedly(Return(false));
      EXPECT_CALL(_render_system, PrepareFrame()).Times(AnyNumber());
      EXPECT_CALL(_render_system, FinishFrame()).Times(AnyNumber());
      EXPECT_CALL(_world_system, Update()).Times(AnyNumber());
      EXPECT_CALL(_window_system, Update()).Times(AnyNumber());
      return runtime;
    }

    void PressPause()
    {
      _raw.SetAction(Action::Pause);
      _game.state.SetAction(Action::Pause);
    }
  };

  TEST_F(PauseMenuTest, ShowsTheMenuWhenPauseIsPressedAndHoldsTheWorldStill)
  {
    const auto runtime = CreateWithMenu(3);
    _press = [this](const int frame) { if (frame == 2) { PressPause(); } };

    EXPECT_CALL(_ui_system, Load(menu)).WillOnce(Return(7));
    EXPECT_CALL(_ui_system, IsShown(7)).WillRepeatedly(Return(true));
    {
      InSequence in_order;
      EXPECT_CALL(_world_system, SetPaused(false));
      EXPECT_CALL(_world_system, SetPaused(true)).Times(2);
    }

    runtime->Run();
    ExpectCleanUp();
  }

  TEST_F(PauseMenuTest, TakesTheMenuAwayWhenResumeIsChosen)
  {
    const auto runtime = CreateWithMenu(3);
    _press = [this](const int frame) { if (frame == 1) { PressPause(); } };

    EXPECT_CALL(_ui_system, Load(menu)).WillOnce(Return(7));
    EXPECT_CALL(_ui_system, IsShown(7)).WillRepeatedly(Return(true));
    EXPECT_CALL(_ui_system, WasClicked("resume")).WillOnce(Return(true)).WillRepeatedly(Return(false));
    EXPECT_CALL(_ui_system, Unload(7));
    {
      InSequence in_order;
      EXPECT_CALL(_world_system, SetPaused(true));
      EXPECT_CALL(_world_system, SetPaused(false)).Times(2);
    }

    runtime->Run();
    ExpectCleanUp();
  }

  TEST_F(PauseMenuTest, ShowsTheSettingsMenuInPlaceOfThePauseMenuAndKeepsTheWorldStill)
  {
    const auto runtime = CreateWithMenu(3);
    _press = [this](const int frame) { if (frame == 1) { PressPause(); } };

    EXPECT_CALL(_ui_system, Load(menu)).WillOnce(Return(7));
    EXPECT_CALL(_ui_system, IsShown(7)).WillRepeatedly(Return(true));
    EXPECT_CALL(_ui_system, WasClicked("settings")).WillOnce(Return(true));
    {
      InSequence in_order;
      EXPECT_CALL(_ui_system, Unload(7));
      EXPECT_CALL(_ui_system, Load(settings)).WillOnce(Return(8));
    }
    EXPECT_CALL(_ui_system, IsShown(8)).WillRepeatedly(Return(true));
    EXPECT_CALL(_world_system, SetPaused(true)).Times(3);

    runtime->Run();
    ExpectCleanUp();
  }

  TEST_F(PauseMenuTest, ShowsThePauseMenuAgainWhenTheSettingsMenuIsClosed)
  {
    const auto runtime = CreateWithMenu(4);
    _press = [this](const int frame) { if (frame == 1) { PressPause(); } };

    EXPECT_CALL(_ui_system, Load(menu)).WillOnce(Return(7)).WillOnce(Return(9));
    EXPECT_CALL(_ui_system, IsShown(7)).WillRepeatedly(Return(true));
    EXPECT_CALL(_ui_system, IsShown(9)).WillRepeatedly(Return(true));
    EXPECT_CALL(_ui_system, WasClicked("settings")).WillOnce(Return(true)).WillRepeatedly(Return(false));
    EXPECT_CALL(_ui_system, Unload(7));
    EXPECT_CALL(_ui_system, Load(settings)).WillOnce(Return(8));

    // closed by its own Back, or by cancel, in the frame after it was shown
    EXPECT_CALL(_ui_system, IsShown(8)).WillOnce(Return(false));
    EXPECT_CALL(_ui_system, Unload(8)).Times(0);

    EXPECT_CALL(_world_system, SetPaused(true)).Times(4);

    runtime->Run();
    ExpectCleanUp();
  }

  TEST_F(PauseMenuTest, KeepsThePauseMenuWhenTheSettingsMenuCannotBeShown)
  {
    const auto runtime = CreateWithMenu(3);
    _press = [this](const int frame) { if (frame == 1) { PressPause(); } };

    EXPECT_CALL(_ui_system, Load(menu)).WillOnce(Return(7)).WillOnce(Return(9));
    EXPECT_CALL(_ui_system, IsShown(7)).WillRepeatedly(Return(true));
    EXPECT_CALL(_ui_system, IsShown(9)).WillRepeatedly(Return(true));
    EXPECT_CALL(_ui_system, WasClicked("settings")).WillOnce(Return(true)).WillRepeatedly(Return(false));
    EXPECT_CALL(_ui_system, Unload(7));
    EXPECT_CALL(_ui_system, Load(settings)).WillOnce(Return(-1));
    EXPECT_CALL(_world_system, SetPaused(true)).Times(3);

    runtime->Run();
    ExpectCleanUp();

    EXPECT_TRUE(_logger->Contains(LogLevel::Error, "The settings menu assets://ui/settings.ui.yml cannot be shown"));
  }

  TEST_F(PauseMenuTest, ClosesTheWindowWhenQuitIsChosen)
  {
    const auto runtime = CreateWithMenu(10);
    _press = [this](const int frame) { if (frame == 1) { PressPause(); } };

    EXPECT_CALL(_ui_system, Load(menu)).WillOnce(Return(7));
    EXPECT_CALL(_ui_system, IsShown(7)).WillRepeatedly(Return(true));
    EXPECT_CALL(_ui_system, WasClicked("quit")).WillOnce(Return(true));
    EXPECT_CALL(_world_system, SetPaused(true)).Times(2);

    runtime->Run();

    EXPECT_EQ(_frame, 2);
    EXPECT_TRUE(_logger->Contains(LogLevel::Info, "Quit was chosen in the pause menu"));
    ExpectCleanUp();
  }

  TEST_F(PauseMenuTest, DoesNotShowTheMenuAgainWhileTheKeyThatClosedItIsStillHeld)
  {
    const auto runtime = CreateWithMenu(5);
    _press = [this](const int frame)
    {
      // pressed, and the menu closes itself on the second press, which it
      // takes: the game does not see it. Held on, let go, pressed again
      if (frame == 1) { PressPause(); }
      if (frame == 2 || frame == 3) { _raw.SetAction(Action::Pause); }
      if (frame == 5) { PressPause(); }
    };

    EXPECT_CALL(_ui_system, Load(menu)).WillOnce(Return(7)).WillOnce(Return(8));
    EXPECT_CALL(_ui_system, IsShown(7)).WillOnce(Return(false));
    EXPECT_CALL(_ui_system, IsShown(8)).WillRepeatedly(Return(true));
    EXPECT_CALL(_world_system, SetPaused(_)).Times(AnyNumber());

    runtime->Run();
    ExpectCleanUp();
  }

  TEST_F(PauseMenuTest, LeavesAPressAloneThatTheUserInterfaceUsed)
  {
    const auto runtime = CreateWithMenu(2);
    _press = [this](const int frame) { if (frame == 1) { _raw.SetAction(Action::Pause); } };

    EXPECT_CALL(_ui_system, Load(_)).Times(0);
    EXPECT_CALL(_world_system, SetPaused(false)).Times(2);

    runtime->Run();
    ExpectCleanUp();
  }

  TEST_F(PauseMenuTest, SaysWhenTheMenuCannotBeShown)
  {
    const auto runtime = CreateWithMenu(2);
    _press = [this](const int frame) { if (frame == 1) { PressPause(); } };

    EXPECT_CALL(_ui_system, Load(menu)).WillOnce(Return(-1));
    EXPECT_CALL(_world_system, SetPaused(false)).Times(2);

    runtime->Run();

    EXPECT_TRUE(_logger->Contains(LogLevel::Error, "The pause menu assets://ui/pause.ui.yml cannot be shown"));
    ExpectCleanUp();
  }

  TEST_F(RuntimeTest, LetsAnExceptionOfASystemThrough)
  {
    const auto runtime = Create({.max_frames = 3});

    ExpectInitialize();
    EXPECT_CALL(_window_system, IsRunning()).WillRepeatedly(Return(true));
    EXPECT_CALL(_input_system, ProcessInput());
    EXPECT_CALL(_render_system, PrepareFrame());
    EXPECT_CALL(_world_system, Update()).WillOnce(::testing::Throw(std::runtime_error("the world broke")));

    EXPECT_THROW(runtime->Run(), std::runtime_error);

    // what the application does afterwards still works
    ExpectCleanUp();
    runtime->CleanUp();
  }
}
