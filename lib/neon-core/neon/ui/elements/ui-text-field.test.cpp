#include "ui-text-field.hpp"

#include <gtest/gtest.h>

// Which directions a text that is typed into keeps for itself, and which it
// leaves to the focus: a controller has to get past a field of one line
// with up and down (#159). A text of several lines gives them up from its
// first and its last line, which the headless walk of the settings menu
// checks, since it takes a font to lay the lines out.
namespace
{
  using neon::Key;
  using neon::UiInput;
  using neon::UiTextArea;

  TEST(UiInputElementTest, KeepsLeftAndRightForItsCaret)
  {
    const UiInput input;

    EXPECT_TRUE(input.UsesDirection(Key::Left));
    EXPECT_TRUE(input.UsesDirection(Key::Right));
  }

  TEST(UiInputElementTest, LeavesUpAndDownToTheFocusSinceItHasNoLines)
  {
    const UiInput input;

    EXPECT_FALSE(input.UsesDirection(Key::Up));
    EXPECT_FALSE(input.UsesDirection(Key::Down));
  }

  TEST(UiTextAreaElementTest, KeepsEveryDirectionForItsCaret)
  {
    const UiTextArea area;

    EXPECT_TRUE(area.UsesDirection(Key::Left));
    EXPECT_TRUE(area.UsesDirection(Key::Right));
    EXPECT_TRUE(area.UsesDirection(Key::Up));
    EXPECT_TRUE(area.UsesDirection(Key::Down));
  }
}
