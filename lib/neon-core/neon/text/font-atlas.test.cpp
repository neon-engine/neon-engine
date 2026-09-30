#include "font-atlas.hpp"

#include <cstddef>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include <neon/testing/fake-font-rasterizer.hpp>
#include <neon/text/utf8.hpp>

namespace
{
  using neon::AtlasGlyph;
  using neon::CharacterRange;
  using neon::FontAtlas;
  using neon::testing::FakeFontRasterizer;

  class FontAtlasTest : public ::testing::Test
  {
  protected:
    FakeFontRasterizer _rasterizer;
    int _font = _rasterizer.LoadFont(FakeFontRasterizer::AFont());
    FontAtlas _atlas;
    std::string _error;

    bool Build(const float pixel_size)
    {
      return _atlas.Build(_rasterizer, _font, pixel_size, FontAtlas::DefaultCharacters(), _error);
    }

    [[nodiscard]] unsigned char Alpha(const int x, const int y) const
    {
      return _atlas.GetPixels()[(static_cast<std::size_t>(y) * _atlas.GetWidth() + x) * 4 + 3];
    }
  };

  TEST_F(FontAtlasTest, DrawsLatin1AndWhatIsCommonInText)
  {
    ASSERT_TRUE(Build(20)) << _error;

    for (char32_t character = 0x20; character <= 0x7E; character++) { EXPECT_TRUE(_atlas.Has(character)); }
    for (char32_t character = 0xA0; character <= 0xFF; character++) { EXPECT_TRUE(_atlas.Has(character)); }
    for (char32_t character = 0x100; character <= 0x17F; character++) { EXPECT_TRUE(_atlas.Has(character)); }

    EXPECT_TRUE(_atlas.Has(0x20AC)) << "the euro sign";
    EXPECT_TRUE(_atlas.Has(0x2026)) << "the ellipsis";
    EXPECT_TRUE(_atlas.Has(neon::Replacement_Character));
  }

  TEST_F(FontAtlasTest, LeavesOutWhatIsNotText)
  {
    ASSERT_TRUE(Build(20)) << _error;

    EXPECT_FALSE(_atlas.Has(0x0A)) << "a line feed";
    EXPECT_FALSE(_atlas.Has(0x7F));
    EXPECT_FALSE(_atlas.Has(0x4E2D)) << "a Chinese character";
    EXPECT_FALSE(_atlas.Has(0x1F600)) << "an emoji";
  }

  TEST_F(FontAtlasTest, KeepsHowACharacterIsPlaced)
  {
    ASSERT_TRUE(Build(20)) << _error;

    const AtlasGlyph *glyph = _atlas.Find(U'A');
    ASSERT_NE(glyph, nullptr);

    EXPECT_EQ(glyph->width, 8);
    EXPECT_EQ(glyph->height, 14);
    EXPECT_EQ(glyph->left, 1);
    EXPECT_EQ(glyph->top, 14);
    EXPECT_FLOAT_EQ(glyph->advance, 10.0f);
  }

  TEST_F(FontAtlasTest, ASpaceMovesThePenAndDrawsNothing)
  {
    ASSERT_TRUE(Build(20)) << _error;

    const AtlasGlyph *space = _atlas.Find(U' ');
    ASSERT_NE(space, nullptr);

    EXPECT_EQ(space->width, 0);
    EXPECT_EQ(space->height, 0);
    EXPECT_FLOAT_EQ(space->advance, 10.0f);
  }

  TEST_F(FontAtlasTest, KeepsTheMeasuresOfTheFont)
  {
    ASSERT_TRUE(Build(20)) << _error;

    EXPECT_FLOAT_EQ(_atlas.GetPixelSize(), 20.0f);
    EXPECT_FLOAT_EQ(_atlas.GetMetrics().ascent, 16.0f);
    EXPECT_FLOAT_EQ(_atlas.GetMetrics().descent, 4.0f);
  }

  TEST_F(FontAtlasTest, IsASquareWithSidesThatArePowersOfTwo)
  {
    ASSERT_TRUE(Build(20)) << _error;

    EXPECT_EQ(_atlas.GetWidth(), _atlas.GetHeight());
    EXPECT_EQ(_atlas.GetWidth() & (_atlas.GetWidth() - 1), 0);
    EXPECT_EQ(_atlas.GetPixels().size(), static_cast<std::size_t>(_atlas.GetWidth()) * _atlas.GetHeight() * 4);
  }

  TEST_F(FontAtlasTest, GrowsWithTheSizeOfTheFont)
  {
    ASSERT_TRUE(Build(12)) << _error;
    const int small = _atlas.GetWidth();

    ASSERT_TRUE(Build(96)) << _error;

    EXPECT_GT(_atlas.GetWidth(), small);
  }

  TEST_F(FontAtlasTest, KeepsThePicturesInsideAndApart)
  {
    ASSERT_TRUE(Build(20)) << _error;

    std::vector<AtlasGlyph> glyphs;
    for (const auto &[first, last] : FontAtlas::DefaultCharacters())
    {
      for (char32_t character = first; character <= last; character++)
      {
        if (_atlas.Has(character) && _atlas.Find(character)->width > 0)
        {
          glyphs.push_back(*_atlas.Find(character));
        }
      }
    }

    ASSERT_GT(glyphs.size(), 300u);

    for (std::size_t i = 0; i < glyphs.size(); i++)
    {
      const AtlasGlyph &a = glyphs[i];

      // a pixel is left free at the edge of the image as well
      ASSERT_GE(a.x, 1);
      ASSERT_GE(a.y, 1);
      ASSERT_LE(a.x + a.width, _atlas.GetWidth() - 1);
      ASSERT_LE(a.y + a.height, _atlas.GetHeight() - 1);

      for (std::size_t k = i + 1; k < glyphs.size(); k++)
      {
        const AtlasGlyph &b = glyphs[k];
        const bool apart =
          a.x + a.width + 1 <= b.x || b.x + b.width + 1 <= a.x ||
          a.y + a.height + 1 <= b.y || b.y + b.height + 1 <= a.y;

        ASSERT_TRUE(apart) << "pictures " << i << " and " << k << " are closer than a pixel";
      }
    }
  }

  TEST_F(FontAtlasTest, IsWhiteWithTheCharacterInTheAlphaChannel)
  {
    ASSERT_TRUE(Build(20)) << _error;

    const auto &pixels = _atlas.GetPixels();
    for (std::size_t i = 0; i < pixels.size(); i += 4)
    {
      ASSERT_EQ(pixels[i], 255);
      ASSERT_EQ(pixels[i + 1], 255);
      ASSERT_EQ(pixels[i + 2], 255);
    }

    const AtlasGlyph *glyph = _atlas.Find(U'A');
    ASSERT_NE(glyph, nullptr);

    EXPECT_EQ(Alpha(glyph->x, glyph->y), 255);
    EXPECT_EQ(Alpha(glyph->x + glyph->width - 1, glyph->y + glyph->height - 1), 255);

    // what is around it stays clear
    EXPECT_EQ(Alpha(glyph->x - 1, glyph->y), 0);
    EXPECT_EQ(Alpha(glyph->x, glyph->y - 1), 0);
    EXPECT_EQ(Alpha(glyph->x + glyph->width, glyph->y), 0);
    EXPECT_EQ(Alpha(glyph->x, glyph->y + glyph->height), 0);
  }

  TEST_F(FontAtlasTest, LeavesOutCharactersTheFontDoesNotHave)
  {
    _rasterizer.missing = {U'Q'};
    ASSERT_TRUE(Build(20)) << _error;

    EXPECT_FALSE(_atlas.Has(U'Q'));
    EXPECT_TRUE(_atlas.Has(U'R'));
  }

  TEST_F(FontAtlasTest, FindsTheReplacementCharacterForOneItDoesNotHave)
  {
    ASSERT_TRUE(Build(20)) << _error;

    EXPECT_EQ(_atlas.Find(0x4E2D), _atlas.Find(neon::Replacement_Character));
    EXPECT_NE(_atlas.Find(0x4E2D), nullptr);
  }

  TEST_F(FontAtlasTest, FindsAQuestionMarkWhenTheFontHasNoReplacementCharacter)
  {
    _rasterizer.missing = {neon::Replacement_Character};
    ASSERT_TRUE(Build(20)) << _error;

    EXPECT_EQ(_atlas.Find(0x4E2D), _atlas.Find(U'?'));
    EXPECT_NE(_atlas.Find(0x4E2D), nullptr);
  }

  TEST_F(FontAtlasTest, FindsNothingWhenThereIsNothingToStandIn)
  {
    const std::vector<CharacterRange> letters = {{U'a', U'z'}};
    ASSERT_TRUE(_atlas.Build(_rasterizer, _font, 20, letters, _error)) << _error;

    EXPECT_NE(_atlas.Find(U'a'), nullptr);
    EXPECT_EQ(_atlas.Find(U'A'), nullptr);
  }

  TEST_F(FontAtlasTest, FailsForAFontThatIsNotLoaded)
  {
    EXPECT_FALSE(_atlas.Build(_rasterizer, 7, 20, FontAtlas::DefaultCharacters(), _error));
    EXPECT_EQ(_error, "the font cannot be used");
  }

  TEST_F(FontAtlasTest, FailsForASizeThatIsNotAboveZero)
  {
    EXPECT_FALSE(Build(0));
    EXPECT_EQ(_error, "a font of size 0 cannot be drawn");
  }

  TEST_F(FontAtlasTest, FailsWhenTheFontHasNoneOfTheCharacters)
  {
    const std::vector<CharacterRange> controls = {{0x00, 0x1F}};

    EXPECT_FALSE(_atlas.Build(_rasterizer, _font, 20, controls, _error));
    EXPECT_EQ(_error, "the font has none of the characters that are drawn");
  }

  TEST_F(FontAtlasTest, FailsWhenTheCharactersDoNotFit)
  {
    EXPECT_FALSE(Build(2000));
    EXPECT_EQ(_error, "the characters of a font of size 2000 do not fit into an image of 4096 by 4096");
    EXPECT_TRUE(_atlas.GetPixels().empty());
  }

  TEST_F(FontAtlasTest, CanBeBuiltAgain)
  {
    ASSERT_TRUE(Build(20)) << _error;
    ASSERT_TRUE(Build(40)) << _error;

    EXPECT_EQ(_atlas.Find(U'A')->width, 16);
    EXPECT_FLOAT_EQ(_atlas.GetPixelSize(), 40.0f);
  }
}
