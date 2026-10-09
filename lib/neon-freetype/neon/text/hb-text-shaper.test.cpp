#include "hb-text-shaper.hpp"

#include <cstddef>
#include <iterator>
#include <set>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "ft-font-rasterizer.hpp"

// Inter Regular and Noto Sans Arabic, written into this header by the build
#include <test-fonts.hpp>

namespace
{
  using neon::CharacterDirection;
  using neon::FT_FontRasterizer;
  using neon::HB_TextShaper;
  using neon::ShapedGlyph;
  using neon::ShapingOptions;
  using neon::TextDirection;

  class HbTextShaperTest : public ::testing::Test
  {
  protected:
    HB_TextShaper _shaper;
    FT_FontRasterizer _rasterizer;

    int _inter = _shaper.LoadFont({std::begin(inter_regular), std::end(inter_regular)});
    int _arabic = _shaper.LoadFont({std::begin(noto_sans_arabic), std::end(noto_sans_arabic)});

    // the same fonts as the rasterizer numbers their glyphs
    int _drawn_inter = _rasterizer.LoadFont({std::begin(inter_regular), std::end(inter_regular)});
    int _drawn_arabic = _rasterizer.LoadFont({std::begin(noto_sans_arabic), std::end(noto_sans_arabic)});

    void SetUp() override
    {
      ASSERT_GE(_inter, 0);
      ASSERT_GE(_arabic, 0);
    }

    std::vector<ShapedGlyph> Shape(
      const int font,
      const std::u32string &text,
      const ShapingOptions &options = {},
      const float pixel_size = 100)
    {
      std::vector<ShapedGlyph> glyphs;
      EXPECT_TRUE(_shaper.Shape(font, pixel_size, text, 0, text.size(), options, glyphs));
      return glyphs;
    }

    static float WidthOf(const std::vector<ShapedGlyph> &glyphs)
    {
      float width = 0.0f;
      for (const auto &glyph : glyphs) { width += glyph.advance; }
      return width;
    }

    unsigned int GlyphOf(const int font, const char32_t character)
    {
      unsigned int glyph = 0;
      EXPECT_TRUE(_rasterizer.GetGlyph(font, character, glyph));
      return glyph;
    }
  };

  TEST_F(HbTextShaperTest, RefusesBytesThatAreNoFont)
  {
    EXPECT_EQ(_shaper.LoadFont({}), -1);
    EXPECT_EQ(_shaper.LoadFont({'n', 'o'}), -1);
    EXPECT_EQ(_shaper.LoadFont(std::vector<unsigned char>(4096, 'x')), -1);
  }

  TEST_F(HbTextShaperTest, AFontThatWasUnloadedShapesNothing)
  {
    _shaper.UnloadFont(_inter);

    std::vector<ShapedGlyph> glyphs;
    EXPECT_FALSE(_shaper.Shape(_inter, 100, U"A", 0, 1, {}, glyphs));
    EXPECT_TRUE(glyphs.empty());
  }

  TEST_F(HbTextShaperTest, HandsOverAGlyphForEveryLetterWithTheNumbersOfTheFont)
  {
    const auto glyphs = Shape(_inter, U"Neon");

    ASSERT_EQ(glyphs.size(), 4u);
    EXPECT_EQ(glyphs[0].glyph, GlyphOf(_drawn_inter, U'N'));
    EXPECT_EQ(glyphs[1].glyph, GlyphOf(_drawn_inter, U'e'));
    EXPECT_EQ(glyphs[3].glyph, GlyphOf(_drawn_inter, U'n'));

    for (std::size_t i = 0; i < glyphs.size(); i++)
    {
      EXPECT_EQ(glyphs[i].cluster, i);
      EXPECT_GT(glyphs[i].advance, 0.0f);
    }
  }

  TEST_F(HbTextShaperTest, MovesPairsOfLettersTogether)
  {
    ShapingOptions apart;
    apart.kerning = false;

    // an A fits under the arm of a V and of a T
    for (const std::u32string pair : {U"AV", U"VA", U"To", U"AT"})
    {
      const float kerned = WidthOf(Shape(_inter, pair));
      const float unkerned = WidthOf(Shape(_inter, pair, apart));

      EXPECT_LT(kerned, unkerned - 1.0f) << "pair " << pair.size();
    }

    // and nothing fits under an H
    EXPECT_NEAR(WidthOf(Shape(_inter, U"HH")), WidthOf(Shape(_inter, U"HH", apart)), 0.01f);
  }

  TEST_F(HbTextShaperTest, TheWidthOfATextIsNotRoundedToPixels)
  {
    const float small = WidthOf(Shape(_inter, U"waffle", {}, 13));
    const float large = WidthOf(Shape(_inter, U"waffle", {}, 130));

    EXPECT_NEAR(small * 10.0f, large, 0.2f);
  }

  TEST_F(HbTextShaperTest, JoinsLettersIntoALigature)
  {
    // Inter draws an arrow for a hyphen and a greater-than sign
    const auto joined = Shape(_inter, U"a->b");

    ShapingOptions apart;
    apart.ligatures = false;
    const auto separate = Shape(_inter, U"a->b", apart);

    ASSERT_EQ(separate.size(), 4u);
    ASSERT_EQ(joined.size(), 3u) << "one glyph for two characters";

    EXPECT_EQ(joined[1].cluster, 1u);
    EXPECT_EQ(joined[2].cluster, 3u) << "the ligature stands for the characters 1 and 2";
    EXPECT_NE(joined[1].glyph, separate[1].glyph);
    EXPECT_NE(joined[1].glyph, separate[2].glyph);
  }

  TEST_F(HbTextShaperTest, ShapesAPartOfATextAndKeepsItsPlaces)
  {
    std::vector<ShapedGlyph> glyphs;
    ASSERT_TRUE(_shaper.Shape(_inter, 100, U"Neon Engine", 5, 6, {}, glyphs));

    ASSERT_EQ(glyphs.size(), 6u);
    EXPECT_EQ(glyphs[0].glyph, GlyphOf(_drawn_inter, U'E'));
    EXPECT_EQ(glyphs[0].cluster, 5u);
    EXPECT_EQ(glyphs[5].cluster, 10u);

    EXPECT_FALSE(_shaper.Shape(_inter, 100, U"Neon", 2, 3, {}, glyphs)) << "past the end";
    EXPECT_TRUE(_shaper.Shape(_inter, 100, U"Neon", 4, 0, {}, glyphs));
    EXPECT_TRUE(glyphs.empty());
  }

  TEST_F(HbTextShaperTest, AnArabicWordComesOutFromRightToLeft)
  {
    // salam: seen, lam, alef, meem
    const std::u32string word = U"\u0633\u0644\u0627\u0645";

    ShapingOptions options;
    options.direction = TextDirection::RightToLeft;
    const auto glyphs = Shape(_arabic, word, options);

    ASSERT_EQ(glyphs.size(), 4u);

    // what is drawn first, at the left, is the last letter of the word
    EXPECT_EQ(glyphs[0].cluster, 3u) << "meem";
    EXPECT_EQ(glyphs[1].cluster, 2u) << "alef";
    EXPECT_EQ(glyphs[2].cluster, 1u) << "lam";
    EXPECT_EQ(glyphs[3].cluster, 0u) << "seen";

    // seen, lam, and alef are joined, and so are drawn in other forms than
    // the font maps the letters to. An alef joins nothing behind it, which
    // leaves the meem as it is
    EXPECT_NE(glyphs[3].glyph, GlyphOf(_drawn_arabic, 0x0633));
    EXPECT_NE(glyphs[2].glyph, GlyphOf(_drawn_arabic, 0x0644));
    EXPECT_NE(glyphs[1].glyph, GlyphOf(_drawn_arabic, 0x0627));
    EXPECT_EQ(glyphs[0].glyph, GlyphOf(_drawn_arabic, 0x0645));
  }

  TEST_F(HbTextShaperTest, TheSameWordFromLeftToRightIsNotJoinedTheSameWay)
  {
    // what a text looks like whose direction nobody has told the shaper
    const std::u32string word = U"\u0633\u0644\u0627\u0645";

    ShapingOptions options;
    options.direction = TextDirection::RightToLeft;

    const auto right = Shape(_arabic, word, options);
    const auto wrong = Shape(_arabic, word);

    ASSERT_EQ(wrong.size(), right.size());
    EXPECT_EQ(wrong[0].cluster, 0u);
    EXPECT_NE(wrong[0].cluster, right[0].cluster);
  }

  TEST_F(HbTextShaperTest, ArabicLettersTakeTheFormTheirNeighborsAskFor)
  {
    ShapingOptions options;
    options.direction = TextDirection::RightToLeft;

    // seen alone, and seen three times in a row: at the start, in the
    // middle, and at the end of a word
    const auto alone = Shape(_arabic, U"\u0633", options);
    const auto word = Shape(_arabic, U"\u0633\u0633\u0633", options);

    ASSERT_EQ(alone.size(), 1u);
    ASSERT_EQ(word.size(), 3u);

    const unsigned int as_written = GlyphOf(_drawn_arabic, 0x0633);
    EXPECT_EQ(alone[0].glyph, as_written) << "a letter on its own is drawn as the font maps it";

    const std::set<unsigned int> forms = {word[0].glyph, word[1].glyph, word[2].glyph};
    EXPECT_EQ(forms.size(), 3u) << "three forms of one letter";
    EXPECT_FALSE(forms.contains(as_written)) << "none of them is the form that stands alone";

    // joined letters are narrower than the letter alone, three times
    EXPECT_LT(WidthOf(word), WidthOf(alone) * 3.0f);
  }

  TEST_F(HbTextShaperTest, PlacesTheDotsOfALetterWhereTheFontSays)
  {
    ShapingOptions options;
    options.direction = TextDirection::RightToLeft;

    // this font draws a beh as its body and a dot below it
    const auto glyphs = Shape(_arabic, U"\u0628", options);

    ASSERT_EQ(glyphs.size(), 2u);
    EXPECT_EQ(glyphs[0].cluster, glyphs[1].cluster);

    EXPECT_EQ(glyphs[0].advance, 0.0f) << "a dot does not move the pen";
    EXPECT_GT(glyphs[0].offset_x, 0.0f);
    EXPECT_GT(glyphs[1].advance, 0.0f);
  }

  TEST_F(HbTextShaperTest, AGlyphTheFontDoesNotHaveIsGlyphZero)
  {
    // what is asked of the next font of a text
    const auto glyphs = Shape(_inter, U"a\u0633");

    ASSERT_EQ(glyphs.size(), 2u);
    EXPECT_NE(glyphs[0].glyph, 0u);
    EXPECT_EQ(glyphs[1].glyph, 0u);
  }

  TEST_F(HbTextShaperTest, KnowsTheWayAScriptRuns)
  {
    EXPECT_EQ(_shaper.GetDirection(U'a'), CharacterDirection::LeftToRight);
    EXPECT_EQ(_shaper.GetDirection(0x0633), CharacterDirection::RightToLeft); // Arabic
    EXPECT_EQ(_shaper.GetDirection(0x05D0), CharacterDirection::RightToLeft); // Hebrew
    EXPECT_EQ(_shaper.GetDirection(0x4E2D), CharacterDirection::LeftToRight); // Chinese
    EXPECT_EQ(_shaper.GetDirection(U' '), CharacterDirection::Neutral);
    EXPECT_EQ(_shaper.GetDirection(U'.'), CharacterDirection::Neutral);
    EXPECT_EQ(_shaper.GetDirection(U'7'), CharacterDirection::Neutral);
  }

  TEST_F(HbTextShaperTest, TellsScriptsApart)
  {
    EXPECT_EQ(_shaper.GetScript(U'a'), _shaper.GetScript(U'Z'));
    EXPECT_NE(_shaper.GetScript(U'a'), _shaper.GetScript(0x0633));
    EXPECT_NE(_shaper.GetScript(U'a'), _shaper.GetScript(0x0434)); // Cyrillic
    EXPECT_NE(_shaper.GetScript(U'a'), 0u);

    EXPECT_EQ(_shaper.GetScript(U' '), 0u);
    EXPECT_EQ(_shaper.GetScript(0x0301), 0u) << "an accent that combines belongs to its letter";
  }
} // namespace
