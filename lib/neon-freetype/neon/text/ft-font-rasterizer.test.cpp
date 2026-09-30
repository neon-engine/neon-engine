#include "ft-font-rasterizer.hpp"

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <numeric>
#include <vector>

#include <gtest/gtest.h>

// Inter Regular and Noto Sans Arabic, written into this header by the build
#include <test-fonts.hpp>

namespace
{
  using neon::FontMetrics;
  using neon::FT_FontRasterizer;
  using neon::GlyphBitmap;
  using neon::GlyphOptions;
  using neon::GlyphRendering;

  std::vector<unsigned char> Inter()
  {
    return {std::begin(inter_regular), std::end(inter_regular)};
  }

  /// Where the weight of a picture lies from left to right, in pixels from
  /// its pen.
  float CenterOf(const GlyphBitmap &glyph)
  {
    double sum = 0.0;
    double weighted = 0.0;

    for (int row = 0; row < glyph.height; row++)
    {
      for (int column = 0; column < glyph.width; column++)
      {
        const double value = glyph.coverage[static_cast<std::size_t>(row) * glyph.width + column];
        sum += value;
        weighted += value * (static_cast<double>(glyph.left + column) + 0.5);
      }
    }

    return sum > 0.0 ? static_cast<float>(weighted / sum) : 0.0f;
  }

  long long SumOf(const GlyphBitmap &glyph)
  {
    return std::accumulate(glyph.coverage.begin(), glyph.coverage.end(), 0ll);
  }

  class FtFontRasterizerTest : public ::testing::Test
  {
  protected:
    FT_FontRasterizer _rasterizer;
    int _font = _rasterizer.LoadFont(Inter());

    void SetUp() override
    {
      ASSERT_GE(_font, 0);
    }

    unsigned int GlyphOf(const char32_t character)
    {
      unsigned int glyph = 0;
      EXPECT_TRUE(_rasterizer.GetGlyph(_font, character, glyph));
      return glyph;
    }

    GlyphBitmap Draw(const char32_t character, const float pixel_size = 32, const GlyphOptions &options = {})
    {
      GlyphBitmap glyph;
      EXPECT_TRUE(_rasterizer.RasterizeGlyph(_font, pixel_size, GlyphOf(character), options, glyph));
      return glyph;
    }
  };

  TEST_F(FtFontRasterizerTest, RefusesBytesThatAreNoFont)
  {
    EXPECT_EQ(_rasterizer.LoadFont({}), -1);
    EXPECT_EQ(_rasterizer.LoadFont({'n', 'o'}), -1);
    EXPECT_EQ(_rasterizer.LoadFont(std::vector<unsigned char>(4096, 'x')), -1);
  }

  TEST_F(FtFontRasterizerTest, GivesTheNumberOfAnUnloadedFontOutAgain)
  {
    const int second = _rasterizer.LoadFont(Inter());
    EXPECT_NE(second, _font);

    _rasterizer.UnloadFont(_font);

    FontMetrics metrics;
    GlyphBitmap glyph;
    EXPECT_FALSE(_rasterizer.GetMetrics(_font, 32, metrics));
    EXPECT_FALSE(_rasterizer.HasGlyph(_font, U'A'));
    EXPECT_FALSE(_rasterizer.Rasterize(_font, 32, U'A', glyph));

    EXPECT_EQ(_rasterizer.LoadFont(Inter()), _font);
  }

  TEST_F(FtFontRasterizerTest, MeasuresAFontTheWayItsDesignSays)
  {
    FontMetrics metrics;
    ASSERT_TRUE(_rasterizer.GetMetrics(_font, 100, metrics));

    // Inter: 2728 up and 679.25 down, in a square of 2816
    EXPECT_NEAR(metrics.ascent, 96.875f, 0.01f);
    EXPECT_NEAR(metrics.descent, 24.121f, 0.01f);
    EXPECT_NEAR(metrics.line_gap, 0.0f, 0.01f);

    FontMetrics half;
    ASSERT_TRUE(_rasterizer.GetMetrics(_font, 50, half));
    EXPECT_NEAR(half.ascent * 2.0f, metrics.ascent, 0.01f);
  }

  TEST_F(FtFontRasterizerTest, KnowsTheGlyphsOfAFontByTheirNumber)
  {
    unsigned int a = 0;
    unsigned int b = 0;
    ASSERT_TRUE(_rasterizer.GetGlyph(_font, U'A', a));
    ASSERT_TRUE(_rasterizer.GetGlyph(_font, U'B', b));

    EXPECT_NE(a, 0u);
    EXPECT_NE(a, b);

    unsigned int missing = 7;
    EXPECT_FALSE(_rasterizer.GetGlyph(_font, 0x4E2D, missing)) << "Inter has no Chinese";
    EXPECT_FALSE(_rasterizer.HasGlyph(_font, 0x4E2D));
    EXPECT_TRUE(_rasterizer.HasGlyph(_font, U'A'));
  }

  TEST_F(FtFontRasterizerTest, DrawsAGlyphByItsNumberAndByItsCharacterAlike)
  {
    const GlyphBitmap by_number = Draw(U'g');

    GlyphBitmap by_character;
    ASSERT_TRUE(_rasterizer.Rasterize(_font, 32, U'g', by_character));

    EXPECT_EQ(by_number.width, by_character.width);
    EXPECT_EQ(by_number.height, by_character.height);
    EXPECT_EQ(by_number.coverage, by_character.coverage);
    EXPECT_GT(by_number.width, 0);
    EXPECT_GT(by_number.top, 0);
    EXPECT_LT(by_number.top, by_number.height) << "a g reaches below the baseline";
  }

  TEST_F(FtFontRasterizerTest, RefusesAGlyphTheFontDoesNotHave)
  {
    GlyphBitmap glyph;
    EXPECT_FALSE(_rasterizer.RasterizeGlyph(_font, 32, 1000000, {}, glyph));
    EXPECT_FALSE(_rasterizer.RasterizeGlyph(_font, 0, GlyphOf(U'A'), {}, glyph));
  }

  TEST_F(FtFontRasterizerTest, ASpaceMovesThePenAndDrawsNothing)
  {
    const GlyphBitmap space = Draw(U' ');

    EXPECT_TRUE(space.coverage.empty());
    EXPECT_EQ(space.width, 0);
    EXPECT_GT(space.advance, 0.0f);
  }

  TEST_F(FtFontRasterizerTest, TheAdvanceIsNotRoundedToAPixel)
  {
    // 13 pixels for a square of 2816, which no advance of Inter divides
    const GlyphBitmap small = Draw(U'e', 13);
    const GlyphBitmap large = Draw(U'e', 130);

    EXPECT_NEAR(small.advance * 10.0f, large.advance, 0.01f);
    EXPECT_NE(small.advance, static_cast<float>(static_cast<int>(small.advance)));
  }

  TEST_F(FtFontRasterizerTest, MovesAGlyphByAPartOfAPixel)
  {
    // What is asked for so that text is spaced evenly at small sizes. The
    // stem of an l is a little more than a pixel wide at this size and
    // starts at a whole pixel, so what it covers of its first column says
    // how far it was moved.
    const GlyphBitmap at_zero = Draw(U'l', 13);
    ASSERT_EQ(at_zero.width, 2);
    ASSERT_EQ(at_zero.coverage[static_cast<std::size_t>(at_zero.height / 2) * 2], 255);

    for (const float offset : {0.25f, 0.5f, 0.75f})
    {
      GlyphOptions options;
      options.offset_x = offset;

      const GlyphBitmap moved = Draw(U'l', 13, options);
      ASSERT_EQ(moved.width, 2);
      EXPECT_EQ(moved.left, at_zero.left);
      EXPECT_EQ(moved.advance, at_zero.advance);

      const int row = moved.height / 2;
      EXPECT_NEAR(moved.coverage[static_cast<std::size_t>(row) * 2], 255.0f * (1.0f - offset), 2.0f)
        << "moved by " << offset;
      EXPECT_GT(moved.coverage[static_cast<std::size_t>(row) * 2 + 1], at_zero.coverage[static_cast<std::size_t>(row) * 2 + 1])
        << "moved by " << offset;

      EXPECT_GT(CenterOf(moved), CenterOf(at_zero));
    }
  }

  TEST_F(FtFontRasterizerTest, AGlyphThatIsMovedCoversAsMuchAsBefore)
  {
    GlyphOptions options;
    options.offset_x = 0.5f;

    const long long before = SumOf(Draw(U'o', 13));
    const long long after = SumOf(Draw(U'o', 13, options));

    EXPECT_NEAR(static_cast<double>(after), static_cast<double>(before), static_cast<double>(before) * 0.03);
  }

  TEST_F(FtFontRasterizerTest, FitsTheRowsOfAGlyphToThePixels)
  {
    // with light hinting the foot of a letter lies on a row of pixels, so
    // the lowest row of an l is covered nearly in full where the stem is
    const GlyphBitmap glyph = Draw(U'l', 13);
    ASSERT_GT(glyph.height, 0);

    unsigned char most = 0;
    for (int column = 0; column < glyph.width; column++)
    {
      most = std::max(most, glyph.coverage[static_cast<std::size_t>(glyph.height - 1) * glyph.width + column]);
    }

    EXPECT_GT(most, 200);
  }

  TEST_F(FtFontRasterizerTest, MakesAGlyphBolderAndLeaning)
  {
    const GlyphBitmap plain = Draw(U'l', 32);

    GlyphOptions bold;
    bold.embolden = 2.0f;
    EXPECT_GT(SumOf(Draw(U'l', 32, bold)), SumOf(plain) * 3 / 2);

    GlyphOptions leaning;
    leaning.slant = 0.25f;
    const GlyphBitmap slanted = Draw(U'l', 32, leaning);
    EXPECT_GT(slanted.width, plain.width + 3) << "the top of the stem moved to the right";
    EXPECT_EQ(slanted.advance, plain.advance);
  }

  TEST_F(FtFontRasterizerTest, DrawsTheLineAroundAGlyph)
  {
    GlyphOptions options;
    options.stroke = 2.0f;

    const GlyphBitmap plain = Draw(U'O', 48);
    const GlyphBitmap line = Draw(U'O', 48, options);

    ASSERT_FALSE(line.coverage.empty());
    EXPECT_EQ(line.width, plain.width + 2) << "a pixel more on both sides";

    // the middle of the stroke of the O is covered, since the line lies on
    // the outline and not over what is inside it
    const int row = line.height / 2;
    const int on_the_outline = 1;
    const int inside_the_stroke = (plain.left - line.left) + 3;
    EXPECT_GT(line.coverage[static_cast<std::size_t>(row) * line.width + on_the_outline], 200);
    EXPECT_LT(line.coverage[static_cast<std::size_t>(row) * line.width + inside_the_stroke], 60);
  }

  TEST_F(FtFontRasterizerTest, DrawsAGlyphAsDistances)
  {
    GlyphOptions options;
    options.rendering = GlyphRendering::DistanceField;

    const GlyphBitmap plain = Draw(U'I', 48);
    const GlyphBitmap field = Draw(U'I', 48, options);

    ASSERT_TRUE(field.is_distance_field);
    EXPECT_EQ(field.distance_range, static_cast<float>(FT_FontRasterizer::kDistance_Range));

    // room for the distances on every side. The glyph itself is a pixel
    // smaller or larger, since it is not fitted to the pixels
    EXPECT_NEAR(field.width, plain.width + 2 * FT_FontRasterizer::kDistance_Range, 1);
    EXPECT_NEAR(field.height, plain.height + 2 * FT_FontRasterizer::kDistance_Range, 1);
    EXPECT_EQ(field.left, plain.left - FT_FontRasterizer::kDistance_Range);

    const auto at = [&field](const int column, const int row)
    {
      return field.coverage[static_cast<std::size_t>(row) * field.width + column];
    };

    // far outside, on the outline, and in the middle of the stem
    EXPECT_LT(at(0, field.height / 2), 20);
    EXPECT_NEAR(at(FT_FontRasterizer::kDistance_Range, field.height / 2), 128, 24);
    EXPECT_GT(at(field.width / 2, field.height / 2), 150);
  }

  TEST_F(FtFontRasterizerTest, SaysWhereTheLineUnderATextGoes)
  {
    float position = 0.0f;
    float thickness = 0.0f;
    ASSERT_TRUE(_rasterizer.GetUnderline(_font, 100, position, thickness));

    EXPECT_GT(position, 0.0f) << "below the baseline";
    EXPECT_LT(position, 30.0f);
    EXPECT_GT(thickness, 1.0f);
    EXPECT_LT(thickness, 15.0f);
  }

  TEST_F(FtFontRasterizerTest, DrawsTheGlyphsOfAnotherScript)
  {
    const int arabic = _rasterizer.LoadFont({std::begin(noto_sans_arabic), std::end(noto_sans_arabic)});
    ASSERT_GE(arabic, 0);

    EXPECT_TRUE(_rasterizer.HasGlyph(arabic, 0x0633)); // seen
    EXPECT_FALSE(_rasterizer.HasGlyph(_font, 0x0633));

    GlyphBitmap glyph;
    ASSERT_TRUE(_rasterizer.Rasterize(arabic, 32, 0x0633, glyph));
    EXPECT_GT(glyph.width, 8);
  }
} // namespace
