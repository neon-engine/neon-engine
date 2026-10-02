#include "sdl2-input-system.hpp"

#include <memory>

#include <gtest/gtest.h>

#include <neon/testing/recording-logger.hpp>
#include <neon/testing/sdl2-without-display.hpp>

// What SDL reports, as the engine sees it. SDL runs with its video driver
// that draws nowhere, and the events are made here.

namespace
{
  using neon::Action;
  using neon::Axis;
  using neon::InputState;
  using neon::Key;
  using neon::SDL2_InputSystem;
  using neon::WindowMetrics;
  using neon::testing::FakeWindow;
  using neon::testing::RecordingLogger;
  using neon::testing::Sdl2WithoutDisplay;

  class SDL2InputSystemTest : public Sdl2WithoutDisplay
  {
  protected:
    std::shared_ptr<RecordingLogger> _logger = std::make_shared<RecordingLogger>();
    FakeWindow _context;
    std::unique_ptr<SDL2_InputSystem> _input;

    void SetUp() override
    {
      Sdl2WithoutDisplay::SetUp();
      if (IsSkipped()) { return; }

      _input = std::make_unique<SDL2_InputSystem>(SettingsConfig{}, &_context, _logger);
      _input->Initialize();

      // a window that is used has the focus
      PushWindowEvent(SDL_WINDOWEVENT_FOCUS_GAINED);
      Frame();
    }

    void TearDown() override
    {
      if (_input != nullptr) { _input->CleanUp(); }
      Sdl2WithoutDisplay::TearDown();
    }

    const InputState &Frame() const
    {
      _input->ProcessInput();
      return _input->GetInputState();
    }
  };

  TEST_F(SDL2InputSystemTest, HasNothingPressedWithoutEvents)
  {
    const InputState &state = Frame();

    for (std::size_t action = 0; action < neon::kAction_Size; action++)
    {
      EXPECT_FALSE(state[static_cast<Action>(action)]) << action;
    }
    EXPECT_FALSE(state.HasPointer());
    EXPECT_TRUE(state.GetKeyEvents().empty());
    EXPECT_TRUE(state.GetText().empty());
    EXPECT_FALSE(_context.was_told_to_close);
  }

  TEST_F(SDL2InputSystemTest, TellsTheWindowOfTheFocus)
  {
    EXPECT_TRUE(_context.has_focus);

    PushWindowEvent(SDL_WINDOWEVENT_FOCUS_LOST);
    Frame();
    EXPECT_FALSE(_context.has_focus);
  }

  // keys

  TEST_F(SDL2InputSystemTest, HoldsAnActionFromTheKeyGoingDownToItGoingUp)
  {
    PushKey(true, SDL_SCANCODE_W);
    EXPECT_TRUE(Frame().IsKeyDown(Key::W));

    // no event, and the key is still held
    EXPECT_TRUE(Frame().IsKeyDown(Key::W));
    EXPECT_TRUE(Frame().IsKeyDown(Key::W));

    PushKey(false, SDL_SCANCODE_W);
    EXPECT_FALSE(Frame().IsKeyDown(Key::W));
  }

  TEST_F(SDL2InputSystemTest, MovesWithWASD)
  {
    PushKey(true, SDL_SCANCODE_W);
    PushKey(true, SDL_SCANCODE_A);
    PushKey(true, SDL_SCANCODE_S);
    PushKey(true, SDL_SCANCODE_D);

    // by where they are, for the input map; they move no user interface
    const InputState &state = Frame();
    EXPECT_TRUE(state.IsKeyDown(Key::W));
    EXPECT_TRUE(state.IsKeyDown(Key::A));
    EXPECT_TRUE(state.IsKeyDown(Key::S));
    EXPECT_TRUE(state.IsKeyDown(Key::D));
    EXPECT_FALSE(state[Action::Ui_Up]);
  }

  TEST_F(SDL2InputSystemTest, MovesThroughAUserInterfaceWithTheArrows)
  {
    PushKey(true, SDL_SCANCODE_UP);
    PushKey(true, SDL_SCANCODE_RIGHT);
    PushKey(true, SDL_SCANCODE_DOWN);
    PushKey(true, SDL_SCANCODE_LEFT);

    const InputState &state = Frame();
    EXPECT_TRUE(state[Action::Ui_Up]);
    EXPECT_TRUE(state[Action::Ui_Right]);
    EXPECT_TRUE(state[Action::Ui_Down]);
    EXPECT_TRUE(state[Action::Ui_Left]);
    EXPECT_FALSE(state.IsKeyDown(Key::W));
  }

  TEST_F(SDL2InputSystemTest, AcceptsWithReturnEnterAndSpace)
  {
    for (const SDL_Scancode key : {SDL_SCANCODE_RETURN, SDL_SCANCODE_KP_ENTER, SDL_SCANCODE_SPACE})
    {
      PushKey(true, key);
      EXPECT_TRUE(Frame()[Action::Ui_Accept]) << key;

      PushKey(false, key);
      EXPECT_FALSE(Frame()[Action::Ui_Accept]) << key;
    }
  }

  TEST_F(SDL2InputSystemTest, CancelsWithBackspace)
  {
    PushKey(true, SDL_SCANCODE_BACKSPACE);
    EXPECT_TRUE(Frame()[Action::Ui_Cancel]);
  }

  TEST_F(SDL2InputSystemTest, CancelsAndAsksToPauseOnEscape)
  {
    PushKey(true, SDL_SCANCODE_ESCAPE);
    const InputState &state = Frame();

    EXPECT_TRUE(state[Action::Ui_Cancel]);
    EXPECT_TRUE(state.IsKeyDown(Key::Escape));
    EXPECT_TRUE(_input->IsActionDown("pause")) << "escape is bound to pause in the default map";
    EXPECT_FALSE(_context.was_told_to_close);
  }

  TEST_F(SDL2InputSystemTest, TellsTheWindowToCloseOnShiftAndEscape)
  {
    PushKey(true, SDL_SCANCODE_LSHIFT, KMOD_LSHIFT);
    PushKey(true, SDL_SCANCODE_ESCAPE, KMOD_LSHIFT);
    Frame();

    EXPECT_TRUE(_context.was_told_to_close);
    EXPECT_FALSE(_input->IsActionDown("pause"));
  }

  TEST_F(SDL2InputSystemTest, StartsWithTheKeyboardAndTheMouseAsTheDevice)
  {
    EXPECT_EQ(Frame().GetDevice(), neon::InputDevice::KeyboardAndMouse);
  }

  TEST_F(SDL2InputSystemTest, TellsWhenAControllerWasUsedLastAndWhenTheMouseWas)
  {
    SDL_Event button{};
    button.type = SDL_CONTROLLERBUTTONDOWN;
    button.cbutton.button = SDL_CONTROLLER_BUTTON_A;
    Push(button);
    EXPECT_EQ(Frame().GetDevice(), neon::InputDevice::Gamepad);

    // it stays until another device is used
    EXPECT_EQ(Frame().GetDevice(), neon::InputDevice::Gamepad);

    SDL_Event motion{};
    motion.type = SDL_MOUSEMOTION;
    motion.motion.xrel = 3;
    Push(motion);
    EXPECT_EQ(Frame().GetDevice(), neon::InputDevice::KeyboardAndMouse);

    SDL_Event axis{};
    axis.type = SDL_CONTROLLERAXISMOTION;
    axis.caxis.axis = SDL_CONTROLLER_AXIS_LEFTX;
    axis.caxis.value = 20000;
    Push(axis);
    EXPECT_EQ(Frame().GetDevice(), neon::InputDevice::Gamepad);

    PushKey(true, SDL_SCANCODE_W);
    EXPECT_EQ(Frame().GetDevice(), neon::InputDevice::KeyboardAndMouse);
  }

  TEST_F(SDL2InputSystemTest, LeavesTheDeviceAloneForAStickThatRests)
  {
    SDL_Event axis{};
    axis.type = SDL_CONTROLLERAXISMOTION;
    axis.caxis.axis = SDL_CONTROLLER_AXIS_LEFTX;
    axis.caxis.value = 500;
    Push(axis);
    EXPECT_EQ(Frame().GetDevice(), neon::InputDevice::KeyboardAndMouse);
  }

  TEST_F(SDL2InputSystemTest, TellsTheWindowToCloseWhenSdlQuits)
  {
    SDL_Event event{};
    event.type = SDL_QUIT;
    Push(event);

    Frame();
    EXPECT_TRUE(_context.was_told_to_close);
  }

  TEST_F(SDL2InputSystemTest, TellsTheWindowToCloseWhenItIsClosed)
  {
    PushWindowEvent(SDL_WINDOWEVENT_CLOSE);
    Frame();
    EXPECT_TRUE(_context.was_told_to_close);
  }

  TEST_F(SDL2InputSystemTest, SaysThatAnActionIsHeldByTheKeyboard)
  {
    PushKey(true, SDL_SCANCODE_W);

    InputState state = Frame();
    EXPECT_TRUE(state.IsKeyDown(Key::W));

    // what takes the keyboard away while a text is typed
    state.ClearKeyboard();
    EXPECT_FALSE(state.IsKeyDown(Key::W));
  }

  TEST_F(SDL2InputSystemTest, ReleasesEveryKeyWhenTheWindowLosesTheFocus)
  {
    PushKey(true, SDL_SCANCODE_W);
    EXPECT_TRUE(Frame().IsKeyDown(Key::W));

    PushWindowEvent(SDL_WINDOWEVENT_FOCUS_LOST);
    EXPECT_FALSE(Frame().IsKeyDown(Key::W));

    // the key went up while another window had the focus
    PushWindowEvent(SDL_WINDOWEVENT_FOCUS_GAINED);
    EXPECT_FALSE(Frame().IsKeyDown(Key::W));
  }

  TEST_F(SDL2InputSystemTest, ReportsAKeyThatEditsOnceForEachEvent)
  {
    PushKey(true, SDL_SCANCODE_LEFT, KMOD_LSHIFT);
    PushKey(true, SDL_SCANCODE_LEFT, KMOD_LSHIFT, true);
    PushKey(false, SDL_SCANCODE_LEFT, KMOD_LSHIFT);

    const InputState &state = Frame();
    ASSERT_EQ(state.GetKeyEvents().size(), 3u);

    EXPECT_EQ(state.GetKeyEvents()[0].key, Key::Left);
    EXPECT_TRUE(state.GetKeyEvents()[0].is_down);
    EXPECT_FALSE(state.GetKeyEvents()[0].is_repeat);
    EXPECT_TRUE(state.GetKeyEvents()[0].modifiers.shift);

    EXPECT_TRUE(state.GetKeyEvents()[1].is_down);
    EXPECT_TRUE(state.GetKeyEvents()[1].is_repeat);

    EXPECT_FALSE(state.GetKeyEvents()[2].is_down);
    EXPECT_FALSE(state.GetKeyEvents()[2].is_repeat);

    // an event is of its frame
    EXPECT_TRUE(Frame().GetKeyEvents().empty());
  }

  TEST_F(SDL2InputSystemTest, KnowsTheKeysThatEdit)
  {
    const std::pair<SDL_Scancode, Key> keys[] = {
      {SDL_SCANCODE_LEFT, Key::Left},
      {SDL_SCANCODE_RIGHT, Key::Right},
      {SDL_SCANCODE_UP, Key::Up},
      {SDL_SCANCODE_DOWN, Key::Down},
      {SDL_SCANCODE_HOME, Key::Home},
      {SDL_SCANCODE_END, Key::End},
      {SDL_SCANCODE_PAGEUP, Key::PageUp},
      {SDL_SCANCODE_PAGEDOWN, Key::PageDown},
      {SDL_SCANCODE_BACKSPACE, Key::Backspace},
      {SDL_SCANCODE_DELETE, Key::Delete},
      {SDL_SCANCODE_RETURN, Key::Enter},
      {SDL_SCANCODE_KP_ENTER, Key::Enter},
      {SDL_SCANCODE_TAB, Key::Tab},
      {SDL_SCANCODE_SPACE, Key::Space},
      {SDL_SCANCODE_A, Key::A},
      {SDL_SCANCODE_C, Key::C},
      {SDL_SCANCODE_V, Key::V},
      {SDL_SCANCODE_X, Key::X},
      {SDL_SCANCODE_Y, Key::Y},
      {SDL_SCANCODE_Z, Key::Z}
    };

    for (const auto &[scancode, key] : keys)
    {
      PushKey(true, scancode);
      PushKey(false, scancode);

      const InputState &state = Frame();
      ASSERT_EQ(state.GetKeyEvents().size(), 2u) << scancode;
      EXPECT_EQ(state.GetKeyEvents()[0].key, key) << scancode;
    }
  }

  TEST_F(SDL2InputSystemTest, LeavesOutAKeyThatOnlyTypes)
  {
    PushKey(true, SDL_SCANCODE_Q);
    PushKey(true, SDL_SCANCODE_7);
    EXPECT_TRUE(Frame().GetKeyEvents().empty());
  }

  TEST_F(SDL2InputSystemTest, SaysWhatIsHeldNextToAKey)
  {
    PushKey(true, SDL_SCANCODE_C, KMOD_LCTRL);
    PushKey(true, SDL_SCANCODE_C, KMOD_LGUI);
    PushKey(true, SDL_SCANCODE_LEFT, KMOD_LALT);
    PushKey(true, SDL_SCANCODE_LEFT, KMOD_RSHIFT | KMOD_RCTRL);

    const InputState &state = Frame();
    ASSERT_EQ(state.GetKeyEvents().size(), 4u);

    EXPECT_TRUE(state.GetKeyEvents()[0].modifiers.control);
    EXPECT_FALSE(state.GetKeyEvents()[0].modifiers.super);

    EXPECT_TRUE(state.GetKeyEvents()[1].modifiers.super);
    EXPECT_FALSE(state.GetKeyEvents()[1].modifiers.control);

    EXPECT_TRUE(state.GetKeyEvents()[2].modifiers.alt);
    EXPECT_FALSE(state.GetKeyEvents()[2].modifiers.shift);

    EXPECT_TRUE(state.GetKeyEvents()[3].modifiers.shift);
    EXPECT_TRUE(state.GetKeyEvents()[3].modifiers.control);
  }

  TEST_F(SDL2InputSystemTest, MakesShortcutsWithTheKeyOfThePlatform)
  {
    PushKey(true, SDL_SCANCODE_C, KMOD_LCTRL);
    PushKey(true, SDL_SCANCODE_C, KMOD_LGUI);
    PushKey(true, SDL_SCANCODE_LEFT, KMOD_LALT);

    const InputState &state = Frame();
    ASSERT_EQ(state.GetKeyEvents().size(), 3u);

#if defined(__APPLE__)
    // command copies, and option moves by a word
    EXPECT_FALSE(state.GetKeyEvents()[0].modifiers.shortcut);
    EXPECT_TRUE(state.GetKeyEvents()[1].modifiers.shortcut);
    EXPECT_TRUE(state.GetKeyEvents()[2].modifiers.word);
    EXPECT_FALSE(state.GetKeyEvents()[0].modifiers.word);
#else
    // control does both
    EXPECT_TRUE(state.GetKeyEvents()[0].modifiers.shortcut);
    EXPECT_TRUE(state.GetKeyEvents()[0].modifiers.word);
    EXPECT_FALSE(state.GetKeyEvents()[1].modifiers.shortcut);
    EXPECT_FALSE(state.GetKeyEvents()[2].modifiers.word);
#endif
  }

  // the pointer

  TEST_F(SDL2InputSystemTest, HasNoPointerUntilItMoved)
  {
    EXPECT_FALSE(Frame().HasPointer());

    PushMotion(10, 20);
    EXPECT_TRUE(Frame().HasPointer());
  }

  TEST_F(SDL2InputSystemTest, LeavesThePointerWhereItIsAtADensityOfOne)
  {
    _context.metrics = WindowMetrics{1920, 1080, 1920, 1080};

    PushMotion(123, 456);

    const InputState &state = Frame();
    EXPECT_DOUBLE_EQ(state.GetPointer().x, 123.0);
    EXPECT_DOUBLE_EQ(state.GetPointer().y, 456.0);
  }

  TEST_F(SDL2InputSystemTest, PutsThePointerOfARetinaDisplayAtTwiceItsPlace)
  {
    // a window of 1440 by 900 points with 2880 by 1800 pixels
    _context.metrics = WindowMetrics{1440, 900, 2880, 1800};

    PushMotion(720, 450);

    const InputState &state = Frame();
    EXPECT_DOUBLE_EQ(state.GetPointer().x, 1440.0);
    EXPECT_DOUBLE_EQ(state.GetPointer().y, 900.0);
  }

  TEST_F(SDL2InputSystemTest, ScalesThePointerByOneAndAQuarter)
  {
    // a window of 1280 by 720 points with 1600 by 900 pixels
    _context.metrics = WindowMetrics{1280, 720, 1600, 900};

    PushMotion(100, 3);

    const InputState &state = Frame();
    EXPECT_DOUBLE_EQ(state.GetPointer().x, 125.0);
    EXPECT_DOUBLE_EQ(state.GetPointer().y, 3.75);
  }

  TEST_F(SDL2InputSystemTest, ScalesThePointerAlongEachSideByItself)
  {
    _context.metrics = WindowMetrics{1000, 500, 1500, 1000};

    PushMotion(100, 100);

    const InputState &state = Frame();
    EXPECT_DOUBLE_EQ(state.GetPointer().x, 150.0);
    EXPECT_DOUBLE_EQ(state.GetPointer().y, 200.0);
  }

  TEST_F(SDL2InputSystemTest, KeepsThePointerWhereItWasLastSeen)
  {
    PushMotion(10, 20);
    Frame();

    const InputState &state = Frame();
    EXPECT_TRUE(state.HasPointer());
    EXPECT_DOUBLE_EQ(state.GetPointer().x, 10.0);
    EXPECT_DOUBLE_EQ(state.GetPointer().y, 20.0);
  }

  TEST_F(SDL2InputSystemTest, FollowsTheWindowWhenItsDensityChanges)
  {
    _context.metrics = WindowMetrics{1440, 900, 1440, 900};
    PushMotion(100, 100);
    EXPECT_DOUBLE_EQ(Frame().GetPointer().x, 100.0);

    // moved to a display of high density, and the pointer has not moved
    _context.metrics = WindowMetrics{1440, 900, 2880, 1800};
    EXPECT_DOUBLE_EQ(Frame().GetPointer().x, 200.0);
  }

  TEST_F(SDL2InputSystemTest, HasNoPointerOutsideTheWindow)
  {
    PushMotion(10, 20);
    EXPECT_TRUE(Frame().HasPointer());

    PushWindowEvent(SDL_WINDOWEVENT_LEAVE);
    EXPECT_FALSE(Frame().HasPointer());

    PushMotion(30, 40);
    EXPECT_TRUE(Frame().HasPointer());
  }

  TEST_F(SDL2InputSystemTest, HasNoPointerWhileTheWindowHasNoFocus)
  {
    PushMotion(10, 20);
    PushWindowEvent(SDL_WINDOWEVENT_FOCUS_LOST);
    EXPECT_FALSE(Frame().HasPointer());
  }

  TEST_F(SDL2InputSystemTest, HasNoPointerWhileTheCursorIsHidden)
  {
    PushMotion(10, 20);
    EXPECT_TRUE(Frame().HasPointer());

    _input->CenterAndHideCursor();
    EXPECT_FALSE(Frame().HasPointer());

    _input->ShowCursor();
    EXPECT_TRUE(Frame().HasPointer());
  }

  TEST_F(SDL2InputSystemTest, HasNoPointerInAWindowWithoutASize)
  {
    _context.metrics = WindowMetrics{0, 0, 0, 0};
    PushMotion(10, 20);
    EXPECT_FALSE(Frame().HasPointer());
  }

  TEST_F(SDL2InputSystemTest, HoldsTheButtonFromItGoingDownToItGoingUp)
  {
    PushButton(true, 50, 60);

    const InputState &pressed = Frame();
    EXPECT_TRUE(pressed[Action::Pointer_Primary]);
    EXPECT_DOUBLE_EQ(pressed.GetPointer().x, 50.0);
    EXPECT_DOUBLE_EQ(pressed.GetPointer().y, 60.0);

    EXPECT_TRUE(Frame()[Action::Pointer_Primary]);

    PushButton(false, 50, 60);
    EXPECT_FALSE(Frame()[Action::Pointer_Primary]);
  }

  TEST_F(SDL2InputSystemTest, ScalesWhereTheButtonWentDown)
  {
    _context.metrics = WindowMetrics{1440, 900, 2880, 1800};
    PushButton(true, 50, 60);

    const InputState &state = Frame();
    EXPECT_DOUBLE_EQ(state.GetPointer().x, 100.0);
    EXPECT_DOUBLE_EQ(state.GetPointer().y, 120.0);
  }

  TEST_F(SDL2InputSystemTest, TakesOnlyTheFirstButtonForThePointer)
  {
    PushButton(true, 50, 60, SDL_BUTTON_RIGHT);
    PushButton(true, 50, 60, SDL_BUTTON_MIDDLE);
    EXPECT_FALSE(Frame()[Action::Pointer_Primary]);
  }

  TEST_F(SDL2InputSystemTest, TurnsTheViewByHowFarTheMouseMoved)
  {
    PushMotion(100, 100, 7, -3);

    const InputState &state = Frame();
    EXPECT_TRUE(state[Action::Mouse]);
    EXPECT_EQ(state[Axis::Mouse].x, 7.0);
    EXPECT_EQ(state[Axis::Mouse].y, -3.0);

    EXPECT_FALSE(Frame()[Action::Mouse]);
  }

  TEST_F(SDL2InputSystemTest, DoesNotTurnTheViewWithoutTheFocus)
  {
    PushWindowEvent(SDL_WINDOWEVENT_FOCUS_LOST);
    PushMotion(100, 100, 7, -3);
    EXPECT_FALSE(Frame()[Action::Mouse]);
  }

  // the wheel

  TEST_F(SDL2InputSystemTest, CountsTheWheelDownThePage)
  {
    // SDL counts up the page, so a notch towards the user is -1
    PushWheel(0.0f, -1.0f);

    const InputState &state = Frame();
    EXPECT_DOUBLE_EQ(state.GetWheel().x, 0.0);
    EXPECT_DOUBLE_EQ(state.GetWheel().y, 1.0);
    EXPECT_FALSE(state.IsWheelPrecise());

    EXPECT_DOUBLE_EQ(Frame().GetWheel().y, 0.0);
  }

  TEST_F(SDL2InputSystemTest, CountsTheWheelToTheRight)
  {
    PushWheel(2.0f, 0.0f);
    EXPECT_DOUBLE_EQ(Frame().GetWheel().x, 2.0);
  }

  TEST_F(SDL2InputSystemTest, AddsUpTheNotchesOfAFrame)
  {
    PushWheel(0.0f, -1.0f);
    PushWheel(0.0f, -2.0f);
    EXPECT_DOUBLE_EQ(Frame().GetWheel().y, 3.0);
  }

  TEST_F(SDL2InputSystemTest, TakesPartsOfANotchFromATrackpad)
  {
    PushWheel(0.25f, -0.5f);

    const InputState &state = Frame();
    EXPECT_DOUBLE_EQ(state.GetWheel().x, 0.25);
    EXPECT_DOUBLE_EQ(state.GetWheel().y, 0.5);
    EXPECT_TRUE(state.IsWheelPrecise());
  }

  TEST_F(SDL2InputSystemTest, TurnsAWheelAroundThatThePlatformFlipped)
  {
    PushWheel(1.0f, -1.0f, true);

    const InputState &state = Frame();
    EXPECT_DOUBLE_EQ(state.GetWheel().x, -1.0);
    EXPECT_DOUBLE_EQ(state.GetWheel().y, -1.0);
  }

  TEST_F(SDL2InputSystemTest, DoesNotScrollWithoutTheFocus)
  {
    PushWindowEvent(SDL_WINDOWEVENT_FOCUS_LOST);
    PushWheel(0.0f, -1.0f);
    EXPECT_DOUBLE_EQ(Frame().GetWheel().y, 0.0);
  }

  // text

  TEST_F(SDL2InputSystemTest, TakesTextAsItWasTyped)
  {
    PushText("a");
    PushText("\xC3\xA9");
    PushText("\xE6\x97\xA5\xE6\x9C\xAC");

    EXPECT_EQ(Frame().GetText(), "a\xC3\xA9\xE6\x97\xA5\xE6\x9C\xAC");
    EXPECT_EQ(Frame().GetText(), "");
  }

  TEST_F(SDL2InputSystemTest, DoesNotAskForTextUntilOneIsTyped)
  {
    EXPECT_EQ(SDL_IsTextInputActive(), SDL_FALSE);

    _input->StartTextInput({100, 200, 2, 20});
    EXPECT_EQ(SDL_IsTextInputActive(), SDL_TRUE);

    _input->StopTextInput();
    EXPECT_EQ(SDL_IsTextInputActive(), SDL_FALSE);
  }

  TEST_F(SDL2InputSystemTest, FollowsWhatAnInputMethodPutsTogether)
  {
    _input->StartTextInput({100, 200, 2, 20});

    PushComposition("\xE3\x81\xAB", 1, 0);
    const InputState &first = Frame();
    EXPECT_EQ(first.GetComposition().text, "\xE3\x81\xAB");
    EXPECT_EQ(first.GetComposition().cursor, 1);
    EXPECT_EQ(first.GetComposition().selection_length, 0);

    // it goes on in the frames in which nothing is typed
    EXPECT_EQ(Frame().GetComposition().text, "\xE3\x81\xAB");

    PushComposition("\xE3\x81\xAB\xE3\x81\xBB", 2, 0);
    EXPECT_EQ(Frame().GetComposition().text, "\xE3\x81\xAB\xE3\x81\xBB");

    // committed: the text arrives, and nothing is put together any more
    PushText("\xE6\x97\xA5\xE6\x9C\xAC");
    const InputState &committed = Frame();
    EXPECT_EQ(committed.GetText(), "\xE6\x97\xA5\xE6\x9C\xAC");
    EXPECT_EQ(committed.GetComposition().text, "");
  }

  TEST_F(SDL2InputSystemTest, ForgetsWhatWasPutTogetherWhenNoTextIsTypedAnyMore)
  {
    _input->StartTextInput({100, 200, 2, 20});
    PushComposition("ni", 2, 0);
    EXPECT_EQ(Frame().GetComposition().text, "ni");

    _input->StopTextInput();
    EXPECT_EQ(Frame().GetComposition().text, "");
  }

  // a controller

  class SDL2ControllerTest : public SDL2InputSystemTest
  {
  protected:
    int _device = -1;
    SDL_Joystick *_joystick = nullptr;

    void SetUp() override
    {
      SDL2InputSystemTest::SetUp();
      if (IsSkipped()) { return; }

      // a controller that exists in SDL alone
      _device = SDL_JoystickAttachVirtual(
        SDL_JOYSTICK_TYPE_GAMECONTROLLER, SDL_CONTROLLER_AXIS_MAX, SDL_CONTROLLER_BUTTON_MAX, 0);

      if (_device < 0 || SDL_IsGameController(_device) != SDL_TRUE)
      {
        GTEST_SKIP() << "SDL cannot make a controller of its own: " << SDL_GetError();
      }

      // SDL reports the controller, which the input system then opens
      Frame();

      _joystick = SDL_JoystickOpen(_device);
      if (_joystick == nullptr) { GTEST_SKIP() << "SDL cannot open the controller: " << SDL_GetError(); }
    }

    void TearDown() override
    {
      if (_joystick != nullptr) { SDL_JoystickClose(_joystick); }
      if (_device >= 0) { SDL_JoystickDetachVirtual(_device); }
      SDL2InputSystemTest::TearDown();
    }

    void SetButton(const SDL_GameControllerButton button, const bool down) const
    {
      ASSERT_EQ(SDL_JoystickSetVirtualButton(_joystick, button, down ? 1 : 0), 0) << SDL_GetError();
    }

    void SetAxis(const SDL_GameControllerAxis axis, const int value) const
    {
      ASSERT_EQ(SDL_JoystickSetVirtualAxis(_joystick, axis, static_cast<Sint16>(value)), 0) << SDL_GetError();
    }
  };

  TEST_F(SDL2ControllerTest, UsesTheControllerThatWasPluggedIn)
  {
    EXPECT_TRUE(_logger->Contains(neon::testing::LogLevel::Info, "Using the controller"));
  }

  TEST_F(SDL2ControllerTest, AcceptsWithTheLowerButtonAndCancelsWithTheRightOne)
  {
    SetButton(SDL_CONTROLLER_BUTTON_A, true);
    const InputState &accept = Frame();
    EXPECT_TRUE(accept[Action::Ui_Accept]);
    EXPECT_FALSE(accept[Action::Ui_Cancel]);

    SetButton(SDL_CONTROLLER_BUTTON_A, false);
    SetButton(SDL_CONTROLLER_BUTTON_B, true);
    const InputState &cancel = Frame();
    EXPECT_FALSE(cancel[Action::Ui_Accept]);
    EXPECT_TRUE(cancel[Action::Ui_Cancel]);
  }

  TEST_F(SDL2ControllerTest, MovesThroughAUserInterfaceWithThePad)
  {
    SetButton(SDL_CONTROLLER_BUTTON_DPAD_DOWN, true);
    const InputState &state = Frame();
    EXPECT_TRUE(state[Action::Ui_Down]);
    EXPECT_EQ(state.GetLeftStick().y, 0.0);
  }

  TEST_F(SDL2ControllerTest, HandsTheLeftStickOverAsItIsAndMovesTheUserInterfaceWithIt)
  {
    SetAxis(SDL_CONTROLLER_AXIS_LEFTX, 32000);
    const InputState &right = Frame();
    EXPECT_NEAR(right.GetLeftStick().x, 32000.0 / 32767.0, 0.0001);
    EXPECT_TRUE(right[Action::Ui_Right]);

    // not far enough to count as a direction, and still handed over for
    // the input map to shape
    SetAxis(SDL_CONTROLLER_AXIS_LEFTX, 8000);
    const InputState &little = Frame();
    EXPECT_NEAR(little.GetLeftStick().x, 8000.0 / 32767.0, 0.0001);
    EXPECT_FALSE(little[Action::Ui_Right]);

    SetAxis(SDL_CONTROLLER_AXIS_LEFTX, 0);
    SetAxis(SDL_CONTROLLER_AXIS_LEFTY, -32000);
    const InputState &up = Frame();
    EXPECT_NEAR(up.GetLeftStick().y, -32000.0 / 32767.0, 0.0001);
    EXPECT_TRUE(up[Action::Ui_Up]);
  }

  TEST_F(SDL2ControllerTest, ScrollsWithTheRightStick)
  {
    SetAxis(SDL_CONTROLLER_AXIS_RIGHTY, 32767);
    EXPECT_DOUBLE_EQ(Frame().GetRightStick().y, 1.0);

    SetAxis(SDL_CONTROLLER_AXIS_RIGHTY, -16384);
    EXPECT_NEAR(Frame().GetRightStick().y, -0.5, 0.001);

    // handed over as it is, a little off the middle; the dead zone is the
    // input map's
    SetAxis(SDL_CONTROLLER_AXIS_RIGHTY, 3000);
    EXPECT_NEAR(Frame().GetRightStick().y, 3000.0 / 32767.0, 0.0001);
  }

  TEST_F(SDL2ControllerTest, IsHeldByMoreThanTheKeyboard)
  {
    SetButton(SDL_CONTROLLER_BUTTON_A, true);

    InputState state = Frame();
    state.ClearKeyboard();
    EXPECT_TRUE(state[Action::Ui_Accept]);
  }
} // namespace
