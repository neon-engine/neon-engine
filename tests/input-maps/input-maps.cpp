// The file of an input map, as an application reads it: InputMapFile with the
// document format for YAML. Files are kept in memory, except for the map of
// the runtime, which is read as it is in the repository.

#include <filesystem>
#include <fstream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <neon/data/ryml-document-format.hpp>
#include <neon/input/input-map-file.hpp>
#include <neon/testing/memory-file-system.hpp>
#include <neon/testing/recording-logger.hpp>

namespace
{
  using neon::Chord;
  using neon::ControllerButton;
  using neon::InputActionType;
  using neon::InputMap;
  using neon::InputMapFile;
  using neon::Key;
  using neon::MouseButton;
  using neon::RYML_DocumentFormat;
  using neon::Sensor;
  using neon::Stick;
  using neon::ControllerTrigger;
  using neon::testing::MemoryFileSystem;
  using neon::testing::RecordingLogger;
  using ::testing::ElementsAre;
  using ::testing::HasSubstr;
  using ::testing::IsEmpty;

  const std::string path = "assets://input/game.input.yml";

  const std::string complete =
    "version: 1\n"
    "actions:\n"
    "  move:  { type: axis2, keys: [w, s, a, d], stick: left }\n"
    "  look:  { type: axis2, mouse: motion, stick: right, rate: 600 }\n"
    "  throttle: { type: axis, trigger: right, keys: [w, s] }\n"
    "  jump:  { type: button, keys: [space], buttons: [south] }\n"
    "  shoot: { type: button, mouse: left, buttons: [right-trigger] }\n"
    "  pause: { type: button, keys: [escape], buttons: [start] }\n"
    "states:\n"
    "  walking: [move, look, jump, shoot, pause, throttle]\n"
    "  menu:    [pause]\n";

  class InputMapsTest : public ::testing::Test
  {
  protected:
    std::shared_ptr<RecordingLogger> _logger = std::make_shared<RecordingLogger>();
    MemoryFileSystem _files{SettingsConfig{}, _logger};
    RYML_DocumentFormat _yaml;
    InputMapFile _file{&_files, &_yaml, _logger};
    InputMap _map;
    std::vector<std::string> _errors;

    void SetUp() override
    {
      _files.Initialize();
    }

    /// Puts the text where the map is read from.
    void Write(const std::string &text)
    {
      _files.AddNativeFile("/assets/input/game.input.yml", text);
    }

    bool Read()
    {
      return _file.Read(path, _map, _errors);
    }

    /// Writes a map with one action and one state around what is handed in,
    /// so that a test says only what it is about.
    static std::string With(const std::string &action, const std::string &states = "  s: [a]\n")
    {
      return "version: 1\nactions:\n  a: " + action + "\nstates:\n" + states;
    }

    /// Reads, and expects it to fail with one error that holds the text.
    void ExpectRefused(const std::string &text)
    {
      EXPECT_FALSE(Read());
      ASSERT_EQ(_errors.size(), 1u) << ::testing::PrintToString(_errors);
      EXPECT_THAT(_errors.front(), HasSubstr(text));
    }

    static std::string FileOfTheRuntime(const std::string &name)
    {
      const std::filesystem::path file_path = std::filesystem::path(NEON_RUNTIME_ASSETS) / name;
      const std::ifstream file(file_path);
      EXPECT_TRUE(file.good()) << file_path;

      std::stringstream text;
      text << file.rdbuf();
      return text.str();
    }
  };

  // reading

  TEST_F(InputMapsTest, ReadsEveryActionWithItsBindings)
  {
    Write(complete);

    ASSERT_TRUE(Read()) << ::testing::PrintToString(_errors);
    EXPECT_THAT(_errors, IsEmpty());
    ASSERT_EQ(_map.GetActions().size(), 6u);

    const auto *move = _map.FindAction("move");
    ASSERT_NE(move, nullptr);
    EXPECT_EQ(move->type, InputActionType::Axis2);
    EXPECT_EQ(move->keys, (std::vector<Chord<Key>>{Key::W, Key::S, Key::A, Key::D}));
    EXPECT_EQ(move->stick, Stick::Left);
    EXPECT_FALSE(move->mouse_motion);

    const auto *look = _map.FindAction("look");
    ASSERT_NE(look, nullptr);
    EXPECT_TRUE(look->mouse_motion);
    EXPECT_EQ(look->stick, Stick::Right);
    EXPECT_EQ(look->rate, 600.0f);
    EXPECT_FALSE(move->rate.has_value());

    const auto *throttle = _map.FindAction("throttle");
    ASSERT_NE(throttle, nullptr);
    EXPECT_EQ(throttle->type, InputActionType::Axis);
    EXPECT_EQ(throttle->trigger, ControllerTrigger::Right);
    EXPECT_EQ(throttle->keys, (std::vector<Chord<Key>>{Key::W, Key::S}));

    const auto *jump = _map.FindAction("jump");
    ASSERT_NE(jump, nullptr);
    EXPECT_EQ(jump->type, InputActionType::Button);
    EXPECT_EQ(jump->keys, (std::vector<Chord<Key>>{Key::Space}));
    EXPECT_EQ(jump->buttons, (std::vector<Chord<ControllerButton>>{ControllerButton::South}));

    const auto *shoot = _map.FindAction("shoot");
    ASSERT_NE(shoot, nullptr);
    EXPECT_EQ(shoot->mouse_button, MouseButton::Left);
    EXPECT_EQ(shoot->buttons, (std::vector<Chord<ControllerButton>>{ControllerButton::RightTrigger}));
  }

  TEST_F(InputMapsTest, ReadsTheStatesInTheOrderWrittenWithTheFirstToStartIn)
  {
    Write(complete);

    ASSERT_TRUE(Read()) << ::testing::PrintToString(_errors);
    ASSERT_EQ(_map.GetStates().size(), 2u);
    EXPECT_EQ(_map.GetFirstState(), "walking");
    EXPECT_THAT(_map.FindState("walking")->actions, ElementsAre("move", "look", "jump", "shoot", "pause", "throttle"));
    EXPECT_THAT(_map.FindState("menu")->actions, ElementsAre("pause"));
  }

  TEST_F(InputMapsTest, ReadsAnActionWithoutBindingsForAPlayerToBindLater)
  {
    Write(With("{ type: button }"));

    ASSERT_TRUE(Read()) << ::testing::PrintToString(_errors);
    EXPECT_TRUE(_map.FindAction("a")->keys.empty());
  }

  TEST_F(InputMapsTest, ReadsAnAxisFromFourButtonsOfAController)
  {
    Write(With("{ type: axis2, buttons: [dpad-up, dpad-down, dpad-left, dpad-right] }"));

    ASSERT_TRUE(Read()) << ::testing::PrintToString(_errors);
    EXPECT_EQ(_map.FindAction("a")->buttons.size(), 4u);
  }

  TEST_F(InputMapsTest, KnowsEveryKeyButtonAndStickByItsName)
  {
    Write(With(
      "{ type: button, keys: [left, right, up, down, home, end, page-up, page-down, backspace, delete, enter, tab, "
      "escape, space, a, z, 0, 9, f1, f12, left-shift, right-shift, left-control, right-control, left-alt, right-alt], "
      "buttons: [south, east, west, north, left-shoulder, right-shoulder, left-trigger, right-trigger, left-stick, "
      "right-stick, start, back, guide, dpad-up, dpad-down, dpad-left, dpad-right], mouse: middle }"));

    ASSERT_TRUE(Read()) << ::testing::PrintToString(_errors);
    EXPECT_EQ(_map.FindAction("a")->keys.size(), 26u);
    EXPECT_EQ(_map.FindAction("a")->buttons.size(), 17u);
    EXPECT_EQ(_map.FindAction("a")->mouse_button, MouseButton::Middle);
  }

  // chords

  TEST_F(InputMapsTest, ReadsAListOfNamesInABindingAsAChordHeldTogether)
  {
    Write(With("{ type: button, keys: [space, [left-shift, w]], buttons: [[left-shoulder, south]] }"));

    ASSERT_TRUE(Read()) << ::testing::PrintToString(_errors);
    EXPECT_EQ(_map.FindAction("a")->keys, (std::vector<Chord<Key>>{Key::Space, {Key::LeftShift, Key::W}}));
    EXPECT_EQ(_map.FindAction("a")->buttons,
              (std::vector<Chord<ControllerButton>>{{ControllerButton::LeftShoulder, ControllerButton::South}}));
  }

  TEST_F(InputMapsTest, ReadsAChordInEveryPlaceOfAnAxis)
  {
    Write(With("{ type: axis2, keys: [[left-shift, w], s, [left-shift, a], d] }"));

    ASSERT_TRUE(Read()) << ::testing::PrintToString(_errors);
    EXPECT_EQ(_map.FindAction("a")->keys,
              (std::vector<Chord<Key>>{{Key::LeftShift, Key::W}, Key::S, {Key::LeftShift, Key::A}, Key::D}));

    Write(With("{ type: axis, keys: [[left-control, 1], [left-control, 2]] }"));

    ASSERT_TRUE(Read()) << ::testing::PrintToString(_errors);
    EXPECT_EQ(_map.FindAction("a")->keys,
              (std::vector<Chord<Key>>{{Key::LeftControl, Key::Digit1}, {Key::LeftControl, Key::Digit2}}));
  }

  TEST_F(InputMapsTest, RefusesAnEmptyChord)
  {
    Write(With("{ type: button, keys: [space, []] }"));

    ExpectRefused(":3: 'keys' holds an empty chord, where names held together were expected, such as [left-shift, w]");
  }

  TEST_F(InputMapsTest, RefusesAChordWithAKeyItDoesNotKnowOrAListInIt)
  {
    Write(With("{ type: button, keys: [[left-shift, fire], [[w]]] }"));

    EXPECT_FALSE(Read());
    EXPECT_THAT(_errors, ElementsAre(
                  HasSubstr(":3: 'keys' names 'fire', which is not known. Known are: left, right, up"),
                  HasSubstr(":3: 'keys' holds a list inside a chord, where a name was expected")));
  }

  TEST_F(InputMapsTest, CountsAChordAsOneOfTheKeysOfAnAxis)
  {
    Write(With("{ type: axis, keys: [[left-shift, w], s, a] }"));

    ExpectRefused(":3: 'keys' holds 3 for an axis, where two were expected: positive, negative");
  }

  TEST_F(InputMapsTest, LeavesTheMapAloneWhenTheFileIsWrong)
  {
    _map = InputMap::Default();
    Write(With("{ type: lever }"));

    EXPECT_FALSE(Read());
    EXPECT_NE(_map.FindAction("move"), nullptr);
  }

  // what is refused

  TEST_F(InputMapsTest, RefusesAMissingFile)
  {
    ExpectRefused("assets://input/game.input.yml: the input map cannot be read");
  }

  TEST_F(InputMapsTest, RefusesTextThatIsNoDocument)
  {
    Write("actions: [\n");

    EXPECT_FALSE(Read());
    EXPECT_EQ(_errors.size(), 1u) << ::testing::PrintToString(_errors);
  }

  TEST_F(InputMapsTest, RefusesAMissingVersion)
  {
    Write("actions:\n  a: { type: button }\nstates:\n  s: [a]\n");

    ExpectRefused("'version' is missing. It holds the version of the layout, which is 1");
  }

  TEST_F(InputMapsTest, RefusesAVersionAboveItsOwn)
  {
    Write("version: 2\nactions:\n  a: { type: button }\nstates:\n  s: [a]\n");

    ExpectRefused("assets://input/game.input.yml:1: the input map has version 2, and this engine reads up to version 1");
  }

  TEST_F(InputMapsTest, RefusesMissingActions)
  {
    Write("version: 1\nstates:\n  s: []\n");

    ExpectRefused("'actions' is missing. It holds the actions by their names, with what is bound to each");
  }

  TEST_F(InputMapsTest, RefusesEmptyActions)
  {
    Write("version: 1\nactions: {}\nstates:\n  s: []\n");

    ExpectRefused("'actions' is empty, a map has at least one action");
  }

  TEST_F(InputMapsTest, RefusesAnActionThatIsNoMap)
  {
    Write(With("[w, a]"));

    ExpectRefused(":3: the action 'a' is a list, where a map was expected, such as { type: button, keys: [space] }");
  }

  TEST_F(InputMapsTest, RefusesAnActionWithoutAType)
  {
    Write(With("{ keys: [space] }"));

    ExpectRefused(":3: 'type' is missing for the action 'a'. It is button, axis, axis2, or axis3");
  }

  TEST_F(InputMapsTest, RefusesATypeItDoesNotKnow)
  {
    Write(With("{ type: lever }"));

    ExpectRefused(":3: 'type' of the action 'a' is 'lever', where one of these was expected: button, axis, axis2, axis3");
  }

  TEST_F(InputMapsTest, RefusesAKeyItDoesNotKnow)
  {
    Write(With("{ type: button, keys: [space, fire] }"));

    EXPECT_FALSE(Read());
    ASSERT_EQ(_errors.size(), 1u) << ::testing::PrintToString(_errors);
    EXPECT_THAT(_errors.front(), HasSubstr(":3: 'keys' names 'fire', which is not known. Known are: left, right, up"));
    EXPECT_THAT(_errors.front(), HasSubstr(", w, x, y, z, 0, 1, "));
    EXPECT_THAT(_errors.front(), HasSubstr(", f12, left-shift, right-shift, left-control, right-control, left-alt, right-alt"));
  }

  TEST_F(InputMapsTest, RefusesAButtonOfAControllerItDoesNotKnow)
  {
    Write(With("{ type: button, buttons: [x] }"));

    ExpectRefused(":3: 'buttons' names 'x', which is not known. Known are: south, east, west, north, left-shoulder, "
      "right-shoulder, left-trigger, right-trigger, left-stick, right-stick, start, back, guide, dpad-up, dpad-down, "
      "dpad-left, dpad-right");
  }

  TEST_F(InputMapsTest, RefusesKeysThatAreNoList)
  {
    Write(With("{ type: button, keys: space }"));

    ExpectRefused(":3: 'keys' is text, where a list of names was expected");
  }

  TEST_F(InputMapsTest, RefusesAKeyThatIsNoName)
  {
    Write(With("{ type: button, keys: [3.5] }"));

    ExpectRefused(":3: 'keys' holds a number, where a name was expected");
  }

  TEST_F(InputMapsTest, RefusesAButtonOfTheMouseItDoesNotKnow)
  {
    Write(With("{ type: button, mouse: fourth }"));

    ExpectRefused(":3: 'mouse' is 'fourth', where a button was expected. Known are: left, right, middle");
  }

  TEST_F(InputMapsTest, RefusesTheMotionOfTheMouseForAButton)
  {
    Write(With("{ type: button, mouse: motion }"));

    ExpectRefused(":3: 'mouse' is 'motion', where a button was expected. Known are: left, right, middle");
  }

  TEST_F(InputMapsTest, RefusesAButtonOfTheMouseForAnAxis)
  {
    Write(With("{ type: axis2, mouse: left }"));

    ExpectRefused(":3: 'mouse' is 'left', where motion was expected for an axis2");
  }

  TEST_F(InputMapsTest, RefusesAStickForAButton)
  {
    Write(With("{ type: button, stick: left }"));

    ExpectRefused(":3: 'stick' is for an axis2, and this action is a button");
  }

  TEST_F(InputMapsTest, RefusesAStickItDoesNotKnow)
  {
    Write(With("{ type: axis2, stick: middle }"));

    ExpectRefused(":3: 'stick' is 'middle', where left or right was expected");
  }

  TEST_F(InputMapsTest, RefusesAnAxisWithOtherThanFourKeys)
  {
    Write(With("{ type: axis2, keys: [w, s] }"));

    ExpectRefused(":3: 'keys' holds 2 for an axis2, where four were expected: up, down, left, right");
  }

  TEST_F(InputMapsTest, RefusesAnAxisWithOtherThanFourButtons)
  {
    Write(With("{ type: axis2, buttons: [dpad-up] }"));

    ExpectRefused(":3: 'buttons' holds 1 for an axis2, where four were expected: up, down, left, right");
  }

  // an axis of one

  TEST_F(InputMapsTest, ReadsAnAxisFromTwoButtonsOfAControllerAndATriggerAtARate)
  {
    Write(With("{ type: axis, buttons: [right-shoulder, left-shoulder], trigger: left, rate: 2.5 }"));

    ASSERT_TRUE(Read()) << ::testing::PrintToString(_errors);
    EXPECT_EQ(_map.FindAction("a")->buttons,
              (std::vector<Chord<ControllerButton>>{ControllerButton::RightShoulder, ControllerButton::LeftShoulder}));
    EXPECT_EQ(_map.FindAction("a")->trigger, ControllerTrigger::Left);
    EXPECT_EQ(_map.FindAction("a")->rate, 2.5f);
  }

  TEST_F(InputMapsTest, RefusesAnAxisWithOtherThanTwoKeys)
  {
    Write(With("{ type: axis, keys: [w, s, a] }"));

    ExpectRefused(":3: 'keys' holds 3 for an axis, where two were expected: positive, negative");
  }

  TEST_F(InputMapsTest, RefusesAnAxisWithOtherThanTwoButtons)
  {
    Write(With("{ type: axis, buttons: [south] }"));

    ExpectRefused(":3: 'buttons' holds 1 for an axis, where two were expected: positive, negative");
  }

  TEST_F(InputMapsTest, RefusesATriggerForAButton)
  {
    Write(With("{ type: button, trigger: right }"));

    ExpectRefused(":3: 'trigger' is for an axis, and this action is a button");
  }

  TEST_F(InputMapsTest, RefusesATriggerForAnAxisOfTwo)
  {
    Write(With("{ type: axis2, trigger: right }"));

    ExpectRefused(":3: 'trigger' is for an axis, and this action is an axis2");
  }

  TEST_F(InputMapsTest, RefusesATriggerItDoesNotKnow)
  {
    Write(With("{ type: axis, trigger: middle }"));

    ExpectRefused(":3: 'trigger' is 'middle', where left or right was expected");
  }

  TEST_F(InputMapsTest, RefusesTheMouseForAnAxis)
  {
    Write(With("{ type: axis, mouse: motion }"));

    ExpectRefused(":3: 'mouse' is for a button or an axis2, and this action is an axis");
  }

  TEST_F(InputMapsTest, RefusesAStickForAnAxis)
  {
    Write(With("{ type: axis, stick: left }"));

    ExpectRefused(":3: 'stick' is for an axis2, and this action is an axis");
  }

  TEST_F(InputMapsTest, RefusesARateForAButton)
  {
    Write(With("{ type: button, rate: 600 }"));

    ExpectRefused(":3: 'rate' is for an axis, an axis2, or an axis3, and this action is a button");
  }

  // a dead zone and a curve

  TEST_F(InputMapsTest, ReadsADeadZoneAndACurveAndHasAQuarterAndAStraightLineWithout)
  {
    Write(With("{ type: axis2, stick: left, dead_zone: 0.1, curve: 2 }"));

    ASSERT_TRUE(Read()) << ::testing::PrintToString(_errors);
    EXPECT_FLOAT_EQ(_map.FindAction("a")->dead_zone, 0.1f);
    EXPECT_FLOAT_EQ(_map.FindAction("a")->curve, 2.0f);

    Write(With("{ type: axis, trigger: right, dead_zone: 0, curve: linear }"));

    ASSERT_TRUE(Read()) << ::testing::PrintToString(_errors);
    EXPECT_FLOAT_EQ(_map.FindAction("a")->dead_zone, 0.0f);
    EXPECT_FLOAT_EQ(_map.FindAction("a")->curve, 1.0f);

    Write(With("{ type: axis2, stick: left }"));

    ASSERT_TRUE(Read()) << ::testing::PrintToString(_errors);
    EXPECT_FLOAT_EQ(_map.FindAction("a")->dead_zone, 0.25f);
    EXPECT_FLOAT_EQ(_map.FindAction("a")->curve, 1.0f);
  }

  TEST_F(InputMapsTest, RefusesADeadZoneOutsideZeroToBelowOne)
  {
    Write(With("{ type: axis2, stick: left, dead_zone: 1 }"));
    ExpectRefused(":3: 'dead_zone' is 1, where a number from 0 to below 1 was expected");

    _errors.clear();
    Write(With("{ type: axis2, stick: left, dead_zone: -0.5 }"));
    ExpectRefused(":3: 'dead_zone' is -0.5, where a number from 0 to below 1 was expected");
  }

  TEST_F(InputMapsTest, RefusesACurveThatIsNotLinearOrAPowerAboveZero)
  {
    Write(With("{ type: axis2, stick: left, curve: 0 }"));
    ExpectRefused(":3: 'curve' is not linear or a power above 0, such as 2 for a stick that is gentle near the middle");

    _errors.clear();
    Write(With("{ type: axis2, stick: left, curve: steep }"));
    ExpectRefused(":3: 'curve' is not linear or a power above 0, such as 2 for a stick that is gentle near the middle");
  }

  TEST_F(InputMapsTest, RefusesADeadZoneAndACurveForAButtonOrAnAxisOfThree)
  {
    Write(With("{ type: button, keys: [space], dead_zone: 0.1, curve: 2 }"));

    EXPECT_FALSE(Read());
    EXPECT_THAT(_errors, ElementsAre(
                  HasSubstr(":3: 'dead_zone' is for an axis or an axis2, which have a stick or a trigger, and this action is a button"),
                  HasSubstr(":3: 'curve' is for an axis or an axis2, which have a stick or a trigger, and this action is a button")));

    _errors.clear();
    Write(With("{ type: axis3, sensor: gyro, dead_zone: 0.1 }"));
    ExpectRefused(":3: 'dead_zone' is for an axis or an axis2, which have a stick or a trigger, and this action is an axis3");
  }

  // an axis of three

  TEST_F(InputMapsTest, ReadsAnAxisOfThreeFromASensorThatIsOffUnlessItSaysSo)
  {
    Write(With("{ type: axis3, sensor: gyro, rate: 2 }"));

    ASSERT_TRUE(Read()) << ::testing::PrintToString(_errors);
    EXPECT_EQ(_map.FindAction("a")->type, InputActionType::Axis3);
    EXPECT_EQ(_map.FindAction("a")->sensor, Sensor::Gyro);
    EXPECT_FALSE(_map.FindAction("a")->enabled);
    EXPECT_EQ(_map.FindAction("a")->rate, 2.0f);

    Write(With("{ type: axis3, sensor: accelerometer, enabled: true }"));

    ASSERT_TRUE(Read()) << ::testing::PrintToString(_errors);
    EXPECT_EQ(_map.FindAction("a")->sensor, Sensor::Accelerometer);
    EXPECT_TRUE(_map.FindAction("a")->enabled);
  }

  TEST_F(InputMapsTest, RefusesAnAxisOfThreeWithoutASensor)
  {
    Write(With("{ type: axis3 }"));

    ExpectRefused(":3: 'sensor' is missing for the axis3 'a'. It is gyro or accelerometer");
  }

  TEST_F(InputMapsTest, RefusesASensorItDoesNotKnow)
  {
    Write(With("{ type: axis3, sensor: compass }"));

    ExpectRefused(":3: 'sensor' is 'compass', where gyro or accelerometer was expected");
  }

  TEST_F(InputMapsTest, RefusesASensorForAnythingButAnAxisOfThree)
  {
    Write(With("{ type: axis2, sensor: gyro }"));
    ExpectRefused(":3: 'sensor' is for an axis3, and this action is an axis2");

    _errors.clear();
    Write(With("{ type: button, sensor: gyro }"));
    ExpectRefused(":3: 'sensor' is for an axis3, and this action is a button");
  }

  TEST_F(InputMapsTest, RefusesEnabledWithoutASensor)
  {
    Write(With("{ type: button, keys: [space], enabled: true }"));

    ExpectRefused(":3: 'enabled' turns a sensor on from the start, and this action is a button");
  }

  TEST_F(InputMapsTest, RefusesKeysButtonsMouseStickAndTriggerForAnAxisOfThree)
  {
    Write(With("{ type: axis3, sensor: gyro, keys: [w, s], buttons: [south, east], mouse: motion, stick: left, trigger: left }"));

    EXPECT_FALSE(Read());
    EXPECT_THAT(_errors, ElementsAre(
                  HasSubstr(":3: 'mouse' is for a button or an axis2, and this action is an axis3"),
                  HasSubstr(":3: 'stick' is for an axis2, and this action is an axis3"),
                  HasSubstr(":3: 'trigger' is for an axis, and this action is an axis3"),
                  HasSubstr(":3: 'keys' is for a button, an axis, or an axis2, and this action is an axis3"),
                  HasSubstr(":3: 'buttons' is for a button, an axis, or an axis2, and this action is an axis3")));
  }

  TEST_F(InputMapsTest, RefusesARateThatIsNotAboveZero)
  {
    Write(With("{ type: axis2, stick: right, rate: 0 }"));

    ExpectRefused(":3: 'rate' is 0, where a number above 0 was expected");
  }

  TEST_F(InputMapsTest, RefusesANameOfAnActionItDoesNotKnow)
  {
    Write(With("{ type: button, key: space }"));

    ExpectRefused(":3: 'key' is not known to the action 'a'. Known are: type, keys, buttons, mouse, stick, trigger, sensor, enabled, rate, dead_zone, curve");
  }

  TEST_F(InputMapsTest, RefusesMissingStates)
  {
    Write("version: 1\nactions:\n  a: { type: button }\n");

    ExpectRefused("'states' is missing. It holds the states by their names, each a list of the actions that are "
      "live in it");
  }

  TEST_F(InputMapsTest, RefusesEmptyStates)
  {
    Write("version: 1\nactions:\n  a: { type: button }\nstates: {}\n");

    ExpectRefused("'states' is empty, a map has at least one state");
  }

  TEST_F(InputMapsTest, RefusesAStateThatIsNoList)
  {
    Write(With("{ type: button }", "  s: a\n"));

    ExpectRefused(":5: the state 's' is text, where a list of actions was expected");
  }

  TEST_F(InputMapsTest, RefusesAStateThatNamesAnActionThereIsNot)
  {
    Write(With("{ type: button }", "  s: [a, fly]\n"));

    ExpectRefused(":5: the state 's' names the action 'fly', which there is not");
  }

  TEST_F(InputMapsTest, ReadsAStateWithoutActionsInWhichNothingFires)
  {
    Write(With("{ type: button }", "  s: [a]\n  cutscene: []\n"));

    ASSERT_TRUE(Read()) << ::testing::PrintToString(_errors);
    EXPECT_TRUE(_map.FindState("cutscene")->actions.empty());
  }

  TEST_F(InputMapsTest, RefusesANameItDoesNotKnow)
  {
    Write("version: 1\nactions:\n  a: { type: button }\nstates:\n  s: [a]\nchords: []\n");

    ExpectRefused(":6: 'chords' is not known to the input map. Known are: version, actions, states");
  }

  TEST_F(InputMapsTest, ReportsEveryProblemNotOnlyTheFirst)
  {
    Write(
      "version: 1\n"
      "actions:\n"
      "  move: { type: axis2, keys: [w, s], stick: top }\n"
      "  jump: { type: button, keys: [spacebar] }\n"
      "states:\n"
      "  walking: [move, jump, fly]\n");

    EXPECT_FALSE(Read());
    EXPECT_THAT(_errors, ElementsAre(
                  HasSubstr(":3: 'stick' is 'top', where left or right was expected"),
                  HasSubstr(":3: 'keys' holds 2 for an axis2, where four were expected: up, down, left, right"),
                  HasSubstr(":4: 'keys' names 'spacebar', which is not known"),
                  HasSubstr(":6: the state 'walking' names the action 'fly', which there is not")));
  }

  // the map of the runtime

  TEST_F(InputMapsTest, ReadsTheInputMapOfTheRuntime)
  {
    Write(FileOfTheRuntime("input/default.input.yml"));

    ASSERT_TRUE(Read()) << ::testing::PrintToString(_errors);
    EXPECT_THAT(_errors, IsEmpty());
    EXPECT_EQ(_map.GetFirstState(), "playing");
  }

  TEST_F(InputMapsTest, TheInputMapOfTheRuntimeIsTheDefaultOfTheEngine)
  {
    Write(FileOfTheRuntime("input/default.input.yml"));
    ASSERT_TRUE(Read()) << ::testing::PrintToString(_errors);

    const InputMap built_in = InputMap::Default();
    ASSERT_EQ(_map.GetActions().size(), built_in.GetActions().size());
    for (const auto &action : built_in.GetActions())
    {
      const auto *read = _map.FindAction(action.name);
      ASSERT_NE(read, nullptr) << action.name;
      EXPECT_EQ(read->type, action.type) << action.name;
      EXPECT_EQ(read->keys, action.keys) << action.name;
      EXPECT_EQ(read->buttons, action.buttons) << action.name;
      EXPECT_EQ(read->mouse_button, action.mouse_button) << action.name;
      EXPECT_EQ(read->mouse_motion, action.mouse_motion) << action.name;
      EXPECT_EQ(read->stick, action.stick) << action.name;
      EXPECT_EQ(read->trigger, action.trigger) << action.name;
      EXPECT_EQ(read->sensor, action.sensor) << action.name;
      EXPECT_EQ(read->enabled, action.enabled) << action.name;
      EXPECT_EQ(read->rate, action.rate) << action.name;
      EXPECT_EQ(read->dead_zone, action.dead_zone) << action.name;
      EXPECT_EQ(read->curve, action.curve) << action.name;
    }

    ASSERT_EQ(_map.GetStates().size(), built_in.GetStates().size());
    for (const auto &state : built_in.GetStates())
    {
      const auto *read = _map.FindState(state.name);
      ASSERT_NE(read, nullptr) << state.name;
      EXPECT_EQ(read->actions, state.actions) << state.name;
    }
  }
} // namespace
