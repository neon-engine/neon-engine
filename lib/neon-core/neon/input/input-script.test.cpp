#include "input-script.hpp"

#include <memory>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <neon/input/headless-input-system.hpp>
#include <neon/testing/recording-logger.hpp>

namespace
{
  using neon::Action;
  using neon::Axis;
  using neon::Headless_InputSystem;
  using neon::InputScript;
  using neon::InputState;
  using neon::Key;
  using neon::testing::RecordingLogger;
  using ::testing::ElementsAre;

  class InputScriptTest : public ::testing::Test
  {
  protected:
    std::shared_ptr<RecordingLogger> _logger = std::make_shared<RecordingLogger>();
    InputScript _script;
    std::vector<std::string> _errors;

    void Read(const std::string &text)
    {
      ASSERT_TRUE(InputScript::Parse(text, "script", _script, _errors)) << ::testing::PrintToString(_errors);
    }

    std::vector<std::string> ProblemsOf(const std::string &text)
    {
      std::vector<std::string> errors;
      InputScript script;
      EXPECT_FALSE(InputScript::Parse(text, "script", script, errors));
      EXPECT_TRUE(script.IsEmpty());
      return errors;
    }

    /// The state of a frame, as an input system hands it out.
    InputState Frame(const std::size_t frame)
    {
      InputState state(_logger);
      _script.Apply(frame, state);
      return state;
    }
  };

  TEST_F(InputScriptTest, IsEmptyWithoutLines)
  {
    Read("");
    EXPECT_TRUE(_script.IsEmpty());
    EXPECT_EQ(_script.GetLastFrame(), 0u);

    Read("# nothing but a comment\n\n   \n");
    EXPECT_TRUE(_script.IsEmpty());
  }

  TEST_F(InputScriptTest, PutsThePointerWhereItSaysAndKeepsItThere)
  {
    Read("2: pointer 640 360.5");

    EXPECT_FALSE(Frame(1).HasPointer());

    const InputState second = Frame(2);
    EXPECT_TRUE(second.HasPointer());
    EXPECT_EQ(second.GetPointer().x, 640.0);
    EXPECT_EQ(second.GetPointer().y, 360.5);

    EXPECT_TRUE(Frame(3).HasPointer());
    EXPECT_EQ(Frame(4).GetPointer().x, 640.0);
  }

  TEST_F(InputScriptTest, TakesThePointerAway)
  {
    Read("1: pointer 10 10\n3: pointer none");

    EXPECT_TRUE(Frame(2).HasPointer());
    EXPECT_FALSE(Frame(3).HasPointer());
  }

  TEST_F(InputScriptTest, HoldsTheButtonFromDownToUp)
  {
    Read("1: pointer 5 5\n2: down\n4: up");

    EXPECT_FALSE(Frame(1)[Action::Pointer_Primary]);
    EXPECT_TRUE(Frame(2)[Action::Pointer_Primary]);
    EXPECT_TRUE(Frame(3)[Action::Pointer_Primary]);
    EXPECT_FALSE(Frame(4)[Action::Pointer_Primary]);
  }

  TEST_F(InputScriptTest, ClicksForOneFrame)
  {
    Read("1: pointer 5 5\n3: click");

    EXPECT_FALSE(Frame(2)[Action::Pointer_Primary]);
    EXPECT_TRUE(Frame(3)[Action::Pointer_Primary]);
    EXPECT_FALSE(Frame(4)[Action::Pointer_Primary]);
    EXPECT_EQ(_script.GetLastFrame(), 4u);
  }

  TEST_F(InputScriptTest, DoesNotPressWithoutAPointer)
  {
    Read("1: down");
    EXPECT_FALSE(Frame(1)[Action::Pointer_Primary]);
  }

  TEST_F(InputScriptTest, PressesAKeyWithWhatIsHeldNextToIt)
  {
    Read("1: key left shift word\n2: key c shortcut");

    const InputState first = Frame(1);
    ASSERT_EQ(first.GetKeyEvents().size(), 2u);
    EXPECT_EQ(first.GetKeyEvents()[0].key, Key::Left);
    EXPECT_TRUE(first.GetKeyEvents()[0].is_down);
    EXPECT_TRUE(first.GetKeyEvents()[0].modifiers.shift);
    EXPECT_TRUE(first.GetKeyEvents()[0].modifiers.word);
    EXPECT_FALSE(first.GetKeyEvents()[0].modifiers.shortcut);
    EXPECT_FALSE(first.GetKeyEvents()[1].is_down);

    const InputState second = Frame(2);
    ASSERT_EQ(second.GetKeyEvents().size(), 2u);
    EXPECT_EQ(second.GetKeyEvents()[0].key, Key::C);
    EXPECT_TRUE(second.GetKeyEvents()[0].modifiers.shortcut);

    EXPECT_TRUE(Frame(3).GetKeyEvents().empty());
  }

  TEST_F(InputScriptTest, TypesTheRestOfTheLine)
  {
    Read("1: text Ada Lovelace\n2: text one\\ntwo\n3: text \\s");

    EXPECT_EQ(Frame(1).GetText(), "Ada Lovelace");
    EXPECT_EQ(Frame(2).GetText(), "one\ntwo");
    EXPECT_EQ(Frame(3).GetText(), " ");
    EXPECT_EQ(Frame(4).GetText(), "");
  }

  TEST_F(InputScriptTest, TypesTextOfSeveralBytes)
  {
    Read("1: text Zo\xC3\xAB \xE2\x82\xAC");
    EXPECT_EQ(Frame(1).GetText(), "Zo\xC3\xAB \xE2\x82\xAC");
  }

  TEST_F(InputScriptTest, KeepsWhatIsPutTogetherUntilItEnds)
  {
    Read("1: compose ni\n3: compose");

    EXPECT_EQ(Frame(1).GetComposition().text, "ni");
    EXPECT_EQ(Frame(2).GetComposition().text, "ni");
    EXPECT_EQ(Frame(3).GetComposition().text, "");
  }

  TEST_F(InputScriptTest, TurnsTheWheelInOneFrame)
  {
    Read("2: wheel 0 3\n3: wheel 0.5 -0.25 precise");

    EXPECT_EQ(Frame(1).GetWheel().y, 0.0);

    const InputState second = Frame(2);
    EXPECT_EQ(second.GetWheel().x, 0.0);
    EXPECT_EQ(second.GetWheel().y, 3.0);
    EXPECT_FALSE(second.IsWheelPrecise());

    const InputState third = Frame(3);
    EXPECT_EQ(third.GetWheel().x, 0.5);
    EXPECT_EQ(third.GetWheel().y, -0.25);
    EXPECT_TRUE(third.IsWheelPrecise());
  }

  TEST_F(InputScriptTest, HoldsAnActionForAsManyFramesAsItSays)
  {
    Read("2: hold ui-down\n5: hold ui-accept 3");

    EXPECT_FALSE(Frame(1)[Action::Ui_Down]);
    EXPECT_TRUE(Frame(2)[Action::Ui_Down]);
    EXPECT_FALSE(Frame(3)[Action::Ui_Down]);

    EXPECT_TRUE(Frame(5)[Action::Ui_Accept]);
    EXPECT_TRUE(Frame(7)[Action::Ui_Accept]);
    EXPECT_FALSE(Frame(8)[Action::Ui_Accept]);
    EXPECT_EQ(_script.GetLastFrame(), 7u);
  }

  TEST_F(InputScriptTest, HoldsAnActionOfTheInputMapByItsName)
  {
    Read("2: hold jump 2\n3: hold pause");

    EXPECT_FALSE(Frame(1).IsActionHeld("jump"));
    EXPECT_TRUE(Frame(2).IsActionHeld("jump"));
    EXPECT_TRUE(Frame(3).IsActionHeld("jump"));
    EXPECT_TRUE(Frame(3).IsActionHeld("pause"));
    EXPECT_FALSE(Frame(4).IsActionHeld("jump"));
    EXPECT_FALSE(Frame(4).IsActionHeld("pause"));
  }

  TEST_F(InputScriptTest, HoldsAKeyByWhereItIs)
  {
    Read("2: hold-key w 3\n2: hold-key space");

    EXPECT_FALSE(Frame(1).IsKeyDown(neon::Key::W));
    EXPECT_TRUE(Frame(2).IsKeyDown(neon::Key::W));
    EXPECT_TRUE(Frame(2).IsKeyDown(neon::Key::Space));
    EXPECT_TRUE(Frame(4).IsKeyDown(neon::Key::W));
    EXPECT_FALSE(Frame(4).IsKeyDown(neon::Key::Space));
    EXPECT_FALSE(Frame(5).IsKeyDown(neon::Key::W));
    EXPECT_EQ(_script.GetLastFrame(), 4u);
  }

  TEST_F(InputScriptTest, RefusesAKeyThatIsNotKnownToHold)
  {
    EXPECT_THAT(
      ProblemsOf("1: hold-key f13\n2: hold-key w soon"),
      ElementsAre(
        "script:1: 'hold-key' is followed by 'f13', where the name of a key was expected, such as w or space, and a "
        "number of frames after it or nothing",
        "script:2: 'hold-key' is followed by 'w soon', where the name of a key was expected, such as w or space, "
        "and a number of frames after it or nothing"));
  }

  TEST_F(InputScriptTest, HoldsAButtonOfAControllerByItsName)
  {
    Read("2: hold-button south 3\n2: hold-button left-shoulder");

    EXPECT_FALSE(Frame(1).IsControllerButtonDown(neon::ControllerButton::South));
    EXPECT_TRUE(Frame(2).IsControllerButtonDown(neon::ControllerButton::South));
    EXPECT_TRUE(Frame(2).IsControllerButtonDown(neon::ControllerButton::LeftShoulder));
    EXPECT_TRUE(Frame(4).IsControllerButtonDown(neon::ControllerButton::South));
    EXPECT_FALSE(Frame(4).IsControllerButtonDown(neon::ControllerButton::LeftShoulder));
    EXPECT_FALSE(Frame(5).IsControllerButtonDown(neon::ControllerButton::South));
    EXPECT_EQ(_script.GetLastFrame(), 4u);
  }

  TEST_F(InputScriptTest, RefusesAButtonThatIsNotKnownToHold)
  {
    EXPECT_THAT(
      ProblemsOf("1: hold-button a\n2: hold-button south soon"),
      ElementsAre(
        "script:1: 'hold-button' is followed by 'a', where the name of a button of a controller was expected, such "
        "as south or left-shoulder, and a number of frames after it or nothing",
        "script:2: 'hold-button' is followed by 'south soon', where the name of a button of a controller was "
        "expected, such as south or left-shoulder, and a number of frames after it or nothing"));
  }

  TEST_F(InputScriptTest, HoldsTheLeftButtonOfTheMouseWhileThePointerIsDown)
  {
    Read("1: down\n3: up");

    EXPECT_TRUE(Frame(1).IsMouseButtonDown(neon::MouseButton::Left));
    EXPECT_TRUE(Frame(2).IsMouseButtonDown(neon::MouseButton::Left));
    EXPECT_FALSE(Frame(3).IsMouseButtonDown(neon::MouseButton::Left));
  }

  TEST_F(InputScriptTest, KnowsEveryActionByItsName)
  {
    for (std::size_t action = 0; action < neon::kAction_Size; action++)
    {
      if (static_cast<Action>(action) == Action::Mouse) { continue; }

      const std::string name = neon::NameOf(static_cast<Action>(action));
      ASSERT_FALSE(name.empty()) << action;

      Read("1: hold " + name);
      EXPECT_TRUE(Frame(1)[static_cast<Action>(action)]) << name;
    }
  }

  TEST_F(InputScriptTest, PushesTheRightStick)
  {
    Read("1: stick 0 1 2");

    EXPECT_EQ(Frame(1).GetRightStick().y, 1.0);
    EXPECT_EQ(Frame(2).GetRightStick().y, 1.0);
    EXPECT_EQ(Frame(3).GetRightStick().y, 0.0);
  }

  TEST_F(InputScriptTest, PushesTheLeftStickApartFromTheRightOne)
  {
    Read("1: left-stick 0.5 -1 2\n2: stick 1 0");

    EXPECT_EQ(Frame(1).GetLeftStick().x, 0.5);
    EXPECT_EQ(Frame(1).GetLeftStick().y, -1.0);
    EXPECT_EQ(Frame(1).GetRightStick().x, 0.0);

    EXPECT_EQ(Frame(2).GetLeftStick().y, -1.0);
    EXPECT_EQ(Frame(2).GetRightStick().x, 1.0);

    EXPECT_EQ(Frame(3).GetLeftStick().x, 0.0);
    EXPECT_EQ(Frame(3).GetLeftStick().y, 0.0);
    EXPECT_EQ(Frame(3).GetRightStick().x, 0.0);
    EXPECT_EQ(_script.GetLastFrame(), 2u);
  }

  TEST_F(InputScriptTest, RefusesALeftStickWithoutTwoNumbers)
  {
    EXPECT_EQ(
      ProblemsOf("1: left-stick up"),
      std::vector<std::string>{
        "script:1: 'left-stick' is followed by 'up', where two numbers from -1 to 1 were expected, and a number of "
        "frames after them or nothing"});
  }

  TEST_F(InputScriptTest, SaysWhichFamilyTheGamepadIsOf)
  {
    Read("2: device gamepad playstation5\n4: device gamepad switch\n6: device gamepad\n8: device keyboard");

    EXPECT_EQ(Frame(1).GetGamepadKind(), neon::GamepadKind::Other);
    EXPECT_EQ(Frame(2).GetDevice(), neon::InputDevice::Gamepad);
    EXPECT_EQ(Frame(2).GetGamepadKind(), neon::GamepadKind::PlayStation5);
    EXPECT_EQ(Frame(5).GetGamepadKind(), neon::GamepadKind::Switch);
    EXPECT_EQ(Frame(6).GetGamepadKind(), neon::GamepadKind::Other) << "one that does not say";
    EXPECT_EQ(Frame(8).GetDevice(), neon::InputDevice::KeyboardAndMouse);

    EXPECT_EQ(
      ProblemsOf("1: device gamepad dreamcast"),
      std::vector<std::string>{
        "script:1: 'device' is followed by 'gamepad dreamcast', where keyboard, gamepad, or gamepad with one of "
        "xbox, playstation4, playstation5, switch, other was expected"});
  }

  TEST_F(InputScriptTest, SaysWhichDeviceIsUsedFromAFrameOn)
  {
    Read("2: device gamepad\n4: device keyboard");

    EXPECT_EQ(Frame(1).GetDevice(), neon::InputDevice::KeyboardAndMouse);
    EXPECT_EQ(Frame(2).GetDevice(), neon::InputDevice::Gamepad);
    EXPECT_EQ(Frame(3).GetDevice(), neon::InputDevice::Gamepad);
    EXPECT_EQ(Frame(4).GetDevice(), neon::InputDevice::KeyboardAndMouse);
  }

  TEST_F(InputScriptTest, TurnsTheViewWithTheMouseInOneFrame)
  {
    Read("2: look 12 -3");

    EXPECT_FALSE(Frame(1)[Action::Mouse]);

    const InputState looked = Frame(2);
    EXPECT_TRUE(looked[Action::Mouse]);
    EXPECT_EQ(looked[Axis::Mouse].x, 12.0);
    EXPECT_EQ(looked[Axis::Mouse].y, -3.0);

    EXPECT_FALSE(Frame(3)[Action::Mouse]);
  }

  TEST_F(InputScriptTest, RefusesADeviceItDoesNotKnow)
  {
    EXPECT_EQ(
      ProblemsOf("1: device wheel"),
      std::vector<std::string>{
        "script:1: 'device' is followed by 'wheel', where keyboard, gamepad, or gamepad with one of xbox, "
        "playstation4, playstation5, switch, other was expected"});
  }

  TEST_F(InputScriptTest, TakesALineWithoutAFrameForTheFrameBefore)
  {
    Read("3: pointer 1 2\nclick\ntext a");

    ASSERT_EQ(_script.GetSteps().size(), 3u);
    for (const auto &step : _script.GetSteps()) { EXPECT_EQ(step.frame, 3u); }
  }

  TEST_F(InputScriptTest, SetsLinesApartWithSemicolons)
  {
    Read("1: pointer 1 2; 2: click; 4: key enter");

    ASSERT_EQ(_script.GetSteps().size(), 3u);
    EXPECT_EQ(_script.GetSteps()[2].frame, 4u);
    EXPECT_EQ(_script.GetSteps()[2].key.key, Key::Enter);
  }

  TEST_F(InputScriptTest, SortsByFrameAndKeepsTheOrderWithinOne)
  {
    Read("5: text b\n2: text a\n5: text c");

    ASSERT_EQ(_script.GetSteps().size(), 3u);
    EXPECT_EQ(_script.GetSteps()[0].text, "a");
    EXPECT_EQ(Frame(5).GetText(), "bc");
  }

  TEST_F(InputScriptTest, SaysWhatIsWrongWithEveryLine)
  {
    EXPECT_THAT(
      ProblemsOf(
        "1: pointer here\n"
        "2: jump\n"
        "# fine\n"
        "0: click\n"
        "3: key f13\n"
        "3: key left hyper\n"
        "4: wheel 1\n"
        "5: hold fire soon\n"
        "6: click twice\n"
        "7: stick 1\n"),
      ElementsAre(
        "script:1: 'pointer' is followed by 'here', where two numbers or none was expected",
        "script:2: 'jump' is not known. Known are: pointer, down, up, click, key, text, compose, wheel, hold, "
        "hold-key, hold-button, stick, left-stick, device, look",
        "script:4: the frame is '0', where a whole number above 0 was expected",
        "script:5: 'key' is followed by 'f13', where the name of a key was expected, such as left, enter, or a",
        "script:6: 'hyper' is held with the key, where shift, control, alt, super, shortcut, or word was "
        "expected",
        "script:7: 'wheel' is followed by '1', where two numbers were expected, and precise after them or "
        "nothing",
        "script:8: 'hold' is followed by 'fire soon', where an action such as ui-accept or jump was expected, and "
        "a number of frames after it or nothing",
        "script:9: 'click' is followed by 'twice', where nothing was expected",
        "script:10: 'stick' is followed by '1', where two numbers from -1 to 1 were expected, and a number of "
        "frames after them or nothing"));
  }

  TEST_F(InputScriptTest, MovesAndJumpsThroughTheInputMapWithoutDevices)
  {
    // the default map: `move` on the left stick, `jump` on south. The stick
    // is pushed past the dead zone of the map
    Read("1: left-stick 0 -1 2\n2: hold-button south");

    Headless_InputSystem input(SettingsConfig{}, _logger);
    std::vector<std::string> errors;
    ASSERT_TRUE(input.SetScript(_script, errors)) << ::testing::PrintToString(errors);

    input.ProcessInput();
    EXPECT_EQ(input.ActionAxis2("move"), glm::vec2(0.0f, 1.0f));
    EXPECT_FALSE(input.IsActionDown("jump"));

    input.ProcessInput();
    EXPECT_EQ(input.ActionAxis2("move"), glm::vec2(0.0f, 1.0f));
    EXPECT_TRUE(input.WasActionPressed("jump"));

    input.ProcessInput();
    EXPECT_EQ(input.ActionAxis2("move"), glm::vec2(0.0f, 0.0f));
    EXPECT_FALSE(input.IsActionDown("jump"));
  }

  TEST_F(InputScriptTest, IsFollowedByTheInputSystemWithoutDevices)
  {
    Read("1: pointer 100 200\n2: click\n3: text hi");

    Headless_InputSystem input(SettingsConfig{}, _logger);
    std::vector<std::string> errors;
    ASSERT_TRUE(input.SetScript(_script, errors)) << ::testing::PrintToString(errors);

    input.ProcessInput();
    EXPECT_TRUE(input.GetInputState().HasPointer());
    EXPECT_EQ(input.GetInputState().GetPointer().x, 100.0);
    EXPECT_FALSE(input.GetInputState()[Action::Pointer_Primary]);

    input.ProcessInput();
    EXPECT_TRUE(input.GetInputState()[Action::Pointer_Primary]);

    input.ProcessInput();
    EXPECT_FALSE(input.GetInputState()[Action::Pointer_Primary]);
    EXPECT_EQ(input.GetInputState().GetText(), "hi");

    input.ProcessInput();
    EXPECT_EQ(input.GetInputState().GetText(), "");
  }

  TEST_F(InputScriptTest, RemembersWhereATextIsTypedWithoutDevices)
  {
    Headless_InputSystem input(SettingsConfig{}, _logger);
    EXPECT_FALSE(input.IsTextInputActive());

    input.StartTextInput({10, 20, 2, 18});
    EXPECT_TRUE(input.IsTextInputActive());
    EXPECT_EQ(input.GetTextInputArea().x, 10);
    EXPECT_EQ(input.GetTextInputArea().height, 18);

    input.StopTextInput();
    EXPECT_FALSE(input.IsTextInputActive());
  }
} // namespace
