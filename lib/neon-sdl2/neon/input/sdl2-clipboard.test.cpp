#include "sdl2-clipboard.hpp"

#include <gtest/gtest.h>

#include <neon/testing/sdl2-without-display.hpp>

namespace
{
  using neon::SDL2_Clipboard;
  using neon::testing::Sdl2WithoutDisplay;

  class SDL2ClipboardTest : public Sdl2WithoutDisplay
  {
  protected:
    SDL2_Clipboard _clipboard;
  };

  TEST_F(SDL2ClipboardTest, HandsBackWhatItWasGiven)
  {
    ASSERT_TRUE(_clipboard.SetText("copied"));

    EXPECT_TRUE(_clipboard.HasText());
    EXPECT_EQ(_clipboard.GetText(), "copied");
  }

  TEST_F(SDL2ClipboardTest, KeepsTextOfSeveralBytes)
  {
    const std::string text = "Zo\xC3\xAB \xE6\x97\xA5\xE6\x9C\xAC e\xCC\x81";
    ASSERT_TRUE(_clipboard.SetText(text));
    EXPECT_EQ(_clipboard.GetText(), text);
  }

  TEST_F(SDL2ClipboardTest, ReplacesWhatWasThere)
  {
    ASSERT_TRUE(_clipboard.SetText("first"));
    ASSERT_TRUE(_clipboard.SetText("second"));
    EXPECT_EQ(_clipboard.GetText(), "second");
  }

  TEST_F(SDL2ClipboardTest, HasNoTextAfterAnEmptyOneWasSet)
  {
    ASSERT_TRUE(_clipboard.SetText("something"));
    ASSERT_TRUE(_clipboard.SetText(""));

    EXPECT_FALSE(_clipboard.HasText());
    EXPECT_EQ(_clipboard.GetText(), "");
  }
} // namespace
