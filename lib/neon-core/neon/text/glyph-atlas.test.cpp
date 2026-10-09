#include "glyph-atlas.hpp"

#include <cstddef>
#include <set>
#include <vector>

#include <gtest/gtest.h>

#include <neon/testing/fake-font-rasterizer.hpp>

namespace
{
  using neon::CachedGlyph;
  using neon::FontRasterizer;
  using neon::GlyphAtlas;
  using neon::GlyphBitmap;
  using neon::GlyphEffect;
  using neon::GlyphOptions;
  using neon::GlyphPage;
  using neon::GlyphRendering;
  using neon::testing::FakeFontRasterizer;

  /// A rasterizer that keeps what it was asked for, and draws a glyph as a
  /// box that says so: moved glyphs are a pixel wider for each quarter.
  class RecordingRasterizer final : public FontRasterizer
  {
  public:
    std::vector<GlyphOptions> asked;
    std::set<unsigned int> missing;
    int width = 6;
    int height = 10;
    bool draws_colors = false;

    int LoadFont(const std::vector<unsigned char> &file) override { return 0; }

    void UnloadFont(int font) override {}

    bool GetMetrics(int font, const float pixel_size, neon::FontMetrics &metrics) override
    {
      if (font != 0) { return false; }
      metrics = {pixel_size * 0.8f, pixel_size * 0.2f, 0.0f};
      return true;
    }

    bool HasGlyph(int font, char32_t character) override { return true; }

    bool Rasterize(int font, float pixel_size, char32_t character, GlyphBitmap &glyph) override { return false; }

    bool RasterizeGlyph(
      int font,
      const float pixel_size,
      const unsigned int glyph,
      const GlyphOptions &options,
      GlyphBitmap &bitmap) override
    {
      if (missing.contains(glyph)) { return false; }

      asked.push_back(options);

      bitmap = GlyphBitmap{};
      bitmap.advance = pixel_size * 0.5f;

      // glyph 32 is a space
      if (glyph == 32) { return true; }

      bitmap.width = width + static_cast<int>(options.offset_x * 4.0f);
      bitmap.height = height;
      bitmap.left = 1;
      bitmap.top = height;
      bitmap.coverage.assign(static_cast<std::size_t>(bitmap.width) * bitmap.height, 200);

      if (draws_colors)
      {
        bitmap.colors.assign(bitmap.coverage.size() * 4, 0);
        for (std::size_t i = 0; i < bitmap.coverage.size(); i++)
        {
          bitmap.colors[i * 4 + 0] = 10;
          bitmap.colors[i * 4 + 1] = 20;
          bitmap.colors[i * 4 + 2] = 30;
          bitmap.colors[i * 4 + 3] = 255;
        }
      }

      if (options.rendering == GlyphRendering::DistanceField)
      {
        bitmap.is_distance_field = true;
        bitmap.distance_range = 8.0f;
      }

      return true;
    }
  };

  class GlyphAtlasTest : public ::testing::Test
  {
  protected:
    RecordingRasterizer _rasterizer;

    GlyphAtlas::Settings Settings(const float pixel_size = 16, const int page_size = 64)
    {
      GlyphAtlas::Settings settings;
      settings.rasterizer = &_rasterizer;
      settings.font = 0;
      settings.pixel_size = pixel_size;
      settings.page_size = page_size;
      return settings;
    }

    static unsigned char Alpha(const GlyphPage &page, const int x, const int y)
    {
      return page.pixels[(static_cast<std::size_t>(y) * page.width + x) * 4 + 3];
    }
  };

  TEST_F(GlyphAtlasTest, StartsWithoutAGlyphAndWithoutAPage)
  {
    const GlyphAtlas atlas(Settings());

    EXPECT_TRUE(atlas.IsUsable());
    EXPECT_EQ(atlas.GetPageCount(), 0u);
    EXPECT_EQ(atlas.GetGlyphCount(), 0u);
    EXPECT_TRUE(_rasterizer.asked.empty());
  }

  TEST_F(GlyphAtlasTest, CannotBeUsedWithoutAFontOrASize)
  {
    auto without_font = Settings();
    without_font.font = 7;
    GlyphAtlas no_font(without_font);
    EXPECT_FALSE(no_font.IsUsable());
    EXPECT_EQ(no_font.Find(65), nullptr);

    GlyphAtlas no_size(Settings(0));
    EXPECT_FALSE(no_size.IsUsable());

    auto without_rasterizer = Settings();
    without_rasterizer.rasterizer = nullptr;
    EXPECT_FALSE(GlyphAtlas(without_rasterizer).IsUsable());
  }

  TEST_F(GlyphAtlasTest, DrawsAGlyphWhenItIsFirstAskedFor)
  {
    GlyphAtlas atlas(Settings());

    const CachedGlyph *glyph = atlas.Find(65);

    ASSERT_NE(glyph, nullptr);
    EXPECT_EQ(glyph->width, 6);
    EXPECT_EQ(glyph->height, 10);
    EXPECT_EQ(glyph->left, 1);
    EXPECT_EQ(glyph->top, 10);
    EXPECT_FLOAT_EQ(glyph->advance, 8.0f);

    ASSERT_EQ(atlas.GetPageCount(), 1u);
    EXPECT_TRUE(atlas.GetPage(0).is_dirty);
    EXPECT_EQ(_rasterizer.asked.size(), 1u);
  }

  TEST_F(GlyphAtlasTest, DrawsAGlyphOnce)
  {
    GlyphAtlas atlas(Settings());

    const CachedGlyph *first = atlas.Find(65);
    const CachedGlyph *second = atlas.Find(65);

    EXPECT_EQ(first, second);
    EXPECT_EQ(_rasterizer.asked.size(), 1u);
    EXPECT_EQ(atlas.GetGlyphCount(), 1u);
  }

  TEST_F(GlyphAtlasTest, PutsTheGlyphIntoThePageWithRoomAroundIt)
  {
    GlyphAtlas atlas(Settings());

    const CachedGlyph *a = atlas.Find(65);
    const CachedGlyph *b = atlas.Find(66);
    ASSERT_NE(a, nullptr);
    ASSERT_NE(b, nullptr);

    const GlyphPage &page = atlas.GetPage(0);
    ASSERT_EQ(page.pixels.size(), 64u * 64u * 4u);

    EXPECT_EQ(Alpha(page, a->x, a->y), 200);
    EXPECT_EQ(Alpha(page, a->x + a->width - 1, a->y + a->height - 1), 200);

    // a pixel is kept free on every side, so that one glyph does not show
    // at the edge of another
    EXPECT_GE(a->x, 1);
    EXPECT_GE(a->y, 1);
    EXPECT_EQ(Alpha(page, a->x - 1, a->y), 0);
    EXPECT_EQ(Alpha(page, a->x + a->width, a->y), 0);
    EXPECT_EQ(Alpha(page, a->x, a->y + a->height), 0);
    EXPECT_GE(b->x, a->x + a->width + 1);

    // white where there is nothing, so that the color next to a glyph is
    // that of the glyph
    EXPECT_EQ(page.pixels[0], 255);
    EXPECT_EQ(page.pixels[3], 0);
  }

  TEST_F(GlyphAtlasTest, GrowsByPagesWhenOneIsFull)
  {
    GlyphAtlas atlas(Settings());

    // a page of 64 by 64 takes 8 glyphs of 6 by 10 in a row, and 4 rows
    std::vector<const CachedGlyph *> glyphs;
    for (unsigned int glyph = 100; glyph < 200; glyph++) { glyphs.push_back(atlas.Find(glyph)); }

    EXPECT_GT(atlas.GetPageCount(), 1u);
    EXPECT_EQ(atlas.GetGlyphCount(), 100u);

    std::set<int> pages;
    for (const CachedGlyph *glyph : glyphs)
    {
      ASSERT_NE(glyph, nullptr);
      pages.insert(glyph->page);

      EXPECT_GE(glyph->x, 1);
      EXPECT_GE(glyph->y, 1);
      EXPECT_LE(glyph->x + glyph->width + 1, 64);
      EXPECT_LE(glyph->y + glyph->height + 1, 64);
    }

    EXPECT_EQ(pages.size(), atlas.GetPageCount());
  }

  TEST_F(GlyphAtlasTest, WhereAGlyphLiesStaysTrueWhileTheAtlasGrows)
  {
    GlyphAtlas atlas(Settings());

    const CachedGlyph *first = atlas.Find(65);
    const CachedGlyph copy = *first;

    for (unsigned int glyph = 100; glyph < 400; glyph++) { (void) atlas.Find(glyph); }

    // what a text has placed is still where it was told
    EXPECT_EQ(atlas.Find(65), first);
    EXPECT_EQ(first->page, copy.page);
    EXPECT_EQ(first->x, copy.x);
    EXPECT_EQ(first->y, copy.y);
    EXPECT_EQ(atlas.GetPage(0).width, 64);
  }

  TEST_F(GlyphAtlasTest, NoTwoGlyphsShareAPixel)
  {
    GlyphAtlas atlas(Settings());

    std::vector<CachedGlyph> glyphs;
    for (unsigned int glyph = 100; glyph < 160; glyph++)
    {
      _rasterizer.width = 3 + static_cast<int>(glyph % 7);
      _rasterizer.height = 5 + static_cast<int>(glyph % 11);
      glyphs.push_back(*atlas.Find(glyph));
    }

    for (std::size_t a = 0; a < glyphs.size(); a++)
    {
      for (std::size_t b = a + 1; b < glyphs.size(); b++)
      {
        if (glyphs[a].page != glyphs[b].page) { continue; }

        const bool apart =
          glyphs[a].x + glyphs[a].width < glyphs[b].x || glyphs[b].x + glyphs[b].width < glyphs[a].x ||
          glyphs[a].y + glyphs[a].height < glyphs[b].y || glyphs[b].y + glyphs[b].height < glyphs[a].y;

        EXPECT_TRUE(apart) << "glyphs " << a << " and " << b;
      }
    }
  }

  TEST_F(GlyphAtlasTest, KeepsAPictureForEveryQuarterOfAPixel)
  {
    GlyphAtlas atlas(Settings());

    std::set<const CachedGlyph *> pictures;
    for (int variant = 0; variant < GlyphAtlas::kVariants; variant++)
    {
      const CachedGlyph *glyph = atlas.Find(65, variant);
      ASSERT_NE(glyph, nullptr);
      pictures.insert(glyph);

      EXPECT_FLOAT_EQ(_rasterizer.asked.back().offset_x, static_cast<float>(variant) / 4.0f);
      EXPECT_EQ(glyph->width, 6 + variant);
    }

    EXPECT_EQ(pictures.size(), 4u);
    EXPECT_EQ(_rasterizer.asked.size(), 4u);

    (void) atlas.Find(65, 2);
    EXPECT_EQ(_rasterizer.asked.size(), 4u) << "and each is drawn once";

    // what is asked for beyond that is the nearest there is
    EXPECT_EQ(atlas.Find(65, 9), atlas.Find(65, 3));
    EXPECT_EQ(atlas.Find(65, -1), atlas.Find(65, 0));
  }

  TEST_F(GlyphAtlasTest, AGlyphThatDrawsNothingTakesNoRoom)
  {
    GlyphAtlas atlas(Settings());

    const CachedGlyph *space = atlas.Find(32);

    ASSERT_NE(space, nullptr);
    EXPECT_EQ(space->width, 0);
    EXPECT_EQ(space->height, 0);
    EXPECT_FLOAT_EQ(space->advance, 8.0f);
    EXPECT_EQ(atlas.GetPageCount(), 0u);
  }

  TEST_F(GlyphAtlasTest, TriesAGlyphThatCannotBeDrawnOnce)
  {
    _rasterizer.missing.insert(65);
    GlyphAtlas atlas(Settings());

    EXPECT_EQ(atlas.Find(65), nullptr);
    EXPECT_EQ(atlas.Find(65), nullptr);
    EXPECT_EQ(atlas.Find(65), nullptr);

    EXPECT_EQ(atlas.GetFailedCount(), 1u);
    EXPECT_EQ(atlas.GetGlyphCount(), 0u);
    EXPECT_NE(atlas.Find(66), nullptr);
  }

  TEST_F(GlyphAtlasTest, RefusesAGlyphThatIsLargerThanAPage)
  {
    _rasterizer.width = 80;
    GlyphAtlas atlas(Settings());

    EXPECT_EQ(atlas.Find(65), nullptr);
    EXPECT_EQ(atlas.GetFailedCount(), 1u);
    EXPECT_EQ(atlas.GetPageCount(), 0u);
  }

  TEST_F(GlyphAtlasTest, APageIsCleanOnceItWasHandedOver)
  {
    GlyphAtlas atlas(Settings());

    (void) atlas.Find(65);
    atlas.MarkClean(0);
    EXPECT_FALSE(atlas.GetPage(0).is_dirty);

    (void) atlas.Find(65);
    EXPECT_FALSE(atlas.GetPage(0).is_dirty) << "a glyph that is there changes nothing";

    (void) atlas.Find(66);
    EXPECT_TRUE(atlas.GetPage(0).is_dirty);

    atlas.MarkClean(7);
  }

  TEST_F(GlyphAtlasTest, ChoosesAPageThatSuitsTheSize)
  {
    GlyphAtlas small(Settings(16, 0));
    GlyphAtlas large(Settings(48, 0));
    GlyphAtlas huge(Settings(120, 0));

    (void) small.Find(65);
    (void) large.Find(65);
    (void) huge.Find(65);

    EXPECT_EQ(small.GetPage(0).width, 512);
    EXPECT_EQ(large.GetPage(0).width, 1024);
    EXPECT_EQ(huge.GetPage(0).width, 2048);
    EXPECT_EQ(huge.GetPage(0).height, GlyphAtlas::kMax_Page_Size);
  }

  TEST_F(GlyphAtlasTest, HandsTheWayAFontIsDrawnToTheRasterizer)
  {
    auto settings = Settings();
    settings.rendering = GlyphRendering::DistanceField;
    settings.embolden = 1.5f;
    settings.slant = 0.2f;

    GlyphAtlas atlas(settings);
    EXPECT_TRUE(atlas.IsDistanceField());
    EXPECT_FLOAT_EQ(atlas.GetDistanceRange(), 0.0f) << "not known before a glyph was drawn";

    ASSERT_NE(atlas.Find(65), nullptr);

    ASSERT_EQ(_rasterizer.asked.size(), 1u);
    EXPECT_EQ(_rasterizer.asked[0].rendering, GlyphRendering::DistanceField);
    EXPECT_FLOAT_EQ(_rasterizer.asked[0].embolden, 1.5f);
    EXPECT_FLOAT_EQ(_rasterizer.asked[0].slant, 0.2f);
    EXPECT_FLOAT_EQ(atlas.GetDistanceRange(), 8.0f);
  }

  TEST_F(GlyphAtlasTest, KeepsTheLineAroundAGlyphAsAPictureOfItsOwn)
  {
    GlyphAtlas atlas(Settings());

    const CachedGlyph *plain = atlas.Find(65);
    const CachedGlyph *line = atlas.Find(65, 0, {GlyphEffect::Kind::Stroke, 2.0f});
    const CachedGlyph *wider = atlas.Find(65, 0, {GlyphEffect::Kind::Stroke, 3.0f});

    EXPECT_NE(line, plain);
    EXPECT_NE(line, wider);
    EXPECT_FLOAT_EQ(_rasterizer.asked[1].stroke, 2.0f);
    EXPECT_FLOAT_EQ(_rasterizer.asked[2].stroke, 3.0f);

    // widths that are all but the same share a picture
    EXPECT_EQ(atlas.Find(65, 0, {GlyphEffect::Kind::Stroke, 2.04f}), line);

    // and an effect of nothing is the glyph itself
    EXPECT_EQ(atlas.Find(65, 0, {GlyphEffect::Kind::Stroke, 0.0f}), plain);
    EXPECT_EQ(atlas.Find(65, 0, {GlyphEffect::Kind::Blur, 0.0f}), plain);
  }

  TEST_F(GlyphAtlasTest, PutsAGlyphOutOfFocusForItsShadow)
  {
    GlyphAtlas atlas(Settings());

    const CachedGlyph *plain = atlas.Find(65);
    const CachedGlyph *shadow = atlas.Find(65, 0, {GlyphEffect::Kind::Blur, 4.0f});

    ASSERT_NE(shadow, nullptr);
    EXPECT_NE(shadow, plain);

    // larger on every side by the same, so that nothing of the blur is
    // cut off, and placed so that its middle is that of the glyph
    const int margin = (shadow->width - plain->width) / 2;
    EXPECT_GT(margin, 2);
    EXPECT_EQ(shadow->width, plain->width + 2 * margin);
    EXPECT_EQ(shadow->height, plain->height + 2 * margin);
    EXPECT_EQ(shadow->left, plain->left - margin);
    EXPECT_EQ(shadow->top, plain->top + margin);
    EXPECT_FLOAT_EQ(shadow->advance, plain->advance);

    const GlyphPage &page = atlas.GetPage(shadow->page);
    const int middle = Alpha(page, shadow->x + shadow->width / 2, shadow->y + shadow->height / 2);
    const int edge = Alpha(page, shadow->x, shadow->y + shadow->height / 2);
    const int between = Alpha(page, shadow->x + margin, shadow->y + shadow->height / 2);

    EXPECT_GT(middle, between);
    EXPECT_GT(between, edge);
    EXPECT_LT(edge, 8);
    EXPECT_LT(middle, 200) << "a box of 6 by 10 is thinned out by a blur of 4";
  }

  TEST_F(GlyphAtlasTest, KeepsTheColorsOfAGlyphThatHasItsOwn)
  {
    _rasterizer.draws_colors = true;
    GlyphAtlas atlas(Settings());

    const CachedGlyph *glyph = atlas.Find(65);
    ASSERT_NE(glyph, nullptr);
    EXPECT_TRUE(glyph->has_colors);

    const GlyphPage &page = atlas.GetPage(0);
    const std::size_t at = (static_cast<std::size_t>(glyph->y) * page.width + glyph->x) * 4;
    EXPECT_EQ(page.pixels[at + 0], 10);
    EXPECT_EQ(page.pixels[at + 1], 20);
    EXPECT_EQ(page.pixels[at + 2], 30);
    EXPECT_EQ(page.pixels[at + 3], 255);

    // its shadow has the color it is given
    const CachedGlyph *shadow = atlas.Find(65, 0, {GlyphEffect::Kind::Blur, 2.0f});
    ASSERT_NE(shadow, nullptr);
    EXPECT_FALSE(shadow->has_colors);
  }

  TEST(BlurCoverageTest, SpreadsWhatIsCoveredAndKeepsHowMuchThereIs)
  {
    int width = 9;
    int height = 9;
    std::vector<unsigned char> coverage(static_cast<std::size_t>(width) * height, 0);
    coverage[4 * 9 + 4] = 255;

    const int margin = neon::BlurCoverage(coverage, width, height, 6.0f);

    EXPECT_GT(margin, 0);
    EXPECT_EQ(width, 9 + 2 * margin);
    EXPECT_EQ(height, 9 + 2 * margin);
    ASSERT_EQ(coverage.size(), static_cast<std::size_t>(width) * height);

    const auto at = [&](const int x, const int y)
    {
      return coverage[static_cast<std::size_t>(y) * width + x];
    };

    const int middle = margin + 4;

    // the same to every side
    EXPECT_EQ(at(middle - 2, middle), at(middle + 2, middle));
    EXPECT_EQ(at(middle, middle - 2), at(middle, middle + 2));
    EXPECT_GE(at(middle, middle), at(middle + 1, middle));
    EXPECT_GE(at(middle + 1, middle), at(middle + 3, middle));

    // nothing is left at the edge
    EXPECT_EQ(at(0, middle), 0);
    EXPECT_EQ(at(width - 1, middle), 0);
  }

  TEST(BlurCoverageTest, LeavesAPictureAloneThatIsNotBlurred)
  {
    int width = 2;
    int height = 2;
    std::vector<unsigned char> coverage = {1, 2, 3, 4};

    EXPECT_EQ(neon::BlurCoverage(coverage, width, height, 0.0f), 0);
    EXPECT_EQ(coverage, (std::vector<unsigned char>{1, 2, 3, 4}));
    EXPECT_EQ(width, 2);
  }

  TEST(BlurCoverageTest, AnAreaThatIsCoveredStaysCoveredInItsMiddle)
  {
    int width = 40;
    int height = 40;
    std::vector<unsigned char> coverage(static_cast<std::size_t>(width) * height, 255);

    const int margin = neon::BlurCoverage(coverage, width, height, 4.0f);

    EXPECT_EQ(coverage[static_cast<std::size_t>(margin + 20) * width + margin + 20], 255);

    // and half covered on its edge, where as much lies inside as outside
    EXPECT_NEAR(coverage[static_cast<std::size_t>(margin + 20) * width + margin], 140, 20);
  }
}
