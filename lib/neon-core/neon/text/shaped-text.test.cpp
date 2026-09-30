#include "shaped-text.hpp"

#include <cmath>
#include <cstddef>
#include <memory>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include <neon/testing/fake-font-rasterizer.hpp>
#include <neon/testing/fake-text-shaper.hpp>
#include <neon/text/utf8.hpp>

// The font of the tests: at the size 20 every character moves the pen by
// 10, its picture is 8 by 14, starts 1 right of the pen, and stands on the
// baseline, which is 16 below the top of a line of 20.

namespace
{
  using neon::FontRasterizer;
  using neon::GlyphAtlas;
  using neon::GlyphBitmap;
  using neon::GlyphOptions;
  using neon::PlacedShapedText;
  using neon::PlaceShapedText;
  using neon::PlacingOptions;
  using neon::ShapedText;
  using neon::ShapeText;
  using neon::ShapingStyle;
  using neon::TextAlign;
  using neon::TextDirection;
  using neon::TextFont;
  using neon::TextOverflow;
  using neon::TextTransform;
  using neon::WhiteSpace;
  using neon::testing::FakeFontRasterizer;
  using neon::testing::FakeTextShaper;

  /// A font that places its glyphs at parts of a pixel, and whose
  /// characters move the pen by 5.3 at the size 10.
  class FineRasterizer final : public FontRasterizer
  {
  public:
    std::vector<float> offsets;

    int LoadFont(const std::vector<unsigned char> &file) override { return 0; }

    void UnloadFont(int font) override {}

    bool GetMetrics(int font, const float pixel_size, neon::FontMetrics &metrics) override
    {
      metrics = {pixel_size * 0.8f, pixel_size * 0.2f, 0.0f};
      return true;
    }

    bool HasGlyph(int font, const char32_t character) override { return character >= 0x20; }

    bool Rasterize(int font, float pixel_size, char32_t character, GlyphBitmap &glyph) override
    {
      return RasterizeGlyph(font, pixel_size, static_cast<unsigned int>(character), {}, glyph);
    }

    bool RasterizeGlyph(
      int font,
      const float pixel_size,
      const unsigned int glyph,
      const GlyphOptions &options,
      GlyphBitmap &bitmap) override
    {
      offsets.push_back(options.offset_x);

      bitmap = GlyphBitmap{};
      bitmap.advance = pixel_size * 0.53f;
      if (glyph == U' ') { return true; }

      bitmap.width = 4;
      bitmap.height = 7;
      bitmap.left = 0;
      bitmap.top = 7;
      bitmap.coverage.assign(28, 255);
      return true;
    }

    [[nodiscard]] bool PlacesAtPartsOfAPixel() const override { return true; }
  };

  class ShapedTextTest : public ::testing::Test
  {
  protected:
    FakeFontRasterizer _rasterizer;
    FakeFontRasterizer _second_rasterizer;
    FakeTextShaper _shaper;

    std::vector<std::unique_ptr<GlyphAtlas>> _atlases;
    std::vector<TextFont> _fonts;

    void SetUp() override
    {
      AddFont(_rasterizer, 20, true);
    }

    void AddFont(FontRasterizer &rasterizer, const float pixel_size, const bool is_shaped)
    {
      const int font = rasterizer.LoadFont({'f', 'o', 'n', 't'});

      GlyphAtlas::Settings settings;
      settings.rasterizer = &rasterizer;
      settings.font = font;
      settings.pixel_size = pixel_size;
      _atlases.push_back(std::make_unique<GlyphAtlas>(settings));

      TextFont text_font;
      text_font.rasterizer = &rasterizer;
      text_font.font = font;
      text_font.atlas = _atlases.back().get();
      text_font.pixel_size = pixel_size;
      text_font.places_at_parts = rasterizer.PlacesAtPartsOfAPixel();

      if (is_shaped)
      {
        text_font.shaper = &_shaper;
        text_font.shaper_font = _shaper.LoadFont({'f', 'o', 'n', 't'});
      }

      _fonts.push_back(text_font);
    }

    /// The text is drawn with one font that is not shaped.
    void WithoutAShaper()
    {
      _fonts.clear();
      _atlases.clear();
      AddFont(_rasterizer, 20, false);
    }

    [[nodiscard]] ShapedText Shape(const std::u32string &text, const ShapingStyle &style = {}) const
    {
      return ShapeText(text, _fonts, style);
    }

    [[nodiscard]] PlacedShapedText Place(
      const std::u32string &text,
      const PlacingOptions &options = {},
      const ShapingStyle &style = {}) const
    {
      return PlaceShapedText(Shape(text, style), _fonts, options);
    }

    /// The characters the glyphs of a text stand for, from left to right.
    [[nodiscard]] static std::u32string Drawn(const PlacedShapedText &placed)
    {
      std::u32string drawn;
      for (const auto &glyph : placed.glyphs) { drawn += static_cast<char32_t>(glyph.glyph); }
      return drawn;
    }

    [[nodiscard]] static std::vector<float> OriginsOf(const PlacedShapedText &placed)
    {
      std::vector<float> origins;
      for (const auto &glyph : placed.glyphs) { origins.push_back(glyph.origin_x); }
      return origins;
    }

    [[nodiscard]] static PlacingOptions Within(const float width)
    {
      PlacingOptions options;
      options.max_width = width;
      return options;
    }
  };

  // Without a shaper

  TEST_F(ShapedTextTest, PlacesEveryCharacterWhereThePenIs)
  {
    WithoutAShaper();

    const auto placed = Place(U"Hi you");

    EXPECT_FLOAT_EQ(placed.width, 60);
    EXPECT_FLOAT_EQ(placed.height, 20);
    ASSERT_EQ(placed.lines.size(), 1u);

    // the space draws nothing
    ASSERT_EQ(placed.glyphs.size(), 5u);
    EXPECT_EQ(OriginsOf(placed), (std::vector<float>{0, 10, 30, 40, 50}));

    for (const auto &glyph : placed.glyphs)
    {
      EXPECT_FLOAT_EQ(glyph.origin_y, 16) << "on the baseline";
      EXPECT_EQ(glyph.variant, 0);
      ASSERT_NE(glyph.picture, nullptr);
      EXPECT_EQ(glyph.picture->width, 8);
      EXPECT_EQ(glyph.picture->left, 1);
      EXPECT_EQ(glyph.picture->top, 14);
    }
  }

  TEST_F(ShapedTextTest, AnEmptyTextHasNoSize)
  {
    const auto placed = Place(U"");

    EXPECT_FLOAT_EQ(placed.width, 0);
    EXPECT_FLOAT_EQ(placed.height, 0);
    EXPECT_TRUE(placed.glyphs.empty());
    EXPECT_TRUE(placed.lines.empty());
  }

  TEST_F(ShapedTextTest, ATextWithoutAFontHasNoGlyphs)
  {
    _fonts.clear();

    EXPECT_TRUE(Shape(U"abc").glyphs.empty());
    EXPECT_TRUE(Place(U"abc").glyphs.empty());
  }

  TEST_F(ShapedTextTest, NumbersAGlyphAsTheFontDoes)
  {
    WithoutAShaper();

    const auto shaped = Shape(U"ab");

    ASSERT_EQ(shaped.glyphs.size(), 2u);
    EXPECT_EQ(shaped.glyphs[0].glyph, static_cast<unsigned int>(U'a'));
    EXPECT_EQ(shaped.glyphs[0].cluster, 0u);
    EXPECT_EQ(shaped.glyphs[1].cluster, 1u);
    EXPECT_FLOAT_EQ(shaped.glyphs[0].advance, 10);

    ASSERT_EQ(shaped.runs.size(), 1u);
    EXPECT_EQ(shaped.runs[0].direction, TextDirection::LeftToRight);
  }

  TEST_F(ShapedTextTest, TextThatCannotBeShapedIsDrawnCharacterByCharacter)
  {
    _shaper.refuses = true;

    const auto placed = Place(U"fin");

    EXPECT_EQ(Drawn(placed), U"fin");
    EXPECT_FLOAT_EQ(placed.width, 30);
  }

  // Shaping

  TEST_F(ShapedTextTest, MovesPairsTogether)
  {
    const auto kerned = Place(U"AV");
    const auto apart = Place(U"AA");

    EXPECT_FLOAT_EQ(apart.width, 20);
    EXPECT_FLOAT_EQ(kerned.width, 18);
    EXPECT_EQ(OriginsOf(kerned), (std::vector<float>{0, 8}));
  }

  TEST_F(ShapedTextTest, JoinsLettersIntoALigature)
  {
    const auto shaped = Shape(U"fin");

    ASSERT_EQ(shaped.glyphs.size(), 2u);
    EXPECT_EQ(shaped.glyphs[0].glyph, FakeTextShaper::kLigature);
    EXPECT_EQ(shaped.glyphs[0].cluster, 0u);
    EXPECT_EQ(shaped.glyphs[1].cluster, 2u);

    // the ligature moves the pen for both characters it stands for
    ASSERT_EQ(shaped.advances.size(), 3u);
    EXPECT_FLOAT_EQ(shaped.advances[0], 10);
    EXPECT_FLOAT_EQ(shaped.advances[1], 0);
    EXPECT_FLOAT_EQ(shaped.advances[2], 10);

    EXPECT_FLOAT_EQ(Place(U"fin").width, 20);
  }

  TEST_F(ShapedTextTest, ShapesATextOnceForItsParts)
  {
    (void) Shape(U"one part");
    EXPECT_EQ(_shaper.shaped, 1u);
  }

  // The properties of text

  TEST_F(ShapedTextTest, SpacesLettersApart)
  {
    ShapingStyle style;
    style.letter_spacing = 3;

    const auto placed = Place(U"abc", {}, style);

    EXPECT_EQ(OriginsOf(placed), (std::vector<float>{0, 13, 26}));
    EXPECT_FLOAT_EQ(placed.width, 39) << "behind every character, the last one included, as in CSS";
  }

  TEST_F(ShapedTextTest, LettersThatAreSpacedApartAreNotJoined)
  {
    ShapingStyle style;
    style.letter_spacing = 2;

    EXPECT_EQ(Drawn(Place(U"fin", {}, style)), U"fin");
    EXPECT_EQ(Shape(U"fin", style).glyphs.size(), 3u);
  }

  TEST_F(ShapedTextTest, MovesLettersTogetherWithSpacingBelowZero)
  {
    ShapingStyle style;
    style.letter_spacing = -2;

    EXPECT_EQ(OriginsOf(Place(U"abc", {}, style)), (std::vector<float>{0, 8, 16}));
  }

  TEST_F(ShapedTextTest, SpacesWordsApart)
  {
    ShapingStyle style;
    style.word_spacing = 7;

    const auto placed = Place(U"a b c", {}, style);

    EXPECT_EQ(OriginsOf(placed), (std::vector<float>{0, 27, 54}));
    EXPECT_FLOAT_EQ(placed.width, 64);
  }

  TEST_F(ShapedTextTest, RaisesAndLowersLetters)
  {
    ShapingStyle upper;
    upper.transform = TextTransform::Uppercase;
    EXPECT_EQ(Drawn(Place(U"Neon", {}, upper)), U"NEON");

    ShapingStyle lower;
    lower.transform = TextTransform::Lowercase;
    EXPECT_EQ(Drawn(Place(U"Neon", {}, lower)), U"neon");

    ShapingStyle capital;
    capital.transform = TextTransform::Capitalize;
    EXPECT_EQ(Drawn(Place(U"neon engine", {}, capital)), U"NeonEngine");
  }

  TEST_F(ShapedTextTest, ATextThatIsRaisedIsShapedAsWhatIsShown)
  {
    ShapingStyle upper;
    upper.transform = TextTransform::Uppercase;

    // a pair of capitals that is moved together
    EXPECT_FLOAT_EQ(Place(U"av", {}, upper).width, 18);
  }

  // White space

  TEST_F(ShapedTextTest, KeepsSpacesAndLineFeedsAsTheyAreWritten)
  {
    ShapingStyle style;
    style.white_space = WhiteSpace::Pre;

    const auto placed = Place(U"a  b\n c", Within(20), style);

    ASSERT_EQ(placed.lines.size(), 2u) << "and breaks nowhere else";
    EXPECT_EQ(OriginsOf(placed), (std::vector<float>{0, 30, 10}));
    EXPECT_FLOAT_EQ(placed.width, 40);
  }

  TEST_F(ShapedTextTest, JoinsSpacesAndLineFeeds)
  {
    ShapingStyle style;
    style.white_space = WhiteSpace::Normal;

    const auto shaped = Shape(U"  a  \t b\n\nc  ", style);

    EXPECT_EQ(shaped.characters, U"a b c");
  }

  TEST_F(ShapedTextTest, KeepsLineFeedsAndJoinsSpaces)
  {
    ShapingStyle style;
    style.white_space = WhiteSpace::PreLine;

    EXPECT_EQ(Shape(U" a   b \n  c ", style).characters, U"a b\nc");
  }

  TEST_F(ShapedTextTest, DoesNotBreakALineThatIsNotToBeBroken)
  {
    ShapingStyle style;
    style.white_space = WhiteSpace::NoWrap;

    // what measures a text hands over no width to break at then
    const auto placed = Place(U"one two\nthree", {}, style);

    ASSERT_EQ(placed.lines.size(), 1u);
    EXPECT_FLOAT_EQ(placed.width, 130);
  }

  TEST_F(ShapedTextTest, LeavesOutWhatIsNotShown)
  {
    const auto shaped = Shape(U"a\r\nb\u00ADc\td");

    EXPECT_EQ(shaped.characters, U"a\nbc d");
  }

  // Lines

  TEST_F(ShapedTextTest, BreaksALineAtASpace)
  {
    const auto placed = Place(U"one two three", Within(70));

    ASSERT_EQ(placed.lines.size(), 2u);
    EXPECT_FLOAT_EQ(placed.lines[0].width, 70);
    EXPECT_FLOAT_EQ(placed.lines[1].width, 50);
    EXPECT_FLOAT_EQ(placed.width, 70);
    EXPECT_FLOAT_EQ(placed.height, 40);

    EXPECT_FLOAT_EQ(placed.lines[0].baseline, 16);
    EXPECT_FLOAT_EQ(placed.lines[1].baseline, 36);

    EXPECT_EQ(placed.lines[0].glyph_count, 6u);
    EXPECT_EQ(placed.lines[1].first_glyph, 6u);
    EXPECT_FLOAT_EQ(placed.glyphs[6].origin_x, 0);
    EXPECT_FLOAT_EQ(placed.glyphs[6].origin_y, 36);
  }

  TEST_F(ShapedTextTest, DoesNotBreakAWord)
  {
    const auto placed = Place(U"unbreakable", Within(30));

    ASSERT_EQ(placed.lines.size(), 1u);
    EXPECT_FLOAT_EQ(placed.width, 110);
  }

  TEST_F(ShapedTextTest, BreaksALineWhereTheTextSaysSo)
  {
    const auto placed = Place(U"a\n\nb");

    ASSERT_EQ(placed.lines.size(), 3u);
    EXPECT_EQ(placed.lines[1].glyph_count, 0u);
    EXPECT_FLOAT_EQ(placed.height, 60);
  }

  TEST_F(ShapedTextTest, BreaksChineseBetweenAnyTwoCharacters)
  {
    const std::u32string text = U"\u4E2D\u6587\u6587\u5B57\u3002";

    const auto placed = Place(text, Within(30));

    ASSERT_EQ(placed.lines.size(), 2u);
    EXPECT_EQ(placed.lines[0].glyph_count, 3u);

    // what closes a sentence does not start a line
    const auto narrow = Place(text, Within(40));
    ASSERT_EQ(narrow.lines.size(), 2u);
    EXPECT_EQ(narrow.lines[0].glyph_count, 3u);
    EXPECT_EQ(narrow.lines[1].glyph_count, 2u);
  }

  TEST_F(ShapedTextTest, TakesTheHeightOfALineItIsGiven)
  {
    PlacingOptions options;
    options.line_height = 30;

    const auto placed = Place(U"a\nb", options);

    EXPECT_FLOAT_EQ(placed.height, 60);

    // what the line is higher than the letters is shared above and below
    EXPECT_FLOAT_EQ(placed.lines[0].baseline, 21);
    EXPECT_FLOAT_EQ(placed.lines[1].baseline, 51);
  }

  TEST_F(ShapedTextTest, AlignsLinesInTheirBox)
  {
    PlacingOptions options;
    options.box_width = 100;

    options.align = TextAlign::Left;
    EXPECT_FLOAT_EQ(Place(U"ab", options).glyphs[0].origin_x, 0);

    options.align = TextAlign::Center;
    EXPECT_FLOAT_EQ(Place(U"ab", options).glyphs[0].origin_x, 40);

    options.align = TextAlign::Right;
    EXPECT_FLOAT_EQ(Place(U"ab", options).glyphs[0].origin_x, 80);
    EXPECT_FLOAT_EQ(Place(U"ab", options).lines[0].left, 80);

    options.align = TextAlign::Start;
    EXPECT_FLOAT_EQ(Place(U"ab", options).glyphs[0].origin_x, 0);

    options.align = TextAlign::End;
    EXPECT_FLOAT_EQ(Place(U"ab", options).glyphs[0].origin_x, 80);
  }

  // What does not fit

  TEST_F(ShapedTextTest, PutsAnEllipsisWhereALineIsCutOff)
  {
    PlacingOptions options;
    options.box_width = 55;
    options.overflow = TextOverflow::Ellipsis;

    const auto placed = Place(U"abcdefgh", options);

    // four characters and the ellipsis are 50 wide, five are 60
    EXPECT_EQ(Drawn(placed), U"abcd\u2026");
    EXPECT_FLOAT_EQ(placed.glyphs[4].origin_x, 40);
    EXPECT_FLOAT_EQ(placed.lines[0].width, 50);
  }

  TEST_F(ShapedTextTest, PutsNoEllipsisWhereEverythingFits)
  {
    PlacingOptions options;
    options.box_width = 80;
    options.overflow = TextOverflow::Ellipsis;

    EXPECT_EQ(Drawn(Place(U"abcdefgh", options)), U"abcdefgh");
  }

  TEST_F(ShapedTextTest, CutsNothingOffThatIsOnlyClipped)
  {
    PlacingOptions options;
    options.box_width = 55;

    EXPECT_EQ(Drawn(Place(U"abcdefgh", options)), U"abcdefgh");
  }

  // Parts of a pixel

  TEST_F(ShapedTextTest, PlacesGlyphsAtQuartersOfAPixel)
  {
    FineRasterizer fine;
    _fonts.clear();
    _atlases.clear();
    AddFont(fine, 10, false);

    const auto placed = Place(U"aaaaa");

    // the pen is at 0, 5.3, 10.6, 15.9, and 21.2
    ASSERT_EQ(placed.glyphs.size(), 5u);

    EXPECT_FLOAT_EQ(placed.glyphs[0].origin_x, 0);
    EXPECT_EQ(placed.glyphs[0].variant, 0);

    EXPECT_FLOAT_EQ(placed.glyphs[1].origin_x, 5);
    EXPECT_EQ(placed.glyphs[1].variant, 1) << "5.3 is nearest to 5.25";

    EXPECT_FLOAT_EQ(placed.glyphs[2].origin_x, 10);
    EXPECT_EQ(placed.glyphs[2].variant, 2) << "10.6 is nearest to 10.5";

    EXPECT_FLOAT_EQ(placed.glyphs[3].origin_x, 16);
    EXPECT_EQ(placed.glyphs[3].variant, 0) << "15.9 is nearest to 16";

    EXPECT_FLOAT_EQ(placed.glyphs[4].origin_x, 21);
    EXPECT_EQ(placed.glyphs[4].variant, 1);

    // no glyph is further than an eighth of a pixel from where its pen is
    for (std::size_t i = 0; i < placed.glyphs.size(); i++)
    {
      const float pen = 5.3f * static_cast<float>(i);
      const float drawn = placed.glyphs[i].origin_x + static_cast<float>(placed.glyphs[i].variant) / 4.0f;
      EXPECT_LE(std::abs(drawn - pen), 0.125f + 0.001f) << "glyph " << i;
    }

    // and every glyph has the picture of its quarter
    EXPECT_NE(placed.glyphs[0].picture, placed.glyphs[1].picture);
    EXPECT_EQ(placed.glyphs[1].picture, placed.glyphs[4].picture);
  }

  TEST_F(ShapedTextTest, RowsOfGlyphsAreAtWholePixels)
  {
    FineRasterizer fine;
    _fonts.clear();
    _atlases.clear();
    AddFont(fine, 10, false);

    PlacingOptions options;
    options.line_height = 13.4f;

    for (const auto &glyph : Place(U"a\nb\nc", options).glyphs)
    {
      EXPECT_FLOAT_EQ(glyph.origin_y, std::round(glyph.origin_y));
    }
  }

  TEST_F(ShapedTextTest, AFontThatCannotMoveItsGlyphsHasThemAtWholePixels)
  {
    WithoutAShaper();

    _fonts.clear();
    _atlases.clear();
    AddFont(_rasterizer, 15, false);

    // the pen moves by 7.5
    const auto placed = Place(U"aaa");

    EXPECT_EQ(OriginsOf(placed), (std::vector<float>{0, 8, 15}));
    for (const auto &glyph : placed.glyphs) { EXPECT_EQ(glyph.variant, 0); }
  }

  // Several fonts

  TEST_F(ShapedTextTest, TakesTheNextFontForWhatTheFirstDoesNotHave)
  {
    _rasterizer.missing.insert(U'x');
    AddFont(_second_rasterizer, 20, true);

    const auto shaped = Shape(U"axxb");

    ASSERT_EQ(shaped.runs.size(), 3u);
    EXPECT_EQ(shaped.runs[0].font, 0u);
    EXPECT_EQ(shaped.runs[1].font, 1u);
    EXPECT_EQ(shaped.runs[1].first, 1u);
    EXPECT_EQ(shaped.runs[1].end, 3u);
    EXPECT_EQ(shaped.runs[2].font, 0u);

    const auto placed = PlaceShapedText(shaped, _fonts, {});
    ASSERT_EQ(placed.glyphs.size(), 4u);
    EXPECT_EQ(placed.glyphs[0].font, 0);
    EXPECT_EQ(placed.glyphs[1].font, 1);
    EXPECT_EQ(placed.glyphs[2].font, 1);
    EXPECT_EQ(placed.glyphs[3].font, 0);
    EXPECT_EQ(OriginsOf(placed), (std::vector<float>{0, 10, 20, 30}));
  }

  TEST_F(ShapedTextTest, ASpaceDoesNotSplitWhatIsDrawnWithTheSecondFont)
  {
    _rasterizer.missing.insert(U'x');
    _rasterizer.missing.insert(U'y');
    AddFont(_second_rasterizer, 20, true);

    // both fonts have the space, which stays with what is before it
    const auto shaped = Shape(U"x y");

    ASSERT_EQ(shaped.runs.size(), 1u);
    EXPECT_EQ(shaped.runs[0].font, 1u);
  }

  TEST_F(ShapedTextTest, DrawsTheReplacementCharacterForWhatNoFontHas)
  {
    _rasterizer.missing.insert(U'x');

    EXPECT_EQ(Drawn(Place(U"axb")), U"a\uFFFDb");
  }

  TEST_F(ShapedTextTest, DrawsAQuestionMarkWhereThereIsNoReplacementCharacter)
  {
    _rasterizer.missing.insert(U'x');
    _rasterizer.missing.insert(neon::Replacement_Character);

    EXPECT_EQ(Drawn(Place(U"axb")), U"a?b");
  }

  // From right to left

  TEST_F(ShapedTextTest, ATextFromRightToLeftComesOutLastCharacterFirst)
  {
    ShapingStyle style;
    style.direction = TextDirection::RightToLeft;

    const std::u32string word = U"\u05D0\u05D1\u05D2";
    const auto shaped = Shape(word, style);

    ASSERT_EQ(shaped.runs.size(), 1u);
    EXPECT_EQ(shaped.runs[0].direction, TextDirection::RightToLeft);

    const auto placed = PlaceShapedText(shaped, _fonts, {});
    ASSERT_EQ(placed.glyphs.size(), 3u);

    // from the left: gimel, bet, alef
    EXPECT_EQ(placed.glyphs[0].cluster, 2u);
    EXPECT_EQ(placed.glyphs[1].cluster, 1u);
    EXPECT_EQ(placed.glyphs[2].cluster, 0u);
    EXPECT_EQ(OriginsOf(placed), (std::vector<float>{0, 10, 20}));
  }

  TEST_F(ShapedTextTest, AScriptFromRightToLeftRunsItsWayInATextFromLeftToRight)
  {
    // no direction is written, and the word is still in its order
    const auto placed = Place(U"\u05D0\u05D1\u05D2");

    ASSERT_EQ(placed.glyphs.size(), 3u);
    EXPECT_EQ(placed.glyphs[0].cluster, 2u);
    EXPECT_EQ(placed.glyphs[2].cluster, 0u);
  }

  TEST_F(ShapedTextTest, StartsATextFromRightToLeftAtTheRight)
  {
    ShapingStyle style;
    style.direction = TextDirection::RightToLeft;

    PlacingOptions options;
    options.box_width = 100;
    options.align = TextAlign::Start;

    const auto placed = Place(U"\u05D0\u05D1\u05D2", options, style);

    EXPECT_FLOAT_EQ(placed.lines[0].left, 70);
    EXPECT_FLOAT_EQ(placed.glyphs[2].origin_x, 90) << "the first letter is the one furthest right";

    options.align = TextAlign::End;
    EXPECT_FLOAT_EQ(Place(U"\u05D0\u05D1\u05D2", options, style).lines[0].left, 0);
  }

  TEST_F(ShapedTextTest, PartsOfATextFromLeftToRightStayInTheOrderTheyAreWrittenIn)
  {
    // ab, a word of Hebrew, cd
    const std::u32string text = U"ab \u05D0\u05D1\u05D2 cd";
    const auto shaped = Shape(text);

    ASSERT_EQ(shaped.runs.size(), 3u);
    EXPECT_EQ(shaped.runs[0].direction, TextDirection::LeftToRight);
    EXPECT_EQ(shaped.runs[1].direction, TextDirection::RightToLeft);
    EXPECT_EQ(shaped.runs[2].direction, TextDirection::LeftToRight);

    const auto placed = PlaceShapedText(shaped, _fonts, {});

    std::vector<std::uint32_t> clusters;
    for (const auto &glyph : placed.glyphs) { clusters.push_back(glyph.cluster); }

    // a b, then the word from its last letter to its first, then c d
    EXPECT_EQ(clusters, (std::vector<std::uint32_t>{0, 1, 5, 4, 3, 7, 8}));
  }

  TEST_F(ShapedTextTest, PartsOfATextFromRightToLeftAreLaidOutFromTheRight)
  {
    ShapingStyle style;
    style.direction = TextDirection::RightToLeft;

    // a word of Hebrew, then ab: the word is at the right, ab left of it
    const auto placed = Place(U"\u05D0\u05D1\u05D2 ab", {}, style);

    std::vector<std::uint32_t> clusters;
    for (const auto &glyph : placed.glyphs) { clusters.push_back(glyph.cluster); }

    EXPECT_EQ(clusters, (std::vector<std::uint32_t>{4, 5, 2, 1, 0}));
    EXPECT_FLOAT_EQ(placed.glyphs[0].origin_x, 0);
    EXPECT_FLOAT_EQ(placed.glyphs[4].origin_x, 50);
  }

  TEST_F(ShapedTextTest, NumbersRunFromLeftToRightInEveryScript)
  {
    ShapingStyle style;
    style.direction = TextDirection::RightToLeft;

    const auto shaped = Shape(U"\u05D0\u05D1\u05D2 42", style);

    ASSERT_EQ(shaped.runs.size(), 2u);
    EXPECT_EQ(shaped.runs[1].direction, TextDirection::LeftToRight);

    const auto placed = PlaceShapedText(shaped, _fonts, {});
    EXPECT_EQ(Drawn(placed).substr(0, 2), U"42") << "at the left, and not as 24";
  }

  TEST_F(ShapedTextTest, ASpaceBetweenTwoWordsOfOneDirectionRunsTheirWay)
  {
    const auto shaped = Shape(U"\u05D0\u05D1 \u05D0\u05D1");

    ASSERT_EQ(shaped.runs.size(), 1u);
    EXPECT_EQ(shaped.runs[0].direction, TextDirection::RightToLeft);
  }

  TEST_F(ShapedTextTest, TheLettersOfAScriptThatIsJoinedAreNotSpacedApart)
  {
    ShapingStyle style;
    style.direction = TextDirection::RightToLeft;
    style.letter_spacing = 5;

    // Arabic, whose letters are joined, and Hebrew, whose letters are not
    EXPECT_FLOAT_EQ(Place(U"\u0633\u0644\u0645", {}, style).width, 30);
    EXPECT_FLOAT_EQ(Place(U"\u05D0\u05D1\u05D2", {}, style).width, 45);
  }

  TEST_F(ShapedTextTest, PutsTheEllipsisAtTheLeftOfATextFromRightToLeft)
  {
    ShapingStyle style;
    style.direction = TextDirection::RightToLeft;

    PlacingOptions options;
    options.box_width = 35;
    options.overflow = TextOverflow::Ellipsis;

    const auto placed = Place(U"\u05D0\u05D1\u05D2\u05D0\u05D1\u05D2", options, style);

    // two letters and the ellipsis
    ASSERT_EQ(placed.glyphs.size(), 3u);
    EXPECT_EQ(placed.glyphs[0].glyph, 0x2026u);
    EXPECT_EQ(placed.glyphs[1].cluster, 1u);
    EXPECT_EQ(placed.glyphs[2].cluster, 0u) << "the first letter stays, at the right";
  }

  TEST(LineHeightOfTest, IsWhatTheFontAsksForUnlessItIsGiven)
  {
    const neon::FontMetrics metrics{15.2f, 4.1f, 1.0f};

    EXPECT_FLOAT_EQ(neon::LineHeightOf(metrics, 0), 21);
    EXPECT_FLOAT_EQ(neon::LineHeightOf(metrics, 30.4f), 30);
  }
}
