#include "ui-text-measure.hpp"

#include <gtest/gtest.h>

#include <neon/testing/fake-font-rasterizer.hpp>

// Where the caret is at a byte, and which byte is at a place, with the fake
// font: every character is half the size wide, and a line is the size high.
// At a size of 16, a character is 8 wide and a line 16 high.

namespace
{
  using neon::Atlas_UiTextMeasure;
  using neon::FontAtlas;
  using neon::TextAlign;
  using neon::UiFont;
  using neon::UiTextMeasure;
  using neon::testing::FakeFontRasterizer;

  class UiTextMeasureTest : public ::testing::Test
  {
  protected:
    FakeFontRasterizer _rasterizer;
    UiFont _font;
    const Atlas_UiTextMeasure &_measure = Atlas_UiTextMeasure::Get();

    void SetUp() override
    {
      std::string error;
      const int font = _rasterizer.LoadFont(FakeFontRasterizer::AFont());
      ASSERT_TRUE(_font.atlas.Build(_rasterizer, font, 16.0f, FontAtlas::DefaultCharacters(), error)) << error;
    }

    [[nodiscard]] UiTextMeasure::Request Request(const std::string &text, const float max_width = -1.0f) const
    {
      UiTextMeasure::Request request;
      request.font = &_font;
      request.text = text;
      if (max_width >= 0.0f) { request.options.max_width = max_width; }
      return request;
    }
  };

  TEST_F(UiTextMeasureTest, ACaretIsAtTheStartOfItsCharacter)
  {
    const auto request = Request("hello");

    EXPECT_FLOAT_EQ(_measure.CaretAt(request, 0).x, 0.0f);
    EXPECT_FLOAT_EQ(_measure.CaretAt(request, 2).x, 16.0f);
    EXPECT_FLOAT_EQ(_measure.CaretAt(request, 5).x, 40.0f);
    EXPECT_FLOAT_EQ(_measure.CaretAt(request, 0).height, 16.0f);
    EXPECT_EQ(_measure.CaretAt(request, 5).line, 0u);
  }

  TEST_F(UiTextMeasureTest, CountsACharacterOfSeveralBytesOnce)
  {
    // e with an accent is two bytes, and a character of Japanese three
    const auto request = Request("\xC3\xA9\xE6\x97\xA5x");

    EXPECT_FLOAT_EQ(_measure.CaretAt(request, 2).x, 8.0f);
    EXPECT_FLOAT_EQ(_measure.CaretAt(request, 5).x, 16.0f);
    EXPECT_FLOAT_EQ(_measure.CaretAt(request, 6).x, 24.0f);
  }

  TEST_F(UiTextMeasureTest, TheOffsetAtAPlaceIsTheNearerSideOfTheCharacter)
  {
    const auto request = Request("hello");

    EXPECT_EQ(_measure.OffsetAt(request, 0.0f, 0.0f), 0u);
    EXPECT_EQ(_measure.OffsetAt(request, 3.0f, 0.0f), 0u);
    EXPECT_EQ(_measure.OffsetAt(request, 5.0f, 0.0f), 1u);
    EXPECT_EQ(_measure.OffsetAt(request, 19.0f, 0.0f), 2u);
    EXPECT_EQ(_measure.OffsetAt(request, 100.0f, 0.0f), 5u) << "behind the text";
    EXPECT_EQ(_measure.OffsetAt(request, -10.0f, 0.0f), 0u) << "in front of it";
  }

  TEST_F(UiTextMeasureTest, TheOffsetAtAPlaceDoesNotSplitACharacter)
  {
    const auto request = Request("\xC3\xA9x");

    EXPECT_EQ(_measure.OffsetAt(request, 6.0f, 0.0f), 2u);
    EXPECT_EQ(_measure.OffsetAt(request, 20.0f, 0.0f), 3u);
  }

  TEST_F(UiTextMeasureTest, BreaksLinesAtSpacesAsTheTextIsPlaced)
  {
    // 80 wide is ten characters: "hello big" fits, "world" goes down
    const auto request = Request("hello big world", 80.0f);

    const auto lines = _measure.LinesOf(request);
    ASSERT_EQ(lines.size(), 2u);
    EXPECT_EQ(lines[0].start, 0u);
    EXPECT_EQ(lines[0].end, 9u) << "without the space that broke it";
    EXPECT_EQ(lines[1].start, 10u);
    EXPECT_EQ(lines[1].end, 15u);
    EXPECT_FLOAT_EQ(lines[1].top, 16.0f);
    EXPECT_FLOAT_EQ(lines[0].width, 72.0f);

    // the caret at the end of the first line stands at the start of the
    // second, as a browser does after a break
    EXPECT_EQ(_measure.CaretAt(request, 10).line, 1u);
    EXPECT_FLOAT_EQ(_measure.CaretAt(request, 10).x, 0.0f);
    EXPECT_EQ(_measure.CaretAt(request, 9).line, 0u);

    // a place on the second line
    EXPECT_EQ(_measure.OffsetAt(request, 19.0f, 20.0f), 12u);
    EXPECT_EQ(_measure.OffsetAt(request, 21.0f, 20.0f), 13u);
    EXPECT_EQ(_measure.OffsetAt(request, 200.0f, 20.0f), 15u);

    const auto size = _measure.SizeOf(request);
    EXPECT_FLOAT_EQ(size.width, 72.0f);
    EXPECT_FLOAT_EQ(size.height, 32.0f);
  }

  TEST_F(UiTextMeasureTest, ALineFeedEndsALine)
  {
    const auto request = Request("ab\ncd");

    const auto lines = _measure.LinesOf(request);
    ASSERT_EQ(lines.size(), 2u);
    EXPECT_EQ(lines[0].end, 2u);
    EXPECT_EQ(lines[1].start, 3u);

    // the caret before the line feed is at the end of the first line, and
    // after it at the start of the second
    EXPECT_EQ(_measure.CaretAt(request, 2).line, 0u);
    EXPECT_FLOAT_EQ(_measure.CaretAt(request, 2).x, 16.0f);
    EXPECT_EQ(_measure.CaretAt(request, 3).line, 1u);

    // behind the last character of the first line is in front of the line
    // feed
    EXPECT_EQ(_measure.OffsetAt(request, 100.0f, 0.0f), 2u);
  }

  TEST_F(UiTextMeasureTest, DoesNotBreakAWordUnlessAskedTo)
  {
    auto request = Request("abcdefghijkl", 40.0f);
    EXPECT_EQ(_measure.LinesOf(request).size(), 1u);

    request.breaks_long_words = true;
    const auto lines = _measure.LinesOf(request);
    ASSERT_EQ(lines.size(), 3u);
    EXPECT_EQ(lines[0].end, 5u);
    EXPECT_EQ(lines[1].start, 5u);
    EXPECT_EQ(lines[2].start, 10u);
    EXPECT_EQ(lines[2].end, 12u);
  }

  TEST_F(UiTextMeasureTest, FollowsTheAlignmentOfTheText)
  {
    auto request = Request("ab");
    request.options.box_width = 100.0f;
    request.options.align = TextAlign::Right;

    EXPECT_FLOAT_EQ(_measure.CaretAt(request, 0).x, 84.0f);
    EXPECT_EQ(_measure.OffsetAt(request, 90.0f, 0.0f), 1u);

    request.options.align = TextAlign::Center;
    EXPECT_FLOAT_EQ(_measure.CaretAt(request, 0).x, 42.0f);
  }

  TEST_F(UiTextMeasureTest, AnEmptyTextHasOneEmptyLine)
  {
    const auto request = Request("");

    const auto lines = _measure.LinesOf(request);
    ASSERT_EQ(lines.size(), 1u);
    EXPECT_FLOAT_EQ(lines[0].height, 16.0f);
    EXPECT_FLOAT_EQ(_measure.CaretAt(request, 0).x, 0.0f);
    EXPECT_FLOAT_EQ(_measure.CaretAt(request, 0).height, 16.0f);
    EXPECT_EQ(_measure.OffsetAt(request, 10.0f, 10.0f), 0u);
    EXPECT_FLOAT_EQ(_measure.SizeOf(request).height, 16.0f);
  }

  TEST_F(UiTextMeasureTest, WithoutAFontThereIsNothing)
  {
    UiTextMeasure::Request request;
    request.text = "hello";

    EXPECT_TRUE(_measure.LinesOf(request).empty());
    EXPECT_FLOAT_EQ(_measure.CaretAt(request, 3).x, 0.0f);
    EXPECT_EQ(_measure.OffsetAt(request, 30.0f, 0.0f), 0u);
    EXPECT_FLOAT_EQ(_measure.SizeOf(request).width, 0.0f);
  }
}
