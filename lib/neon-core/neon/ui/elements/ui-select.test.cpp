#include "ui-select.hpp"

#include <string>
#include <vector>

#include <gtest/gtest.h>

// Which directions a dropdown keeps for itself, and which it leaves to the
// focus: closed, a controller has to get past it to the next row (#159).
namespace
{
  using neon::Key;
  using neon::UiFrame;
  using neon::UiInteraction;
  using neon::UiSelect;

  class UiSelectTest : public ::testing::Test
  {
  protected:
    UiSelect _select;
    UiFrame _frame;

    void SetUp() override
    {
      std::string error;
      ASSERT_TRUE(_select.SetField("options", std::vector<std::string>{"low", "medium", "high"}, error)) << error;
      ASSERT_TRUE(_select.SetField("value", std::string{"medium"}, error)) << error;
    }

    bool Press(const UiInteraction::Kind kind, const Key key = Key::Unknown)
    {
      UiInteraction interaction;
      interaction.kind = kind;
      interaction.key = key;
      _select.Interact(interaction, _frame);
      return interaction.is_used;
    }
  };

  TEST_F(UiSelectTest, LeavesEveryDirectionToTheFocusWhileClosed)
  {
    EXPECT_FALSE(_select.UsesDirection(Key::Up));
    EXPECT_FALSE(_select.UsesDirection(Key::Down));
    EXPECT_FALSE(_select.UsesDirection(Key::Left));
    EXPECT_FALSE(_select.UsesDirection(Key::Right));
  }

  TEST_F(UiSelectTest, ChangesNothingByADirectionWhileClosed)
  {
    EXPECT_FALSE(Press(UiInteraction::Kind::Direction, Key::Down));
    EXPECT_FALSE(Press(UiInteraction::Kind::Direction, Key::Up));

    EXPECT_EQ(_select.GetValue(), "medium");
  }

  TEST_F(UiSelectTest, TakesEveryDirectionOnceAcceptOpenedIt)
  {
    EXPECT_TRUE(Press(UiInteraction::Kind::Accept));

    EXPECT_TRUE(_select.UsesDirection(Key::Up));
    EXPECT_TRUE(_select.UsesDirection(Key::Down));
    EXPECT_TRUE(_select.UsesDirection(Key::Left));
    EXPECT_TRUE(_select.UsesDirection(Key::Right));
  }

  TEST_F(UiSelectTest, MovesThroughTheOpenListAndChoosesWithAccept)
  {
    EXPECT_TRUE(Press(UiInteraction::Kind::Accept));
    EXPECT_TRUE(Press(UiInteraction::Kind::Direction, Key::Down));
    EXPECT_EQ(_select.GetValue(), "medium") << "moving through the list chooses nothing yet";

    EXPECT_TRUE(Press(UiInteraction::Kind::Accept));
    EXPECT_EQ(_select.GetValue(), "high");
    EXPECT_FALSE(_select.UsesDirection(Key::Down)) << "accept closed the list";
  }

  TEST_F(UiSelectTest, KeepsTheFocusInsideTheOpenListSideways)
  {
    EXPECT_TRUE(Press(UiInteraction::Kind::Accept));
    EXPECT_TRUE(Press(UiInteraction::Kind::Direction, Key::Left));
    EXPECT_TRUE(Press(UiInteraction::Kind::Direction, Key::Right));
    EXPECT_EQ(_select.GetValue(), "medium");
  }

  TEST_F(UiSelectTest, ClosesWithCancelAndLeavesTheDirectionsAgain)
  {
    EXPECT_TRUE(Press(UiInteraction::Kind::Accept));
    EXPECT_TRUE(Press(UiInteraction::Kind::Direction, Key::Up));
    EXPECT_TRUE(Press(UiInteraction::Kind::Cancel));

    EXPECT_FALSE(_select.UsesDirection(Key::Up));
    EXPECT_EQ(_select.GetValue(), "medium") << "cancel keeps what was chosen before";
  }
}
