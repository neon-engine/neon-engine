#include "text-layout.hpp"

#include <cmath>
#include <string>

#include <gtest/gtest.h>

#include <neon/testing/fake-font-rasterizer.hpp>
#include <neon/text/utf8.hpp>

// The font of the tests moves the pen by half its size for every character.
// At size 20 that is 10, and a line is 20 high. A picture is 8 by 14, starts
// 1 right of the pen, and stands on the baseline, which is 16 below the top
// of its line.

namespace
{
  using neon::FontAtlas;
  using neon::PlacedText;
  using neon::PlaceText;
  using neon::TextAlign;
  using neon::TextOptions;
  using neon::testing::FakeFontRasterizer;

  class TextLayoutTest : public ::testing::Test
  {
  protected:
    FakeFontRasterizer _rasterizer;
    int _font = _rasterizer.LoadFont(FakeFontRasterizer::AFont());
    FontAtlas _atlas;

    void SetUp() override
    {
      Build(20);
    }

    void Build(const float pixel_size)
    {
      std::string error;
      ASSERT_TRUE(_atlas.Build(_rasterizer, _font, pixel_size, FontAtlas::DefaultCharacters(), error)) << error;
    }

    [[nodiscard]] PlacedText Place(const std::string &text, const TextOptions &options = {}) const
    {
      return PlaceText(_atlas, neon::DecodeUtf8(text), options);
    }

    [[nodiscard]] static TextOptions NoWiderThan(const float width)
    {
      TextOptions options;
      options.max_width = width;
      return options;
    }
  };

  TEST_F(TextLayoutTest, AnEmptyTextHasNoSize)
  {
    const auto placed = Place("");

    EXPECT_FLOAT_EQ(placed.width, 0);
    EXPECT_FLOAT_EQ(placed.height, 0);
    EXPECT_EQ(placed.lines, 0u);
    EXPECT_TRUE(placed.glyphs.empty());
  }

  TEST_F(TextLayoutTest, MeasuresOneLine)
  {
    const auto placed = Place("Health");

    EXPECT_FLOAT_EQ(placed.width, 60);
    EXPECT_FLOAT_EQ(placed.height, 20);
    EXPECT_EQ(placed.lines, 1u);
  }

  TEST_F(TextLayoutTest, PlacesEveryCharacterAtThePen)
  {
    const auto placed = Place("abc");

    ASSERT_EQ(placed.glyphs.size(), 3u);

    // 1 right of the pen, and with its 14 rows ending on the baseline at 16
    EXPECT_FLOAT_EQ(placed.glyphs[0].x, 1);
    EXPECT_FLOAT_EQ(placed.glyphs[1].x, 11);
    EXPECT_FLOAT_EQ(placed.glyphs[2].x, 21);
    for (const auto &glyph : placed.glyphs) { EXPECT_FLOAT_EQ(glyph.y, 2); }

    EXPECT_EQ(placed.glyphs[0].glyph, _atlas.Find(U'a'));
    EXPECT_EQ(placed.glyphs[2].glyph, _atlas.Find(U'c'));
  }

  TEST_F(TextLayoutTest, ASpaceMovesThePenAndPlacesNothing)
  {
    const auto placed = Place("a b");

    ASSERT_EQ(placed.glyphs.size(), 2u);
    EXPECT_FLOAT_EQ(placed.glyphs[1].x, 21);
    EXPECT_FLOAT_EQ(placed.width, 30);
  }

  TEST_F(TextLayoutTest, ATabCountsAsASpace)
  {
    EXPECT_FLOAT_EQ(Place("a\tb").width, 30);
  }

  TEST_F(TextLayoutTest, CountsCharactersAndNotBytes)
  {
    // five characters in ten bytes
    EXPECT_FLOAT_EQ(Place("\xC3\xA4\xC3\xB6\xC3\xBC\xC3\x9F\xC3\xA9").width, 50);
  }

  TEST_F(TextLayoutTest, ACharacterTheFontDoesNotHaveIsDrawnAsTheReplacementCharacter)
  {
    // a Chinese character
    const auto placed = Place("a\xE4\xB8\xAD");

    ASSERT_EQ(placed.glyphs.size(), 2u);
    EXPECT_EQ(placed.glyphs[1].glyph, _atlas.Find(neon::Replacement_Character));
    EXPECT_FLOAT_EQ(placed.width, 20);
  }

  TEST_F(TextLayoutTest, ALineFeedStartsANewLine)
  {
    const auto placed = Place("ab\ncdef");

    EXPECT_EQ(placed.lines, 2u);
    EXPECT_FLOAT_EQ(placed.width, 40);
    EXPECT_FLOAT_EQ(placed.height, 40);

    ASSERT_EQ(placed.glyphs.size(), 6u);
    EXPECT_FLOAT_EQ(placed.glyphs[2].x, 1);
    EXPECT_FLOAT_EQ(placed.glyphs[2].y, 22);
  }

  TEST_F(TextLayoutTest, ACarriageReturnIsLeftOut)
  {
    const auto placed = Place("ab\r\ncd");

    EXPECT_EQ(placed.lines, 2u);
    EXPECT_FLOAT_EQ(placed.width, 20);
    EXPECT_EQ(placed.glyphs.size(), 4u);
  }

  TEST_F(TextLayoutTest, ASoftHyphenIsLeftOut)
  {
    const auto placed = Place("ab\xC2\xAD" "cd");

    EXPECT_FLOAT_EQ(placed.width, 40);
    EXPECT_EQ(placed.glyphs.size(), 4u);
  }

  TEST_F(TextLayoutTest, ALineFeedAtTheEndLeavesAnEmptyLine)
  {
    const auto placed = Place("ab\n");

    EXPECT_EQ(placed.lines, 2u);
    EXPECT_FLOAT_EQ(placed.height, 40);
  }

  TEST_F(TextLayoutTest, DoesNotBreakLinesWithoutALimit)
  {
    EXPECT_EQ(Place("aaaa bbbb cccc dddd eeee").lines, 1u);
  }

  TEST_F(TextLayoutTest, BreaksALineAtTheSpaceInFrontOfTheWordThatDoesNotFit)
  {
    const auto placed = Place("aaaa bbbb cccc", NoWiderThan(100));

    EXPECT_EQ(placed.lines, 2u);
    // the space at the break is part of neither line
    EXPECT_FLOAT_EQ(placed.width, 90);
    EXPECT_FLOAT_EQ(placed.height, 40);

    ASSERT_EQ(placed.glyphs.size(), 12u);
    EXPECT_FLOAT_EQ(placed.glyphs[8].x, 1);
    EXPECT_FLOAT_EQ(placed.glyphs[8].y, 22);
  }

  TEST_F(TextLayoutTest, AWordThatFitsExactlyStaysOnTheLine)
  {
    EXPECT_EQ(Place("aaaa bbbbb", NoWiderThan(100)).lines, 1u);
    EXPECT_EQ(Place("aaaa bbbbbb", NoWiderThan(100)).lines, 2u);
  }

  TEST_F(TextLayoutTest, BreaksAsOftenAsItHasTo)
  {
    const auto placed = Place("aa bb cc dd ee", NoWiderThan(50));

    // `aa bb`, `cc dd`, and `ee`
    EXPECT_EQ(placed.lines, 3u);
    EXPECT_FLOAT_EQ(placed.width, 50);
    EXPECT_FLOAT_EQ(placed.height, 60);
  }

  TEST_F(TextLayoutTest, DoesNotBreakAWordThatIsWiderThanALine)
  {
    const auto placed = Place("aaaaaaaaaaaa bb", NoWiderThan(50));

    EXPECT_EQ(placed.lines, 2u);
    EXPECT_FLOAT_EQ(placed.width, 120);
  }

  TEST_F(TextLayoutTest, KeepsTheLineFeedsOfATextThatIsBroken)
  {
    const auto placed = Place("aa bb\ncc dd ee", NoWiderThan(50));

    EXPECT_EQ(placed.lines, 3u);
  }

  TEST_F(TextLayoutTest, AlignsLinesInTheWidthOfTheLongest)
  {
    TextOptions options;
    options.align = TextAlign::Center;
    auto placed = Place("aaaa\nbb", options);

    ASSERT_EQ(placed.glyphs.size(), 6u);
    EXPECT_FLOAT_EQ(placed.glyphs[0].x, 1);
    EXPECT_FLOAT_EQ(placed.glyphs[4].x, 11);

    options.align = TextAlign::Right;
    placed = Place("aaaa\nbb", options);

    EXPECT_FLOAT_EQ(placed.glyphs[0].x, 1);
    EXPECT_FLOAT_EQ(placed.glyphs[4].x, 21);
  }

  TEST_F(TextLayoutTest, AlignsLinesInTheWidthItIsGiven)
  {
    TextOptions options;
    options.box_width = 100;

    options.align = TextAlign::Left;
    EXPECT_FLOAT_EQ(Place("ab", options).glyphs[0].x, 1);

    options.align = TextAlign::Center;
    EXPECT_FLOAT_EQ(Place("ab", options).glyphs[0].x, 41);

    options.align = TextAlign::Right;
    EXPECT_FLOAT_EQ(Place("ab", options).glyphs[0].x, 81);

    // the text is as wide as it is, whatever it is aligned in
    EXPECT_FLOAT_EQ(Place("ab", options).width, 20);
  }

  TEST_F(TextLayoutTest, TheHeightOfALineCanBeGiven)
  {
    TextOptions options;
    options.line_height = 30;
    const auto placed = Place("ab\ncd", options);

    EXPECT_FLOAT_EQ(placed.height, 60);

    // the 10 a line is higher than the letters are shared above and below
    EXPECT_FLOAT_EQ(placed.glyphs[0].y, 7);
    EXPECT_FLOAT_EQ(placed.glyphs[2].y, 37);
  }

  TEST_F(TextLayoutTest, PlacesCharactersAtWholePixels)
  {
    // the pen moves by 7.5
    Build(15);

    TextOptions options;
    options.align = TextAlign::Center;
    options.box_width = 100;
    const auto placed = Place("abcdefg\nhij", options);

    EXPECT_FLOAT_EQ(placed.width, std::ceil(7 * 7.5f));
    EXPECT_FLOAT_EQ(placed.height, std::round(placed.height));

    for (const auto &glyph : placed.glyphs)
    {
      EXPECT_FLOAT_EQ(glyph.x, std::round(glyph.x));
      EXPECT_FLOAT_EQ(glyph.y, std::round(glyph.y));
    }
  }

  TEST_F(TextLayoutTest, TheHeightOfALineIsAWholeNumber)
  {
    Build(15);

    TextOptions options;
    EXPECT_FLOAT_EQ(neon::LineHeightOf(_atlas, options), 15);

    options.line_height = 17.6f;
    EXPECT_FLOAT_EQ(neon::LineHeightOf(_atlas, options), 18);
  }
}
