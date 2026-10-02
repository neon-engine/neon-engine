#include "input-actions.hpp"

#include <memory>

#include <gtest/gtest.h>

#include <neon/testing/recording-logger.hpp>

namespace
{
  using neon::Action;
  using neon::Axis;
  using neon::ControllerButton;
  using neon::InputAction;
  using neon::InputActions;
  using neon::InputActionType;
  using neon::InputMap;
  using neon::InputMapState;
  using neon::InputState;
  using neon::Key;
  using neon::MouseButton;
  using neon::Sensor;
  using neon::Stick;
  using neon::ControllerTrigger;
  using neon::testing::RecordingLogger;

  /// A game that walks and drives: jumping is for walking, honking for
  /// driving, and pausing for both.
  InputMap WalkingAndDriving()
  {
    InputMap map;
    // the sticks and the triggers of these are taken as they are; `walk`,
    // `swim`, and `gas` have a dead zone and a curve
    map.Add({
      .name = "move",
      .type = InputActionType::Axis2,
      .keys = {Key::W, Key::S, Key::A, Key::D},
      .stick = Stick::Left,
      .dead_zone = 0.0f
    });
    map.Add({.name = "look", .type = InputActionType::Axis2, .mouse_motion = true, .stick = Stick::Right, .dead_zone = 0.0f});
    map.Add({.name = "jump", .keys = {Key::Space}, .buttons = {ControllerButton::South}});
    map.Add({.name = "honk", .keys = {Key::H}, .mouse_button = MouseButton::Right});
    map.Add({.name = "pause", .keys = {Key::Escape}, .buttons = {ControllerButton::Start}});
    map.Add({
      .name = "sprint-jump",
      .keys = {{Key::LeftShift, Key::Space}},
      .buttons = {{ControllerButton::LeftShoulder, ControllerButton::South}}
    });
    map.Add({
      .name = "lean",
      .type = InputActionType::Axis,
      .keys = {{Key::LeftShift, Key::D}, {Key::LeftShift, Key::A}}
    });
    map.Add({
      .name = "crawl",
      .type = InputActionType::Axis2,
      .keys = {{Key::LeftControl, Key::W}, {Key::LeftControl, Key::S}, {Key::LeftControl, Key::A}, {Key::LeftControl, Key::D}}
    });
    map.Add({.name = "turn", .type = InputActionType::Axis2, .stick = Stick::Right, .rate = 600.0f, .dead_zone = 0.0f});
    map.Add({
      .name = "throttle",
      .type = InputActionType::Axis,
      .keys = {Key::W, Key::S},
      .trigger = ControllerTrigger::Right,
      .dead_zone = 0.0f
    });
    map.Add({
      .name = "gear",
      .type = InputActionType::Axis,
      .buttons = {ControllerButton::RightShoulder, ControllerButton::LeftShoulder}
    });
    map.Add({.name = "brake", .type = InputActionType::Axis, .trigger = ControllerTrigger::Left, .rate = 10.0f, .dead_zone = 0.0f});
    map.Add({.name = "walk", .type = InputActionType::Axis2, .stick = Stick::Left});
    map.Add({.name = "swim", .type = InputActionType::Axis2, .stick = Stick::Left, .dead_zone = 0.5f, .curve = 2.0f});
    map.Add({.name = "gas", .type = InputActionType::Axis, .trigger = ControllerTrigger::Right, .dead_zone = 0.2f, .curve = 2.0f});
    map.Add({.name = "aim", .type = InputActionType::Axis3, .sensor = Sensor::Gyro});
    map.Add({.name = "tilt", .type = InputActionType::Axis3, .sensor = Sensor::Accelerometer, .rate = 0.5f});
    map.Add({
      .name = "dpad",
      .type = InputActionType::Axis2,
      .buttons = {ControllerButton::DpadUp, ControllerButton::DpadDown, ControllerButton::DpadLeft, ControllerButton::DpadRight}
    });
    map.Add(InputMapState{
      .name = "walking",
      .actions = {"move", "look", "jump", "pause", "dpad", "turn", "aim", "tilt", "sprint-jump", "lean", "crawl", "walk", "swim"}
    });
    map.Add(InputMapState{.name = "driving", .actions = {"move", "honk", "pause", "throttle", "gear", "brake", "gas"}});
    return map;
  }

  class InputActionsTest : public ::testing::Test
  {
  protected:
    InputMap _map = WalkingAndDriving();
    InputState _input{std::make_shared<RecordingLogger>()};
    InputActions _actions;

    /// A frame in the state, with what was set on the input.
    void Frame(const std::string &state = "walking")
    {
      _actions.Refresh(_map, state, _input);
      _input.Reset();
    }
  };

  TEST_F(InputActionsTest, HasNothingDownBeforeAFrame)
  {
    EXPECT_FALSE(_actions.IsDown("jump"));
    EXPECT_FALSE(_actions.WasPressed("jump"));
    EXPECT_EQ(_actions.GetAxis("move"), glm::vec2(0.0f, 0.0f));
  }

  TEST_F(InputActionsTest, HoldsAButtonWhileAnyOfItsKeysOrButtonsIs)
  {
    _input.SetKeyDown(Key::Space);
    Frame();
    EXPECT_TRUE(_actions.IsDown("jump"));

    _input.SetControllerButtonDown(ControllerButton::South);
    Frame();
    EXPECT_TRUE(_actions.IsDown("jump"));

    Frame();
    EXPECT_FALSE(_actions.IsDown("jump"));
  }

  TEST_F(InputActionsTest, HoldsAButtonFromAChordOnlyWhileEveryKeyOfItIs)
  {
    _input.SetKeyDown(Key::Space);
    Frame();
    EXPECT_TRUE(_actions.IsDown("jump"));
    EXPECT_FALSE(_actions.IsDown("sprint-jump")) << "shift is not held";

    _input.SetKeyDown(Key::LeftShift);
    _input.SetKeyDown(Key::Space);
    Frame();
    EXPECT_TRUE(_actions.IsDown("sprint-jump"));
    EXPECT_TRUE(_actions.IsDown("jump")) << "a chord does not take the key away from a plain binding";

    _input.SetKeyDown(Key::LeftShift);
    Frame();
    EXPECT_FALSE(_actions.IsDown("sprint-jump"));
  }

  TEST_F(InputActionsTest, HoldsAButtonFromAChordOfButtonsOfAController)
  {
    _input.SetControllerButtonDown(ControllerButton::South);
    Frame();
    EXPECT_FALSE(_actions.IsDown("sprint-jump"));

    _input.SetControllerButtonDown(ControllerButton::South);
    _input.SetControllerButtonDown(ControllerButton::LeftShoulder);
    Frame();
    EXPECT_TRUE(_actions.IsDown("sprint-jump"));
  }

  TEST_F(InputActionsTest, PutsAnAxisTogetherFromChords)
  {
    _input.SetKeyDown(Key::D);
    Frame();
    EXPECT_FLOAT_EQ(_actions.GetAmount("lean"), 0.0f) << "d alone is not the chord";

    _input.SetKeyDown(Key::LeftShift);
    _input.SetKeyDown(Key::D);
    Frame();
    EXPECT_FLOAT_EQ(_actions.GetAmount("lean"), 1.0f);

    _input.SetKeyDown(Key::LeftShift);
    _input.SetKeyDown(Key::A);
    Frame();
    EXPECT_FLOAT_EQ(_actions.GetAmount("lean"), -1.0f);

    _input.SetKeyDown(Key::LeftControl);
    _input.SetKeyDown(Key::W);
    _input.SetKeyDown(Key::D);
    Frame();
    EXPECT_EQ(_actions.GetAxis("crawl"), glm::vec2(1.0f, 1.0f));
    EXPECT_EQ(_actions.GetAxis("move"), glm::vec2(1.0f, 1.0f)) << "the plain keys still move";
  }

  TEST_F(InputActionsTest, HoldsAButtonFromAButtonOfTheMouse)
  {
    _input.SetMouseButtonDown(MouseButton::Right);
    Frame("driving");
    EXPECT_TRUE(_actions.IsDown("honk"));

    _input.SetMouseButtonDown(MouseButton::Left);
    Frame("driving");
    EXPECT_FALSE(_actions.IsDown("honk"));
  }

  TEST_F(InputActionsTest, SaysWhenAButtonWentDownAndNotWhileItIsHeld)
  {
    _input.SetKeyDown(Key::Space);
    Frame();
    EXPECT_TRUE(_actions.WasPressed("jump"));

    _input.SetKeyDown(Key::Space);
    Frame();
    EXPECT_TRUE(_actions.IsDown("jump"));
    EXPECT_FALSE(_actions.WasPressed("jump"));

    Frame();
    EXPECT_FALSE(_actions.WasPressed("jump"));

    _input.SetKeyDown(Key::Space);
    Frame();
    EXPECT_TRUE(_actions.WasPressed("jump"));
  }

  TEST_F(InputActionsTest, PutsAnAxisTogetherFromFourKeys)
  {
    _input.SetKeyDown(Key::W);
    Frame();
    EXPECT_EQ(_actions.GetAxis("move"), glm::vec2(0.0f, 1.0f));
    EXPECT_TRUE(_actions.IsDown("move"));

    _input.SetKeyDown(Key::S);
    _input.SetKeyDown(Key::A);
    Frame();
    EXPECT_EQ(_actions.GetAxis("move"), glm::vec2(-1.0f, -1.0f));

    _input.SetKeyDown(Key::W);
    _input.SetKeyDown(Key::S);
    _input.SetKeyDown(Key::D);
    Frame();
    EXPECT_EQ(_actions.GetAxis("move"), glm::vec2(1.0f, 0.0f));

    Frame();
    EXPECT_EQ(_actions.GetAxis("move"), glm::vec2(0.0f, 0.0f));
    EXPECT_FALSE(_actions.IsDown("move"));
  }

  TEST_F(InputActionsTest, PutsAnAxisTogetherFromFourButtonsOfAController)
  {
    _input.SetControllerButtonDown(ControllerButton::DpadDown);
    _input.SetControllerButtonDown(ControllerButton::DpadRight);
    Frame();
    EXPECT_EQ(_actions.GetAxis("dpad"), glm::vec2(1.0f, -1.0f));
  }

  TEST_F(InputActionsTest, TakesAnAxisFromAStickWithForwardUp)
  {
    // the sticks count down as positive, the axis counts forward
    _input.SetLeftStick(0.5, -0.25);
    Frame();
    EXPECT_FLOAT_EQ(_actions.GetAxis("move").x, 0.5f);
    EXPECT_FLOAT_EQ(_actions.GetAxis("move").y, 0.25f);

    _input.SetRightStick(-1.0, 1.0);
    Frame();
    EXPECT_FLOAT_EQ(_actions.GetAxis("look").x, -1.0f);
    EXPECT_FLOAT_EQ(_actions.GetAxis("look").y, -1.0f);
  }

  TEST_F(InputActionsTest, DoesNotLetKeysAndAStickTogetherGoPastOne)
  {
    _input.SetKeyDown(Key::W);
    _input.SetKeyDown(Key::D);
    _input.SetLeftStick(0.7, -0.7);
    Frame();
    EXPECT_EQ(_actions.GetAxis("move"), glm::vec2(1.0f, 1.0f));
  }

  TEST_F(InputActionsTest, TakesAnAxisFromTheMouseInPixelsWithUpPositive)
  {
    _input.SetAxisMotion(Axis::Mouse, 12.0, -3.0);
    Frame();
    EXPECT_EQ(_actions.GetAxis("look"), glm::vec2(12.0f, 3.0f));
    EXPECT_TRUE(_actions.IsDown("look"));

    // the stick adds to it, and the pixels are not cut down to one
    _input.SetAxisMotion(Axis::Mouse, 100.0, 0.0);
    _input.SetRightStick(0.5, 0.0);
    Frame();
    EXPECT_EQ(_actions.GetAxis("look"), glm::vec2(100.5f, 0.0f));
  }

  TEST_F(InputActionsTest, TurnsAStickIntoPixelsAtItsRateForTheTimeOfTheFrame)
  {
    _input.SetRightStick(1.0, -0.5);
    _input.SetFrameTime(0.1);
    Frame();
    EXPECT_FLOAT_EQ(_actions.GetAxis("turn").x, 60.0f);
    EXPECT_FLOAT_EQ(_actions.GetAxis("turn").y, 30.0f);
    EXPECT_TRUE(_actions.IsDown("turn"));

    // half the time, half the turn; and nothing without time
    _input.SetRightStick(1.0, -0.5);
    _input.SetFrameTime(0.05);
    Frame();
    EXPECT_FLOAT_EQ(_actions.GetAxis("turn").x, 30.0f);

    _input.SetRightStick(1.0, 0.0);
    _input.SetFrameTime(0.0);
    Frame();
    EXPECT_EQ(_actions.GetAxis("turn"), glm::vec2(0.0f, 0.0f));
  }

  TEST_F(InputActionsTest, TakesNothingFromAStickInsideTheDeadZoneAndStretchesWhatIsPastIt)
  {
    // a quarter of the way unless the map says, round, so that aslant is
    // the same as straight
    _input.SetLeftStick(0.2, 0.1);
    Frame();
    EXPECT_EQ(_actions.GetAxis("walk"), glm::vec2(0.0f, 0.0f));
    EXPECT_FALSE(_actions.IsDown("walk"));

    _input.SetLeftStick(0.25, 0.0);
    Frame();
    EXPECT_EQ(_actions.GetAxis("walk"), glm::vec2(0.0f, 0.0f)) << "the edge of the zone is still inside";

    // half way is a third of what is past the zone, and all the way is one
    _input.SetLeftStick(0.5, 0.0);
    Frame();
    EXPECT_NEAR(_actions.GetAxis("walk").x, 1.0f / 3.0f, 0.0001f);

    _input.SetLeftStick(0.0, -1.0);
    Frame();
    EXPECT_FLOAT_EQ(_actions.GetAxis("walk").y, 1.0f);

    // the corner is further than one, and is cut to one in each, as a stick
    // on top of keys is
    _input.SetLeftStick(1.0, 1.0);
    Frame();
    EXPECT_EQ(_actions.GetAxis("walk"), glm::vec2(1.0f, -1.0f));
  }

  TEST_F(InputActionsTest, ShapesAStickByItsOwnDeadZoneAndCurve)
  {
    // half the way is the edge of the zone, three quarters is half of what
    // is past it, and the curve squares that
    _input.SetLeftStick(0.5, 0.0);
    Frame();
    EXPECT_EQ(_actions.GetAxis("swim"), glm::vec2(0.0f, 0.0f));

    _input.SetLeftStick(0.75, 0.0);
    Frame();
    EXPECT_FLOAT_EQ(_actions.GetAxis("swim").x, 0.25f);

    _input.SetLeftStick(1.0, 0.0);
    Frame();
    EXPECT_FLOAT_EQ(_actions.GetAxis("swim").x, 1.0f);

    // a dead zone of nothing takes the stick as it is
    _input.SetLeftStick(0.1, 0.0);
    Frame();
    EXPECT_FLOAT_EQ(_actions.GetAxis("move").x, 0.1f);
  }

  TEST_F(InputActionsTest, ShapesATriggerByItsDeadZoneAndCurve)
  {
    _input.SetTrigger(ControllerTrigger::Right, 0.2);
    Frame("driving");
    EXPECT_FLOAT_EQ(_actions.GetAmount("gas"), 0.0f);
    EXPECT_FALSE(_actions.IsDown("gas"));

    // six tenths is half of what is past the zone, squared
    _input.SetTrigger(ControllerTrigger::Right, 0.6);
    Frame("driving");
    EXPECT_FLOAT_EQ(_actions.GetAmount("gas"), 0.25f);

    _input.SetTrigger(ControllerTrigger::Right, 1.0);
    Frame("driving");
    EXPECT_FLOAT_EQ(_actions.GetAmount("gas"), 1.0f);
  }

  TEST_F(InputActionsTest, TakesAnAmountFromTwoKeys)
  {
    _input.SetKeyDown(Key::W);
    Frame("driving");
    EXPECT_FLOAT_EQ(_actions.GetAmount("throttle"), 1.0f);
    EXPECT_TRUE(_actions.IsDown("throttle"));

    _input.SetKeyDown(Key::S);
    Frame("driving");
    EXPECT_FLOAT_EQ(_actions.GetAmount("throttle"), -1.0f);

    _input.SetKeyDown(Key::W);
    _input.SetKeyDown(Key::S);
    Frame("driving");
    EXPECT_FLOAT_EQ(_actions.GetAmount("throttle"), 0.0f);
    EXPECT_FALSE(_actions.IsDown("throttle"));
  }

  TEST_F(InputActionsTest, TakesAnAmountFromTwoButtonsOfAController)
  {
    _input.SetControllerButtonDown(ControllerButton::LeftShoulder);
    Frame("driving");
    EXPECT_FLOAT_EQ(_actions.GetAmount("gear"), -1.0f);

    _input.SetControllerButtonDown(ControllerButton::RightShoulder);
    Frame("driving");
    EXPECT_FLOAT_EQ(_actions.GetAmount("gear"), 1.0f);
  }

  TEST_F(InputActionsTest, TakesAnAmountFromATriggerAsFarAsItIsPulled)
  {
    _input.SetTrigger(ControllerTrigger::Right, 0.5);
    Frame("driving");
    EXPECT_FLOAT_EQ(_actions.GetAmount("throttle"), 0.5f);
    EXPECT_TRUE(_actions.IsDown("throttle"));

    _input.SetTrigger(ControllerTrigger::Left, 0.5);
    Frame("driving");
    EXPECT_FLOAT_EQ(_actions.GetAmount("throttle"), 0.0f) << "the other trigger";
  }

  TEST_F(InputActionsTest, LetsTheLargestOfKeysAndTriggerWin)
  {
    _input.SetTrigger(ControllerTrigger::Right, 0.5);
    _input.SetKeyDown(Key::W);
    Frame("driving");
    EXPECT_FLOAT_EQ(_actions.GetAmount("throttle"), 1.0f);

    // the key that goes the other way is further from zero than the trigger
    _input.SetTrigger(ControllerTrigger::Right, 0.5);
    _input.SetKeyDown(Key::S);
    Frame("driving");
    EXPECT_FLOAT_EQ(_actions.GetAmount("throttle"), -1.0f);
  }

  TEST_F(InputActionsTest, ScalesATriggerByItsRateForTheTimeOfTheFrame)
  {
    _input.SetTrigger(ControllerTrigger::Left, 0.5);
    _input.SetFrameTime(0.2);
    Frame("driving");
    EXPECT_FLOAT_EQ(_actions.GetAmount("brake"), 1.0f);
  }

  TEST_F(InputActionsTest, HasNoAmountForAnAxisOfTwoAndNoAxisForAnAxisOfOne)
  {
    _input.SetKeyDown(Key::W);
    Frame("driving");
    EXPECT_FLOAT_EQ(_actions.GetAmount("move"), 0.0f);
    EXPECT_EQ(_actions.GetAxis("throttle"), glm::vec2(0.0f, 0.0f));
  }

  TEST_F(InputActionsTest, TurnsAGyroIntoRadiansTurnedInTheFrame)
  {
    // radians a second about x, y, and z, for a tenth of a second
    _input.SetSensor(Sensor::Gyro, 1.0, -2.0, 0.5);
    _input.SetFrameTime(0.1);
    Frame();
    EXPECT_FLOAT_EQ(_actions.GetAxis3("aim").x, 0.1f);
    EXPECT_FLOAT_EQ(_actions.GetAxis3("aim").y, -0.2f);
    EXPECT_FLOAT_EQ(_actions.GetAxis3("aim").z, 0.05f);
    EXPECT_TRUE(_actions.IsDown("aim"));

    Frame();
    EXPECT_EQ(_actions.GetAxis3("aim"), glm::vec3(0.0f, 0.0f, 0.0f)) << "nothing read, as while the sensor is off";
    EXPECT_FALSE(_actions.IsDown("aim"));
  }

  TEST_F(InputActionsTest, TakesAnAccelerometerAsItIsAtItsRate)
  {
    _input.SetSensor(Sensor::Accelerometer, 0.0, 9.8, 2.0);
    _input.SetFrameTime(0.1);
    Frame();
    EXPECT_FLOAT_EQ(_actions.GetAxis3("tilt").y, 4.9f);
    EXPECT_FLOAT_EQ(_actions.GetAxis3("tilt").z, 1.0f);
  }

  TEST_F(InputActionsTest, HasNoAxisOfThreeOutsideTheStateOrForAnotherType)
  {
    _input.SetSensor(Sensor::Gyro, 1.0, 1.0, 1.0);
    _input.SetFrameTime(0.1);
    Frame("driving");
    EXPECT_EQ(_actions.GetAxis3("aim"), glm::vec3(0.0f, 0.0f, 0.0f));
    EXPECT_EQ(_actions.GetAxis3("move"), glm::vec3(0.0f, 0.0f, 0.0f));
  }

  TEST_F(InputActionsTest, DoesNotFireAnActionOutsideTheState)
  {
    _input.SetKeyDown(Key::Space);
    _input.SetKeyDown(Key::H);
    Frame("driving");
    EXPECT_FALSE(_actions.IsDown("jump"));
    EXPECT_TRUE(_actions.IsDown("honk"));

    _input.SetKeyDown(Key::Space);
    _input.SetKeyDown(Key::H);
    Frame("walking");
    EXPECT_TRUE(_actions.IsDown("jump"));
    EXPECT_FALSE(_actions.IsDown("honk"));
  }

  TEST_F(InputActionsTest, FiresAnActionOfEveryStateItIsIn)
  {
    _input.SetKeyDown(Key::Escape);
    Frame("walking");
    EXPECT_TRUE(_actions.IsDown("pause"));

    _input.SetKeyDown(Key::Escape);
    Frame("driving");
    EXPECT_TRUE(_actions.IsDown("pause"));
    EXPECT_FALSE(_actions.WasPressed("pause")) << "held across the states, not pressed again";
  }

  TEST_F(InputActionsTest, HasNothingDownInAStateTheMapDoesNotHave)
  {
    _input.SetKeyDown(Key::Escape);
    Frame("swimming");
    EXPECT_FALSE(_actions.IsDown("pause"));
  }

  TEST_F(InputActionsTest, HasNothingForAnActionTheMapDoesNotHave)
  {
    _input.SetKeyDown(Key::Space);
    Frame();
    EXPECT_FALSE(_actions.IsDown("fly"));
    EXPECT_FALSE(_actions.WasPressed("fly"));
    EXPECT_EQ(_actions.GetAxis("fly"), glm::vec2(0.0f, 0.0f));
  }

  TEST_F(InputActionsTest, FiresAButtonThatIsHeldByItsName)
  {
    _input.HoldAction("jump");
    _input.HoldAction("honk");
    Frame("walking");
    EXPECT_TRUE(_actions.IsDown("jump"));
    EXPECT_FALSE(_actions.IsDown("honk")) << "held, and not in the state";
  }

  TEST_F(InputActionsTest, ForgetsTheFrameWhenReset)
  {
    _input.SetKeyDown(Key::Space);
    Frame();
    _actions.Reset();

    EXPECT_FALSE(_actions.IsDown("jump"));

    _input.SetKeyDown(Key::Space);
    Frame();
    EXPECT_TRUE(_actions.WasPressed("jump"));
  }

  TEST_F(InputActionsTest, FollowsAMapThatChangedBetweenFrames)
  {
    _input.SetKeyDown(Key::Space);
    Frame();

    InputMap changed;
    changed.Add({.name = "fly", .keys = {Key::F}});
    changed.Add({.name = "jump", .keys = {Key::Space}});
    changed.Add(InputMapState{.name = "walking", .actions = {"fly", "jump"}});

    _input.SetKeyDown(Key::F);
    _input.SetKeyDown(Key::Space);
    _actions.Refresh(changed, "walking", _input);

    EXPECT_TRUE(_actions.WasPressed("fly"));
    EXPECT_TRUE(_actions.IsDown("jump"));
    EXPECT_FALSE(_actions.WasPressed("jump")) << "down in the frame before as well";
    EXPECT_FALSE(_actions.IsDown("move"));
  }
} // namespace
