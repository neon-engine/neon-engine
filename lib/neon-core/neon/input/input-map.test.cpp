#include "input-map.hpp"

#include <gtest/gtest.h>

#include <neon/testing/print-chord.hpp>

namespace
{
  using neon::Chord;
  using neon::ControllerButton;
  using neon::InputAction;
  using neon::InputActionType;
  using neon::InputMap;
  using neon::InputMapState;
  using neon::Key;
  using neon::Stick;

  TEST(InputMapTest, AChordIsOneKeyOrSeveralHeldTogether)
  {
    const Chord<Key> one = Key::W;
    const Chord<Key> together = {Key::LeftShift, Key::W};
    const Chord<Key> none;

    EXPECT_EQ(one.parts, (std::vector{Key::W}));
    EXPECT_EQ(together.parts, (std::vector{Key::LeftShift, Key::W}));
    EXPECT_EQ(one, Chord<Key>{Key::W});
    EXPECT_NE(one, together);

    const auto shift_is_down = [](const Key key) { return key == Key::LeftShift; };
    EXPECT_FALSE(one.IsDown(shift_is_down));
    EXPECT_FALSE(together.IsDown(shift_is_down)) << "w is not down";
    EXPECT_TRUE(together.IsDown([](const Key) { return true; }));
    EXPECT_FALSE(none.IsDown([](const Key) { return true; })) << "nothing to hold";
  }

  TEST(InputMapTest, StartsEmptyAndWithoutAState)
  {
    const InputMap map;

    EXPECT_TRUE(map.GetActions().empty());
    EXPECT_TRUE(map.GetStates().empty());
    EXPECT_EQ(map.GetFirstState(), "");
    EXPECT_EQ(map.FindAction("move"), nullptr);
    EXPECT_EQ(map.FindState("walking"), nullptr);
  }

  TEST(InputMapTest, FindsActionsAndStatesByTheirNames)
  {
    InputMap map;
    map.Add(InputAction{.name = "jump", .keys = {Key::Space}});
    map.Add(InputMapState{.name = "walking", .actions = {"jump"}});
    map.Add(InputMapState{.name = "menu"});

    ASSERT_NE(map.FindAction("jump"), nullptr);
    EXPECT_EQ(map.FindAction("jump")->keys.size(), 1u);
    ASSERT_NE(map.FindState("walking"), nullptr);
    EXPECT_TRUE(map.FindState("walking")->Has("jump"));
    EXPECT_FALSE(map.FindState("menu")->Has("jump"));
    EXPECT_EQ(map.GetFirstState(), "walking");
  }

  TEST(InputMapTest, ReplacesAnActionOrAStateOfTheSameName)
  {
    InputMap map;
    map.Add(InputAction{.name = "jump", .keys = {Key::Space}});
    map.Add(InputAction{.name = "jump", .keys = {Key::Enter}});
    map.Add(InputMapState{.name = "walking", .actions = {"jump"}});
    map.Add(InputMapState{.name = "walking"});

    ASSERT_EQ(map.GetActions().size(), 1u);
    EXPECT_EQ(map.GetActions().front().keys.front(), Chord<Key>{Key::Enter});
    ASSERT_EQ(map.GetStates().size(), 1u);
    EXPECT_TRUE(map.GetStates().front().actions.empty());
  }

  TEST(InputMapTest, PrintsAChordByTheNamesOfItsKeysOrButtons)
  {
    // as an input map file writes them, so that a failed expectation on a
    // binding reads as one
    EXPECT_EQ(::testing::PrintToString(Chord<Key>{Key::W}), "w");
    EXPECT_EQ(::testing::PrintToString(Chord<Key>{Key::LeftShift, Key::W}), "[left-shift, w]");
    EXPECT_EQ(::testing::PrintToString(Chord<Key>{}), "[]");
    EXPECT_EQ(::testing::PrintToString(Chord<ControllerButton>{ControllerButton::LeftShoulder, ControllerButton::South}),
              "[left-shoulder, south]");
    EXPECT_EQ(::testing::PrintToString(std::vector<Chord<Key>>{Key::Space, {Key::LeftControl, Key::Digit1}}),
              "{ space, [left-control, 1] }");
  }

  TEST(InputMapTest, HasADefaultThatPlaysAsTheEngineDidBeforeAProjectCouldSay)
  {
    const InputMap map = InputMap::Default();

    const auto *move = map.FindAction("move");
    ASSERT_NE(move, nullptr);
    EXPECT_EQ(move->type, InputActionType::Axis2);
    EXPECT_EQ(move->keys, (std::vector<Chord<Key>>{Key::W, Key::S, Key::A, Key::D}));
    EXPECT_EQ(move->stick, Stick::Left);
    EXPECT_EQ(move->dead_zone, InputAction::default_dead_zone);

    const auto *look = map.FindAction("look");
    ASSERT_NE(look, nullptr);
    EXPECT_EQ(look->type, InputActionType::Axis2);
    EXPECT_TRUE(look->mouse_motion);
    EXPECT_EQ(look->stick, Stick::Right);
    EXPECT_EQ(look->rate, 600.0f);
    EXPECT_EQ(look->dead_zone, InputAction::default_dead_zone);

    const auto *jump = map.FindAction("jump");
    ASSERT_NE(jump, nullptr);
    EXPECT_EQ(jump->type, InputActionType::Button);
    EXPECT_EQ(jump->keys, (std::vector<Chord<Key>>{Key::Space}));
    EXPECT_EQ(jump->buttons, (std::vector<Chord<ControllerButton>>{ControllerButton::South}));

    const auto *run = map.FindAction("run");
    ASSERT_NE(run, nullptr);
    EXPECT_EQ(run->type, InputActionType::Button);
    EXPECT_EQ(run->keys, (std::vector<Chord<Key>>{Key::LeftShift}));
    EXPECT_EQ(run->buttons, (std::vector<Chord<ControllerButton>>{ControllerButton::LeftStick}));

    const auto *pause = map.FindAction("pause");
    ASSERT_NE(pause, nullptr);
    EXPECT_EQ(pause->type, InputActionType::Button);
    EXPECT_EQ(pause->keys, (std::vector<Chord<Key>>{Key::Escape}));
    EXPECT_EQ(pause->buttons, (std::vector<Chord<ControllerButton>>{ControllerButton::Start}));

    EXPECT_EQ(map.GetFirstState(), "playing");
    EXPECT_TRUE(map.FindState("playing")->Has("move"));
    EXPECT_TRUE(map.FindState("playing")->Has("look"));
    EXPECT_TRUE(map.FindState("playing")->Has("jump"));
    EXPECT_TRUE(map.FindState("playing")->Has("run"));
    EXPECT_TRUE(map.FindState("playing")->Has("pause"));
    ASSERT_NE(map.FindState("menu"), nullptr);
    EXPECT_EQ(map.FindState("menu")->actions, (std::vector<std::string>{"pause"}));
  }
} // namespace
