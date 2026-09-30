#include <cmath>
#include <cstddef>
#include <iterator>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include <neon/testing/memory-file-system.hpp>
#include <neon/testing/mock-render-2d-context.hpp>
#include <neon/testing/recording-logger.hpp>
#include <neon/text/ft-font-rasterizer.hpp>
#include <neon/text/glyph-atlas.hpp>
#include <neon/text/hb-text-shaper.hpp>
#include <neon/text/shaped-text.hpp>
#include <neon/text/stb-font-rasterizer.hpp>
#include <neon/ui/ui-resources.hpp>

// Inter Regular and Noto Sans Arabic, written into this header by the build
#include <test-fonts.hpp>

namespace
{
  using neon::FT_FontRasterizer;
  using neon::GlyphAtlas;
  using neon::HB_TextShaper;
  using neon::PlacedShapedText;
  using neon::PlaceShapedText;
  using neon::PlacingOptions;
  using neon::ShapedText;
  using neon::ShapeText;
  using neon::ShapingStyle;
  using neon::TextDirection;
  using neon::TextFont;
  using neon::testing::LogLevel;

  std::vector<unsigned char> Inter()
  {
    return {std::begin(inter_regular), std::end(inter_regular)};
  }

  std::vector<unsigned char> NotoSansArabic()
  {
    return {std::begin(noto_sans_arabic), std::end(noto_sans_arabic)};
  }

  class TextShapingTest : public ::testing::Test
  {
  protected:
    FT_FontRasterizer _rasterizer;
    HB_TextShaper _shaper;

    std::vector<std::unique_ptr<GlyphAtlas>> _atlases;
    std::vector<TextFont> _fonts;

    void Add(const std::vector<unsigned char> &file, const float pixel_size)
    {
      TextFont font;
      font.rasterizer = &_rasterizer;
      font.font = _rasterizer.LoadFont(file);
      font.shaper = &_shaper;
      font.shaper_font = _shaper.LoadFont(file);
      font.pixel_size = pixel_size;
      font.places_at_parts = true;

      ASSERT_GE(font.font, 0);
      ASSERT_GE(font.shaper_font, 0);

      GlyphAtlas::Settings settings;
      settings.rasterizer = &_rasterizer;
      settings.font = font.font;
      settings.pixel_size = pixel_size;
      _atlases.push_back(std::make_unique<GlyphAtlas>(settings));

      font.atlas = _atlases.back().get();
      _fonts.push_back(font);
    }

    /// Inter, and Noto Sans Arabic for what Inter does not have.
    void UseBothFonts(const float pixel_size)
    {
      _fonts.clear();
      _atlases.clear();
      Add(Inter(), pixel_size);
      Add(NotoSansArabic(), pixel_size);
    }

    [[nodiscard]] PlacedShapedText Place(const std::u32string &text, const ShapingStyle &style = {}) const
    {
      return PlaceShapedText(ShapeText(text, _fonts, style), _fonts, {});
    }

    [[nodiscard]] unsigned int GlyphOf(const std::size_t font, const char32_t character)
    {
      unsigned int glyph = 0;
      EXPECT_TRUE(_rasterizer.GetGlyph(_fonts[font].font, character, glyph));
      return glyph;
    }

    /// Where the glyph is drawn, as exactly as its picture says.
    [[nodiscard]] static float DrawnAt(const neon::PlacedShapedGlyph &glyph)
    {
      return glyph.origin_x + static_cast<float>(glyph.variant) / static_cast<float>(GlyphAtlas::kVariants);
    }
  };

  TEST_F(TextShapingTest, MovesAPairOfLettersTogether)
  {
    UseBothFonts(100);

    const auto kerned = Place(U"AV");
    const auto first = Place(U"A");
    const auto second = Place(U"V");

    ASSERT_EQ(kerned.glyphs.size(), 2u);

    // the V starts left of where the A ends
    const float unkerned_width = ShapeText(U"A", _fonts, {}).advances[0] + ShapeText(U"V", _fonts, {}).advances[0];
    const ShapedText shaped = ShapeText(U"AV", _fonts, {});

    EXPECT_LT(shaped.advances[0] + shaped.advances[1], unkerned_width - 2.0f);
    EXPECT_LT(kerned.width, first.width + second.width - 1.0f);
  }

  TEST_F(TextShapingTest, DrawsALigatureAsOneGlyph)
  {
    UseBothFonts(32);

    // Inter draws an arrow for a hyphen and a greater-than sign
    const auto placed = Place(U"a->b");

    ASSERT_EQ(placed.glyphs.size(), 3u);
    EXPECT_EQ(placed.glyphs[0].glyph, GlyphOf(0, U'a'));
    EXPECT_NE(placed.glyphs[1].glyph, GlyphOf(0, U'-'));
    EXPECT_NE(placed.glyphs[1].glyph, GlyphOf(0, U'>'));
    EXPECT_EQ(placed.glyphs[1].cluster, 1u);
    EXPECT_EQ(placed.glyphs[2].cluster, 3u);

    // a glyph no character stands for is drawn all the same, since the
    // atlas knows glyphs by their number
    ASSERT_NE(placed.glyphs[1].picture, nullptr);
    EXPECT_GT(placed.glyphs[1].picture->width, 10);
  }

  TEST_F(TextShapingTest, LettersThatAreSpacedApartAreNotJoined)
  {
    UseBothFonts(32);

    ShapingStyle style;
    style.letter_spacing = 2;

    const auto placed = Place(U"a->b", style);

    ASSERT_EQ(placed.glyphs.size(), 4u);
    EXPECT_EQ(placed.glyphs[1].glyph, GlyphOf(0, U'-'));
    EXPECT_EQ(placed.glyphs[2].glyph, GlyphOf(0, U'>'));
  }

  TEST_F(TextShapingTest, AnArabicWordIsInItsOrderAndJoined)
  {
    UseBothFonts(48);

    // salam: seen, lam, alef, meem. No direction is written: the script
    // says which way the word runs.
    const std::u32string word = U"\u0633\u0644\u0627\u0645";
    const ShapedText shaped = ShapeText(word, _fonts, {});

    ASSERT_EQ(shaped.runs.size(), 1u);
    EXPECT_EQ(shaped.runs[0].font, 1u) << "Inter has no Arabic";
    EXPECT_EQ(shaped.runs[0].direction, TextDirection::RightToLeft);

    const auto placed = PlaceShapedText(shaped, _fonts, {});
    ASSERT_EQ(placed.glyphs.size(), 4u);

    // from the left: meem, alef, lam, seen
    EXPECT_EQ(placed.glyphs[0].cluster, 3u);
    EXPECT_EQ(placed.glyphs[1].cluster, 2u);
    EXPECT_EQ(placed.glyphs[2].cluster, 1u);
    EXPECT_EQ(placed.glyphs[3].cluster, 0u);

    for (std::size_t i = 1; i < placed.glyphs.size(); i++)
    {
      EXPECT_GT(DrawnAt(placed.glyphs[i]), DrawnAt(placed.glyphs[i - 1])) << "glyph " << i;
    }

    // the seen at the start of the word is drawn in the form that joins
    // what follows, and not as the font maps the letter
    EXPECT_NE(placed.glyphs[3].glyph, GlyphOf(1, 0x0633));
    EXPECT_NE(placed.glyphs[2].glyph, GlyphOf(1, 0x0644));

    // what is joined leaves no gap: the strokes that join two letters lie
    // over each other by a little
    const float seen_starts = DrawnAt(placed.glyphs[3]) + static_cast<float>(placed.glyphs[3].picture->left);
    const float lam_ends = DrawnAt(placed.glyphs[2]) + static_cast<float>(placed.glyphs[2].picture->left) +
                           static_cast<float>(placed.glyphs[2].picture->width);
    EXPECT_LE(seen_starts, lam_ends + 0.5f);
    EXPECT_GT(seen_starts, lam_ends - 6.0f);
  }

  TEST_F(TextShapingTest, EveryFormOfAnArabicLetterIsDrawnOnDemand)
  {
    UseBothFonts(24);

    const std::size_t before = _atlases[1]->GetGlyphCount();
    EXPECT_EQ(before, 0u) << "nothing is drawn before it is asked for";

    const auto placed = Place(U"\u0633\u0633\u0633");
    ASSERT_EQ(placed.glyphs.size(), 3u);

    std::set<unsigned int> forms;
    for (const auto &glyph : placed.glyphs)
    {
      forms.insert(glyph.glyph);
      ASSERT_NE(glyph.picture, nullptr);
      EXPECT_GT(glyph.picture->width, 0);
    }

    EXPECT_EQ(forms.size(), 3u) << "at the start, in the middle, and at the end of a word";
    EXPECT_GE(_atlases[1]->GetGlyphCount(), 3u);
    EXPECT_GE(_atlases[1]->GetPageCount(), 1u);
  }

  TEST_F(TextShapingTest, TakesTheNextFontForWhatTheFirstDoesNotHave)
  {
    UseBothFonts(24);

    const std::u32string text = U"Hi \u0633\u0644\u0627\u0645 ok";
    const ShapedText shaped = ShapeText(text, _fonts, {});

    ASSERT_EQ(shaped.runs.size(), 3u);
    EXPECT_EQ(shaped.runs[0].font, 0u);
    EXPECT_EQ(shaped.runs[1].font, 1u);
    EXPECT_EQ(shaped.runs[2].font, 0u);

    EXPECT_EQ(shaped.runs[1].first, 3u);
    EXPECT_EQ(shaped.runs[1].end, 7u);
    EXPECT_EQ(shaped.runs[1].direction, TextDirection::RightToLeft);

    const auto placed = PlaceShapedText(shaped, _fonts, {});

    // H, i, the four letters from the last to the first, o, k
    std::vector<std::uint32_t> clusters;
    for (const auto &glyph : placed.glyphs) { clusters.push_back(glyph.cluster); }
    EXPECT_EQ(clusters, (std::vector<std::uint32_t>{0, 1, 6, 5, 4, 3, 8, 9}));

    for (const auto &glyph : placed.glyphs)
    {
      EXPECT_NE(glyph.glyph, 0u) << "no glyph is the one that stands for what a font does not have";
    }
  }

  TEST_F(TextShapingTest, WithoutASecondFontWhatIsMissingIsReplaced)
  {
    _fonts.clear();
    _atlases.clear();
    Add(Inter(), 24);

    const auto placed = Place(U"a\u0633b");

    // Inter has no replacement character, which leaves the question mark
    ASSERT_EQ(placed.glyphs.size(), 3u);
    EXPECT_FALSE(_rasterizer.HasGlyph(_fonts[0].font, 0xFFFD));
    EXPECT_EQ(placed.glyphs[1].glyph, GlyphOf(0, U'?'));
  }

  TEST_F(TextShapingTest, PlacesSmallTextAtQuartersOfAPixel)
  {
    UseBothFonts(12);

    const std::u32string text = U"Illiterate waffle";
    const ShapedText shaped = ShapeText(text, _fonts, {});
    const auto placed = PlaceShapedText(shaped, _fonts, {});

    // where the pen stands in front of every glyph, as the font means it
    float pen = 0.0f;
    std::vector<float> pens;
    for (const auto &glyph : shaped.glyphs)
    {
      pens.push_back(pen + glyph.offset_x);
      pen += glyph.advance;
    }

    std::set<int> variants;
    std::size_t at = 0;
    float furthest = 0.0f;

    for (std::size_t i = 0; i < shaped.glyphs.size(); i++)
    {
      // the space draws nothing, and is not among what was placed
      if (text[shaped.glyphs[i].cluster] == U' ') { continue; }

      ASSERT_LT(at, placed.glyphs.size());
      const auto &glyph = placed.glyphs[at++];

      EXPECT_FLOAT_EQ(glyph.origin_x, std::round(glyph.origin_x));
      EXPECT_FLOAT_EQ(glyph.origin_y, std::round(glyph.origin_y)) << "rows are at whole pixels";

      furthest = std::max(furthest, std::abs(DrawnAt(glyph) - pens[i]));
      variants.insert(glyph.variant);
    }

    // at whole pixels a glyph is up to half a pixel from where it belongs
    EXPECT_LE(furthest, 0.125f + 0.001f);
    EXPECT_GE(variants.size(), 3u) << "and the text uses the quarters it has";
  }

  TEST_F(TextShapingTest, AGlyphThatIsMovedIsDrawnMoved)
  {
    UseBothFonts(13);

    const unsigned int glyph = GlyphOf(0, U'l');

    const neon::CachedGlyph *at_zero = _atlases[0]->Find(glyph, 0);
    const neon::CachedGlyph *at_half = _atlases[0]->Find(glyph, 2);

    ASSERT_NE(at_zero, nullptr);
    ASSERT_NE(at_half, nullptr);
    EXPECT_NE(at_zero, at_half);

    const neon::GlyphPage &page = _atlases[0]->GetPage(at_zero->page);
    const auto alpha = [&page](const neon::CachedGlyph &cached, const int column)
    {
      const int row = cached.y + cached.height / 2;
      return page.pixels[(static_cast<std::size_t>(row) * page.width + cached.x + column) * 4 + 3];
    };

    // the stem of the l covers its first column in full, and half of it
    // when it is moved by half a pixel
    EXPECT_EQ(alpha(*at_zero, 0), 255);
    EXPECT_NEAR(alpha(*at_half, 0), 128, 3);
  }

  TEST_F(TextShapingTest, TheAtlasGrowsByPagesAsGlyphsAreAskedFor)
  {
    _fonts.clear();
    _atlases.clear();

    TextFont font;
    font.rasterizer = &_rasterizer;
    font.font = _rasterizer.LoadFont(NotoSansArabic());

    GlyphAtlas::Settings settings;
    settings.rasterizer = &_rasterizer;
    settings.font = font.font;
    settings.pixel_size = 40;
    settings.page_size = 256;
    GlyphAtlas atlas(settings);

    // every glyph of the font by its number, which includes the forms that
    // no character stands for
    std::size_t drawn = 0;
    for (unsigned int glyph = 1; glyph <= 300; glyph++)
    {
      if (atlas.Find(glyph) != nullptr) { drawn++; }
    }

    EXPECT_GT(drawn, 250u);
    EXPECT_GT(atlas.GetPageCount(), 3u);

    for (std::size_t page = 0; page < atlas.GetPageCount(); page++)
    {
      EXPECT_EQ(atlas.GetPage(page).width, 256);
      EXPECT_EQ(atlas.GetPage(page).height, 256);
    }
  }

  TEST_F(TextShapingTest, MeasuresTheSameWithEitherRasterizer)
  {
    // so that taking one for the other moves nothing in a layout
    neon::STB_FontRasterizer stb;
    const int stb_font = stb.LoadFont(Inter());
    const int ft_font = _rasterizer.LoadFont(Inter());

    neon::FontMetrics from_stb;
    neon::FontMetrics from_freetype;
    ASSERT_TRUE(stb.GetMetrics(stb_font, 24, from_stb));
    ASSERT_TRUE(_rasterizer.GetMetrics(ft_font, 24, from_freetype));

    EXPECT_NEAR(from_stb.ascent, from_freetype.ascent, 0.01f);
    EXPECT_NEAR(from_stb.descent, from_freetype.descent, 0.01f);
    EXPECT_NEAR(from_stb.line_gap, from_freetype.line_gap, 0.01f);

    unsigned int stb_glyph = 0;
    unsigned int ft_glyph = 0;
    ASSERT_TRUE(stb.GetGlyph(stb_font, U'g', stb_glyph));
    ASSERT_TRUE(_rasterizer.GetGlyph(ft_font, U'g', ft_glyph));
    EXPECT_EQ(stb_glyph, ft_glyph) << "both number the glyphs as the font does";

    neon::GlyphBitmap by_stb;
    neon::GlyphBitmap by_freetype;
    ASSERT_TRUE(stb.RasterizeGlyph(stb_font, 24, stb_glyph, {}, by_stb));
    ASSERT_TRUE(_rasterizer.RasterizeGlyph(ft_font, 24, ft_glyph, {}, by_freetype));
    EXPECT_NEAR(by_stb.advance, by_freetype.advance, 0.01f);
  }

  // The fonts of a user interface

  class TextResourcesTest : public ::testing::Test
  {
  protected:
    std::shared_ptr<neon::testing::RecordingLogger> _logger = std::make_shared<neon::testing::RecordingLogger>();
    neon::testing::MemoryFileSystem _file_system{SettingsConfig{}, _logger};
    neon::testing::RecordingRenderer2D _renderer;
    FT_FontRasterizer _rasterizer;
    HB_TextShaper _shaper;
    neon::UiResources _resources{&_renderer, &_rasterizer, &_file_system, _logger};

    void SetUp() override
    {
      _file_system.Initialize();

      const auto inter = Inter();
      const auto arabic = NotoSansArabic();
      _file_system.AddNativeFile("/assets/fonts/inter.ttf", std::string(inter.begin(), inter.end()));
      _file_system.AddNativeFile("/assets/fonts/arabic.ttf", std::string(arabic.begin(), arabic.end()));

      _resources.SetTextShaper(&_shaper);
      _resources.AddFace("sans-serif", 400, "assets://fonts/inter.ttf");
      _resources.AddFace("arabic", 400, "assets://fonts/arabic.ttf");
    }

    void TearDown() override
    {
      _resources.CleanUp();
    }
  };

  TEST_F(TextResourcesTest, HandsOverTheFontsOfAListOfFamilies)
  {
    const neon::UiTextFonts *fonts = _resources.GetTextFonts("sans-serif, 'arabic'", 400, false, 16);

    ASSERT_NE(fonts, nullptr) << _logger->Messages(LogLevel::Error);
    ASSERT_EQ(fonts->fonts.size(), 2u);
    ASSERT_EQ(fonts->glyphs.size(), 2u);

    EXPECT_NE(fonts->fonts[0].font, fonts->fonts[1].font);
    EXPECT_NE(fonts->fonts[0].shaper, nullptr);
    EXPECT_TRUE(fonts->fonts[0].places_at_parts);
    EXPECT_FLOAT_EQ(fonts->fonts[0].scale, 1.0f);
    EXPECT_EQ(_logger->Count(LogLevel::Error), 0u);
  }

  TEST_F(TextResourcesTest, LeavesOutAFamilyThatIsNotKnownAndSaysSoOnce)
  {
    for (int i = 0; i < 3; i++)
    {
      const neon::UiTextFonts *fonts = _resources.GetTextFonts("missing, sans-serif", 400, false, 16 + i);
      ASSERT_NE(fonts, nullptr);
      EXPECT_EQ(fonts->fonts.size(), 1u);
    }

    EXPECT_EQ(_logger->Count(LogLevel::Error), 1u) << _logger->Messages(LogLevel::Error);
    EXPECT_TRUE(_logger->Contains(LogLevel::Error, "The font family 'missing' is not known"));
  }

  TEST_F(TextResourcesTest, DrawsTheCharactersOfMostTextAheadAndTheRestOnDemand)
  {
    const neon::UiTextFonts *fonts = _resources.GetTextFonts("sans-serif, arabic", 400, false, 16);
    ASSERT_NE(fonts, nullptr);

    const std::size_t ahead = fonts->fonts[0].atlas->GetGlyphCount();
    EXPECT_GT(ahead, 300u);

    // Cyrillic, which is not among what is drawn ahead
    const ShapedText shaped = ShapeText(U"\u0416\u0434\u044F", fonts->fonts, {});
    const auto placed = PlaceShapedText(shaped, fonts->fonts, {});

    ASSERT_EQ(placed.glyphs.size(), 3u);
    EXPECT_EQ(fonts->fonts[0].atlas->GetGlyphCount(), ahead + 3);
    EXPECT_TRUE(fonts->fonts[0].atlas->GetPage(0).is_dirty);
  }

  TEST_F(TextResourcesTest, HandsNewGlyphsToTheRendererIntoTheTextureThatIsThere)
  {
    const neon::UiTextFonts *fonts = _resources.GetTextFonts("sans-serif", 400, false, 16);
    ASSERT_NE(fonts, nullptr);

    const int texture = _resources.GetGlyphTexture(*fonts->glyphs[0], 0);
    ASSERT_NE(texture, neon::No_Texture);
    EXPECT_EQ(_renderer.created, 1u);
    EXPECT_EQ(_renderer.updated, 0u);

    // nothing changed, so nothing is handed over
    EXPECT_EQ(_resources.GetGlyphTexture(*fonts->glyphs[0], 0), texture);
    EXPECT_EQ(_renderer.updated, 0u);

    (void) PlaceShapedText(ShapeText(U"\u0416\u0434\u044F", fonts->fonts, {}), fonts->fonts, {});

    EXPECT_EQ(_resources.GetGlyphTexture(*fonts->glyphs[0], 0), texture) << "the texture stays the one it was";
    EXPECT_EQ(_renderer.created, 1u);
    EXPECT_EQ(_renderer.updated, 1u);

    EXPECT_EQ(_resources.GetGlyphTexture(*fonts->glyphs[0], 0), texture);
    EXPECT_EQ(_renderer.updated, 1u);
  }

  TEST_F(TextResourcesTest, GivesARendererThatCannotWriteToATextureANewOne)
  {
    _renderer.refuses_updates = true;

    const neon::UiTextFonts *fonts = _resources.GetTextFonts("sans-serif", 400, false, 16);
    ASSERT_NE(fonts, nullptr);

    const int texture = _resources.GetGlyphTexture(*fonts->glyphs[0], 0);
    (void) PlaceShapedText(ShapeText(U"\u0416\u0434\u044F", fonts->fonts, {}), fonts->fonts, {});

    EXPECT_NE(_resources.GetGlyphTexture(*fonts->glyphs[0], 0), neon::No_Texture);
    EXPECT_EQ(_renderer.created, 2u);
    EXPECT_EQ(_renderer.destroyed, 1u);
    EXPECT_EQ(_renderer.TextureCount(), 1u);
    (void) texture;
  }

  TEST_F(TextResourcesTest, MakesABoldAndAnItalicThatAFamilyDoesNotHave)
  {
    const neon::UiTextFonts *regular = _resources.GetTextFonts("sans-serif", 400, false, 32);
    const neon::UiTextFonts *bold = _resources.GetTextFonts("sans-serif", 700, false, 32);
    const neon::UiTextFonts *italic = _resources.GetTextFonts("sans-serif", 400, true, 32);

    ASSERT_NE(regular, nullptr);
    ASSERT_NE(bold, nullptr);
    ASSERT_NE(italic, nullptr);

    // one face, and three sets of glyphs
    EXPECT_EQ(regular->fonts[0].font, bold->fonts[0].font);
    EXPECT_NE(regular->fonts[0].atlas, bold->fonts[0].atlas);
    EXPECT_NE(regular->fonts[0].atlas, italic->fonts[0].atlas);

    unsigned int glyph = 0;
    ASSERT_TRUE(_rasterizer.GetGlyph(regular->fonts[0].font, U'l', glyph));

    const neon::CachedGlyph *plain = regular->fonts[0].atlas->Find(glyph);
    const neon::CachedGlyph *thick = bold->fonts[0].atlas->Find(glyph);
    const neon::CachedGlyph *leaning = italic->fonts[0].atlas->Find(glyph);

    EXPECT_GT(thick->width, plain->width);
    EXPECT_GT(leaning->width, plain->width + 2);
  }

  TEST_F(TextResourcesTest, TakesTheItalicOfAFamilyThatHasOne)
  {
    neon::UiFaceOptions options;
    options.is_italic = true;
    _resources.AddFace("sans-serif", 400, "assets://fonts/arabic.ttf", options);

    const neon::UiTextFonts *upright = _resources.GetTextFonts("sans-serif", 400, false, 16);
    const neon::UiTextFonts *italic = _resources.GetTextFonts("sans-serif", 400, true, 16);

    ASSERT_NE(upright, nullptr);
    ASSERT_NE(italic, nullptr);
    EXPECT_NE(upright->fonts[0].font, italic->fonts[0].font) << "the other file";
  }

  TEST_F(TextResourcesTest, KeepsGlyphsAsDistancesAtOneSizeForEverySize)
  {
    neon::UiFaceOptions options;
    options.rendering = neon::GlyphRendering::DistanceField;
    _resources.AddFace("title", 400, "assets://fonts/inter.ttf", options);

    const neon::UiTextFonts *small = _resources.GetTextFonts("title", 400, false, 24);
    const neon::UiTextFonts *large = _resources.GetTextFonts("title", 400, false, 96);

    ASSERT_NE(small, nullptr) << _logger->Messages(LogLevel::Error);
    ASSERT_NE(large, nullptr);

    EXPECT_EQ(small->fonts[0].atlas, large->fonts[0].atlas) << "one set of glyphs";
    EXPECT_TRUE(small->fonts[0].atlas->IsDistanceField());
    EXPECT_FLOAT_EQ(small->fonts[0].atlas->GetPixelSize(), neon::UiResources::kDistance_Field_Size);

    EXPECT_FLOAT_EQ(small->fonts[0].scale, 0.5f);
    EXPECT_FLOAT_EQ(large->fonts[0].scale, 2.0f);
    EXPECT_FALSE(small->fonts[0].places_at_parts);

    // a text is as wide as its size says, whatever size its glyphs are
    // kept at
    const auto at_24 = PlaceShapedText(ShapeText(U"Neon", small->fonts, {}), small->fonts, {});
    const auto at_96 = PlaceShapedText(ShapeText(U"Neon", large->fonts, {}), large->fonts, {});
    EXPECT_NEAR(at_24.width * 4.0f, at_96.width, 4.0f);

    const neon::UiTextFonts *bitmap = _resources.GetTextFonts("sans-serif", 400, false, 24);
    const auto as_bitmap = PlaceShapedText(ShapeText(U"Neon", bitmap->fonts, {}), bitmap->fonts, {});
    EXPECT_NEAR(at_24.width, as_bitmap.width, 1.0f);
  }

  TEST_F(TextResourcesTest, ReleasesTheTexturesOfItsGlyphs)
  {
    const neon::UiTextFonts *fonts = _resources.GetTextFonts("sans-serif, arabic", 400, false, 16);
    ASSERT_NE(fonts, nullptr);

    (void) PlaceShapedText(ShapeText(U"\u0633\u0644\u0627\u0645", fonts->fonts, {}), fonts->fonts, {});
    (void) _resources.GetGlyphTexture(*fonts->glyphs[0], 0);
    (void) _resources.GetGlyphTexture(*fonts->glyphs[1], 0);

    EXPECT_EQ(_renderer.TextureCount(), 2u);

    _resources.CleanUp();

    EXPECT_EQ(_renderer.TextureCount(), 0u);
  }
}
