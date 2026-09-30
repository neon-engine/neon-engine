#include "stb-font-rasterizer.hpp"

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include <neon/text/font-atlas.hpp>
#include <neon/text/text-layout.hpp>
#include <neon/text/utf8.hpp>

// Inter Regular, written into this header by the build
#include <test-font.hpp>

namespace
{
  using neon::FontMetrics;
  using neon::GlyphBitmap;
  using neon::STB_FontRasterizer;

  std::vector<unsigned char> Inter()
  {
    return {std::begin(test_font), std::end(test_font)};
  }

  class StbFontRasterizerTest : public ::testing::Test
  {
  protected:
    STB_FontRasterizer _rasterizer;
    int _font = _rasterizer.LoadFont(Inter());

    void SetUp() override
    {
      ASSERT_GE(_font, 0);
    }

    GlyphBitmap Draw(const char32_t character, const float pixel_size = 32)
    {
      GlyphBitmap glyph;
      EXPECT_TRUE(_rasterizer.Rasterize(_font, pixel_size, character, glyph));
      return glyph;
    }
  };

  TEST_F(StbFontRasterizerTest, RefusesBytesThatAreNoFont)
  {
    EXPECT_EQ(_rasterizer.LoadFont({}), -1);
    EXPECT_EQ(_rasterizer.LoadFont({'n', 'o'}), -1);
    EXPECT_EQ(_rasterizer.LoadFont(std::vector<unsigned char>(4096, 'x')), -1);
  }

  TEST_F(StbFontRasterizerTest, GivesEveryFontANumberOfItsOwn)
  {
    const int second = _rasterizer.LoadFont(Inter());

    EXPECT_GE(second, 0);
    EXPECT_NE(second, _font);
  }

  TEST_F(StbFontRasterizerTest, AFontThatWasUnloadedCannotBeUsed)
  {
    _rasterizer.UnloadFont(_font);

    FontMetrics metrics;
    GlyphBitmap glyph;
    EXPECT_FALSE(_rasterizer.GetMetrics(_font, 32, metrics));
    EXPECT_FALSE(_rasterizer.HasGlyph(_font, U'A'));
    EXPECT_FALSE(_rasterizer.Rasterize(_font, 32, U'A', glyph));
  }

  TEST_F(StbFontRasterizerTest, GivesTheNumberOfAnUnloadedFontOutAgain)
  {
    _rasterizer.UnloadFont(_font);

    EXPECT_EQ(_rasterizer.LoadFont(Inter()), _font);
  }

  TEST_F(StbFontRasterizerTest, AFontThatDoesNotExistCannotBeUsed)
  {
    FontMetrics metrics;
    GlyphBitmap glyph;

    for (const int font : {-1, 7})
    {
      EXPECT_FALSE(_rasterizer.GetMetrics(font, 32, metrics));
      EXPECT_FALSE(_rasterizer.HasGlyph(font, U'A'));
      EXPECT_FALSE(_rasterizer.Rasterize(font, 32, U'A', glyph));
      _rasterizer.UnloadFont(font);
    }
  }

  TEST_F(StbFontRasterizerTest, ASizeThatIsNotAboveZeroCannotBeUsed)
  {
    FontMetrics metrics;
    GlyphBitmap glyph;

    EXPECT_FALSE(_rasterizer.GetMetrics(_font, 0, metrics));
    EXPECT_FALSE(_rasterizer.Rasterize(_font, -4, U'A', glyph));
  }

  TEST_F(StbFontRasterizerTest, MeasuresTheFontInPartsOfItsSize)
  {
    FontMetrics metrics;
    ASSERT_TRUE(_rasterizer.GetMetrics(_font, 100, metrics));

    // Inter reaches 96.9 above the baseline and 24.1 below, of 100
    EXPECT_NEAR(metrics.ascent, 96.9f, 0.1f);
    EXPECT_NEAR(metrics.descent, 24.1f, 0.1f);
    EXPECT_NEAR(metrics.line_gap, 0.0f, 0.1f);
  }

  TEST_F(StbFontRasterizerTest, TheMeasuresGrowWithTheSize)
  {
    FontMetrics small;
    FontMetrics large;
    ASSERT_TRUE(_rasterizer.GetMetrics(_font, 16, small));
    ASSERT_TRUE(_rasterizer.GetMetrics(_font, 32, large));

    EXPECT_NEAR(large.ascent, small.ascent * 2, 0.001f);
    EXPECT_NEAR(large.descent, small.descent * 2, 0.001f);
  }

  TEST_F(StbFontRasterizerTest, HasTheCharactersOfLatin1)
  {
    for (char32_t character = 0x21; character <= 0x7E; character++)
    {
      EXPECT_TRUE(_rasterizer.HasGlyph(_font, character)) << static_cast<int>(character);
    }

    for (char32_t character = 0xA1; character <= 0xFF; character++)
    {
      // the soft hyphen says where a word may be broken and is not drawn
      if (character == 0xAD) { continue; }

      EXPECT_TRUE(_rasterizer.HasGlyph(_font, character)) << static_cast<int>(character);
    }

    EXPECT_TRUE(_rasterizer.HasGlyph(_font, 0x20AC)) << "the euro sign";
  }

  TEST_F(StbFontRasterizerTest, DoesNotHaveWhatInterDoesNotDraw)
  {
    EXPECT_FALSE(_rasterizer.HasGlyph(_font, 0x4E2D)) << "a Chinese character";
    EXPECT_FALSE(_rasterizer.HasGlyph(_font, 0x1F600)) << "an emoji";

    GlyphBitmap glyph;
    EXPECT_FALSE(_rasterizer.Rasterize(_font, 32, 0x4E2D, glyph));
  }

  TEST_F(StbFontRasterizerTest, DrawsALetter)
  {
    const GlyphBitmap glyph = Draw(U'H');

    // a capital of Inter is 0.727 of the size high, and stands on the
    // baseline
    EXPECT_NEAR(glyph.height, 23, 1);
    EXPECT_NEAR(glyph.top, 23, 1);
    EXPECT_GT(glyph.width, 10);
    EXPECT_LT(glyph.width, 32);
    EXPECT_GT(glyph.advance, static_cast<float>(glyph.width));
    ASSERT_EQ(glyph.coverage.size(), static_cast<std::size_t>(glyph.width) * glyph.height);

    // the two stems are covered from top to bottom, and between them there
    // is nothing above the bar
    const auto at = [&glyph](const int x, const int y)
    {
      return glyph.coverage[static_cast<std::size_t>(y) * glyph.width + x];
    };

    EXPECT_EQ(at(2, 2), 255);
    EXPECT_EQ(at(glyph.width - 3, glyph.height - 3), 255);
    EXPECT_EQ(at(glyph.width / 2, 2), 0);
  }

  TEST_F(StbFontRasterizerTest, SmoothsTheEdgesOfARoundLetter)
  {
    const GlyphBitmap glyph = Draw(U'o');

    const auto partly = std::ranges::count_if(glyph.coverage, [](const unsigned char value)
    {
      return value > 0 && value < 255;
    });

    EXPECT_GT(partly, 20);
  }

  TEST_F(StbFontRasterizerTest, ALetterWithADescenderReachesBelowTheBaseline)
  {
    const GlyphBitmap glyph = Draw(U'g');

    EXPECT_GT(glyph.height, glyph.top);
  }

  TEST_F(StbFontRasterizerTest, ASpaceMovesThePenAndDrawsNothing)
  {
    const GlyphBitmap glyph = Draw(U' ');

    EXPECT_EQ(glyph.width, 0);
    EXPECT_EQ(glyph.height, 0);
    EXPECT_TRUE(glyph.coverage.empty());
    EXPECT_GT(glyph.advance, 4.0f);
  }

  TEST_F(StbFontRasterizerTest, APictureGrowsWithTheSize)
  {
    const GlyphBitmap small = Draw(U'H', 16);
    const GlyphBitmap large = Draw(U'H', 64);

    EXPECT_NEAR(large.height, small.height * 4, 3);
    EXPECT_NEAR(large.advance, small.advance * 4, 0.01f);
  }

  TEST_F(StbFontRasterizerTest, DrawsAccentedLetters)
  {
    // the accent of an É is above the capital
    EXPECT_GT(Draw(0xC9).top, Draw(U'E').top);
    EXPECT_GT(Draw(0xFC).coverage.size(), 0u);
  }

  TEST_F(StbFontRasterizerTest, FillsAnAtlasAndPlacesAText)
  {
    neon::FontAtlas atlas;
    std::string error;
    ASSERT_TRUE(atlas.Build(_rasterizer, _font, 24, neon::FontAtlas::DefaultCharacters(), error)) << error;

    EXPECT_LE(atlas.GetWidth(), 512);
    EXPECT_TRUE(atlas.Has(0xDF)) << "a sharp s";

    const auto placed = neon::PlaceText(atlas, neon::DecodeUtf8("Health: 75"), {});

    // nine characters draw something, the space does not
    EXPECT_EQ(placed.glyphs.size(), 9u);
    EXPECT_FLOAT_EQ(placed.height, 30);
    EXPECT_GT(placed.width, 100);
    EXPECT_LT(placed.width, 140);
  }

  TEST_F(StbFontRasterizerTest, DrawsAGlyphByItsNumberAndByItsCharacterAlike)
  {
    unsigned int index = 0;
    ASSERT_TRUE(_rasterizer.GetGlyph(_font, U'g', index));
    EXPECT_NE(index, 0u);

    GlyphBitmap by_number;
    ASSERT_TRUE(_rasterizer.RasterizeGlyph(_font, 32, index, {}, by_number));

    const GlyphBitmap by_character = Draw(U'g');
    EXPECT_EQ(by_number.width, by_character.width);
    EXPECT_EQ(by_number.coverage, by_character.coverage);
    EXPECT_EQ(by_number.advance, by_character.advance);

    unsigned int missing = 0;
    EXPECT_FALSE(_rasterizer.GetGlyph(_font, 0x4E2D, missing));
    EXPECT_FALSE(_rasterizer.RasterizeGlyph(_font, 32, 1000000, {}, by_number));
  }

  TEST_F(StbFontRasterizerTest, MovesAGlyphByAPartOfAPixel)
  {
    unsigned int index = 0;
    ASSERT_TRUE(_rasterizer.GetGlyph(_font, U'l', index));

    neon::GlyphOptions options;
    options.offset_x = 0.5f;

    GlyphBitmap at_zero;
    GlyphBitmap moved;
    ASSERT_TRUE(_rasterizer.RasterizeGlyph(_font, 13, index, {}, at_zero));
    ASSERT_TRUE(_rasterizer.RasterizeGlyph(_font, 13, index, options, moved));

    EXPECT_EQ(moved.advance, at_zero.advance);
    EXPECT_NE(moved.coverage, at_zero.coverage);
  }

  TEST_F(StbFontRasterizerTest, RefusesWhatItCannotDraw)
  {
    unsigned int index = 0;
    ASSERT_TRUE(_rasterizer.GetGlyph(_font, U'A', index));

    GlyphBitmap glyph;

    neon::GlyphOptions distances;
    distances.rendering = neon::GlyphRendering::DistanceField;
    EXPECT_FALSE(_rasterizer.RasterizeGlyph(_font, 32, index, distances, glyph));

    neon::GlyphOptions outline;
    outline.stroke = 2.0f;
    EXPECT_FALSE(_rasterizer.RasterizeGlyph(_font, 32, index, outline, glyph));

    neon::GlyphOptions leaning;
    leaning.slant = 0.2f;
    EXPECT_FALSE(_rasterizer.RasterizeGlyph(_font, 32, index, leaning, glyph));
  }
}
