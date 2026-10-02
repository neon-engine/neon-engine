#include "input-map.hpp"

#include <gtest/gtest.h>

namespace
{
  using neon::ControllerButton;
  using neon::InputAction;
  using neon::InputActionType;
  using neon::InputMap;
  using neon::InputMapState;
  using neon::Key;
  using neon::Stick;

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
    EXPECT_EQ(map.GetActions().front().keys.front(), Key::Enter);
    ASSERT_EQ(map.GetStates().size(), 1u);
    EXPECT_TRUE(map.GetStates().front().actions.empty());
  }

  TEST(InputMapTest, HasADefaultThatPlaysAsTheEngineDidBeforeAProjectCouldSay)
  {
    const InputMap map = InputMap::Default();

    const auto *move = map.FindAction("move");
    ASSERT_NE(move, nullptr);
    EXPECT_EQ(move->type, InputActionType::Axis2);
    EXPECT_EQ(move->keys, (std::vector{Key::W, Key::S, Key::A, Key::D}));
    EXPECT_EQ(move->stick, Stick::Left);

    const auto *look = map.FindAction("look");
    ASSERT_NE(look, nullptr);
    EXPECT_EQ(look->type, InputActionType::Axis2);
    EXPECT_TRUE(look->mouse_motion);
    EXPECT_EQ(look->stick, Stick::Right);
    EXPECT_EQ(look->rate, 600.0f);

    const auto *jump = map.FindAction("jump");
    ASSERT_NE(jump, nullptr);
    EXPECT_EQ(jump->type, InputActionType::Button);
    EXPECT_EQ(jump->keys, (std::vector{Key::Space}));
    EXPECT_EQ(jump->buttons, (std::vector{ControllerButton::South}));

    const auto *run = map.FindAction("run");
    ASSERT_NE(run, nullptr);
    EXPECT_EQ(run->type, InputActionType::Button);
    EXPECT_EQ(run->keys, (std::vector{Key::LeftShift}));
    EXPECT_TRUE(run->buttons.empty());

    const auto *pause = map.FindAction("pause");
    ASSERT_NE(pause, nullptr);
    EXPECT_EQ(pause->type, InputActionType::Button);
    EXPECT_EQ(pause->keys, (std::vector{Key::Escape}));
    EXPECT_EQ(pause->buttons, (std::vector{ControllerButton::Start}));

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
