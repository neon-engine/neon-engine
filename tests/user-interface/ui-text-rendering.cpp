#include "ui-fixture.hpp"

#include <neon/text/font-rasterizer.hpp>

// Text with a font that can do what the font of the other tests cannot:
// draw a glyph as distances, draw the line around it, and move it by a part
// of a pixel.

namespace
{
  using neon::FontMetrics;
  using neon::FontRasterizer;
  using neon::GlyphBitmap;
  using neon::GlyphOptions;
  using neon::GlyphRendering;
  using neon::Shape2D;
  using neon::ShapeKind2D;
  using neon::Tree_UiSystem;
  using neon::UiSettings;
  using neon::testing::LogLevel;
  using neon::testing::UiTest;

  /// A font whose characters are boxes, as those of the other tests. At the
  /// size 20 a character moves the pen by 10.4 and is 8 by 14. As
  /// distances it is larger by 8 on every side. The line around it makes
  /// it larger by half its width on every side.
  class AbleRasterizer final : public FontRasterizer
  {
  public:
    std::vector<std::pair<float, GlyphOptions>> asked;

    int LoadFont(const std::vector<unsigned char> &file) override { return file.empty() ? -1 : 0; }

    void UnloadFont(int font) override {}

    bool GetMetrics(int font, const float pixel_size, FontMetrics &metrics) override
    {
      metrics = {pixel_size * 0.8f, pixel_size * 0.2f, 0.0f};
      return true;
    }

    bool HasGlyph(int font, const char32_t character) override { return character >= 0x20; }

    bool Rasterize(int font, const float pixel_size, const char32_t character, GlyphBitmap &glyph) override
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
      asked.emplace_back(pixel_size, options);

      bitmap = GlyphBitmap{};
      bitmap.advance = pixel_size * 0.52f;
      if (glyph == U' ') { return true; }

      int grown = 0;
      if (options.rendering == GlyphRendering::DistanceField)
      {
        grown = 8;
        bitmap.is_distance_field = true;
        bitmap.distance_range = 8.0f;
      } else if (options.stroke > 0.0f)
      {
        grown = static_cast<int>(options.stroke / 2.0f + 0.5f);
      }

      bitmap.width = static_cast<int>(pixel_size * 0.4f) + 2 * grown;
      bitmap.height = static_cast<int>(pixel_size * 0.7f) + 2 * grown;
      bitmap.left = 1 - grown;
      bitmap.top = static_cast<int>(pixel_size * 0.7f) + grown;
      bitmap.coverage.assign(static_cast<std::size_t>(bitmap.width) * bitmap.height, 255);
      return true;
    }

    [[nodiscard]] bool PlacesAtPartsOfAPixel() const override { return true; }
  };

  class UiTextRenderingTest : public UiTest
  {
  protected:
    AbleRasterizer _able;

    void SetUp() override
    {
      UiTest::SetUp();

      _ui->CleanUp();
      _ui = std::make_unique<Tree_UiSystem>(
        &_renderer,
        &_able,
        &_layout,
        &_input,
        &_file_system,
        &_yaml,
        UiSettings{
          .fonts = {
            {"sans-serif", 400, "assets://fonts/regular.ttf"},
            {"title", 400, "assets://fonts/bold.ttf", {false, GlyphRendering::DistanceField}}
          },
          .start_path = ""
        },
        _logger);
    }

    void ShowLabel(const std::string &text, const std::string &properties = "")
    {
      ASSERT_GE(ShowUnderRoot(
        "- type: label\n"
        "  name: label\n"
        "  text: \"" + text + "\"\n"
        "  position: absolute\n"
        "  left: 100\n"
        "  top: 100\n"
        "  font_size: 20\n" + Indented(properties, "  ")), 0) << _logger->Messages(LogLevel::Error);
      Frame();
    }

    [[nodiscard]] const Shape2D &ShapeOf(const std::size_t quad) const
    {
      const auto &batch = _renderer.batches.at(0);
      return batch.shapes.at(static_cast<std::size_t>(batch.vertices.at(quad * 4).shape));
    }
  };

  // parts of a pixel

  TEST_F(UiTextRenderingTest, PlacesGlyphsAtQuartersOfAPixel)
  {
    ShowLabel("aaaa");

    // the pen is at 0, 10.4, 20.8, and 31.2: the glyphs are drawn at whole
    // pixels, from pictures that are moved by what is left
    const auto quads = _renderer.Quads();
    ASSERT_EQ(quads.size(), 4u);

    EXPECT_FLOAT_EQ(quads[0].left, 101);
    EXPECT_FLOAT_EQ(quads[1].left, 111);
    EXPECT_FLOAT_EQ(quads[2].left, 121);
    EXPECT_FLOAT_EQ(quads[3].left, 132);

    // four glyphs of one character, from three places of the atlas
    EXPECT_NE(quads[0].texture_left, quads[1].texture_left);
    EXPECT_NE(quads[1].texture_left, quads[2].texture_left);
    EXPECT_EQ(quads[0].texture, quads[2].texture);

    std::vector<float> offsets;
    for (const auto &[size, options] : _able.asked)
    {
      if (options.offset_x > 0.0f) { offsets.push_back(options.offset_x); }
    }
    EXPECT_EQ(offsets, (std::vector<float>{0.5f, 0.75f, 0.25f}));

    for (const auto &quad : quads)
    {
      EXPECT_FLOAT_EQ(quad.top, 102) << "rows are at whole pixels";
      EXPECT_FLOAT_EQ(quad.left, std::round(quad.left));
    }
  }

  TEST_F(UiTextRenderingTest, MeasuresATextAsWideAsItsFontMeansItToBe)
  {
    ShowLabel("aaaa");

    // 41.6, rounded up so that the text fits its box
    ExpectBox("label", 100, 100, 42, 20);
  }

  TEST_F(UiTextRenderingTest, DrawsAPictureOfAQuarterOnce)
  {
    ShowLabel("aaaa");
    const std::size_t after_the_first = _able.asked.size();

    Frame();
    Frame();

    EXPECT_EQ(_able.asked.size(), after_the_first);
    EXPECT_EQ(_renderer.created, 1u);
  }

  // the line around a glyph

  TEST_F(UiTextRenderingTest, DrawsTheLineAroundEveryGlyphOverTheGlyph)
  {
    ShowLabel("ab", "text_stroke_width: 4\ntext_stroke_color: \"#ff0000\"\ncolor: \"#ffffff\"\n");

    const auto quads = _renderer.Quads();
    ASSERT_EQ(quads.size(), 4u);

    // the glyphs first, in the colour of the text
    EXPECT_FLOAT_EQ(quads[0].left, 101);
    EXPECT_FLOAT_EQ(quads[0].Width(), 8);
    EXPECT_FLOAT_EQ(quads[0].color.g, 1);

    // then their lines, which are larger by half their width on every
    // side, and lie around the same middle
    EXPECT_FLOAT_EQ(quads[2].left, 99);
    EXPECT_FLOAT_EQ(quads[2].top, 100);
    EXPECT_FLOAT_EQ(quads[2].Width(), 12);
    EXPECT_FLOAT_EQ(quads[2].Height(), 18);
    EXPECT_FLOAT_EQ(quads[2].color.r, 1);
    EXPECT_FLOAT_EQ(quads[2].color.g, 0);

    // pictures of their own in the same atlas, which costs no draw call
    EXPECT_EQ(quads[0].texture, quads[2].texture);
    EXPECT_EQ(_renderer.batches.size(), 1u);
  }

  TEST_F(UiTextRenderingTest, TheLineAroundAGlyphGrowsWithTheFrame)
  {
    _renderer.SetResolution(3840, 2160);
    ShowLabel("a", "text_stroke_width: 2\n");

    bool was_asked = false;
    for (const auto &[size, options] : _able.asked)
    {
      if (options.stroke > 0.0f)
      {
        was_asked = true;
        EXPECT_FLOAT_EQ(options.stroke, 4);
        EXPECT_FLOAT_EQ(size, 40);
      }
    }
    EXPECT_TRUE(was_asked);
  }

  TEST_F(UiTextRenderingTest, ALineWithoutAColourHasThatOfTheText)
  {
    ShowLabel("a", "text_stroke_width: 2\ncolor: \"#00ff00\"\n");

    const auto quads = _renderer.Quads();
    ASSERT_EQ(quads.size(), 2u);
    EXPECT_FLOAT_EQ(quads[1].color.g, 1);
    EXPECT_FLOAT_EQ(quads[1].color.r, 0);
  }

  // distances

  TEST_F(UiTextRenderingTest, KeepsTheGlyphsOfAFontOfDistancesAtOneSize)
  {
    ASSERT_GE(ShowUnderRoot(
      "- type: label\n  text: a\n  font_family: title\n  font_size: 24\n"
      "- type: label\n  text: a\n  font_family: title\n  font_size: 96\n"
      "- type: label\n  text: a\n  font_family: title\n  font_size: 12\n"), 0);
    Frame();

    // one atlas for all three, drawn at 48
    EXPECT_EQ(_renderer.created, 1u);

    for (const auto &[size, options] : _able.asked)
    {
      EXPECT_FLOAT_EQ(size, 48);
      EXPECT_EQ(options.rendering, GlyphRendering::DistanceField);
      EXPECT_FLOAT_EQ(options.offset_x, 0) << "what is scaled is not moved by parts of a pixel";
    }

    const auto quads = _renderer.Quads();
    ASSERT_EQ(quads.size(), 3u);
    EXPECT_EQ(quads[0].texture, quads[1].texture);
    EXPECT_FLOAT_EQ(quads[0].texture_left, quads[1].texture_left) << "the same picture, at three sizes";

    // The picture is 19 + 16 by 33 + 16 at the size 48, and is drawn half
    // as large, twice as large, and a quarter as large.
    EXPECT_FLOAT_EQ(quads[0].Width(), 17.5f);
    EXPECT_FLOAT_EQ(quads[1].Width(), 70);
    EXPECT_FLOAT_EQ(quads[2].Width(), 8.75f);
    EXPECT_FLOAT_EQ(quads[1].Height(), 98);
  }

  TEST_F(UiTextRenderingTest, ATextOfDistancesIsAsWideAsOneOfBitmaps)
  {
    ASSERT_GE(ShowUnderRoot(
      "- type: label\n  name: distances\n  text: abcd\n  font_family: title\n  font_size: 20\n"
      "- type: label\n  name: bitmaps\n  text: abcd\n  font_size: 20\n"), 0);
    Frame();

    EXPECT_FLOAT_EQ(Element("distances").GetBox().Width(), Element("bitmaps").GetBox().Width());
    EXPECT_FLOAT_EQ(Element("distances").GetBox().Height(), Element("bitmaps").GetBox().Height());
  }

  TEST_F(UiTextRenderingTest, TellsTheRendererToReadDistances)
  {
    ShowLabel("a", "font_family: title\n");

    ASSERT_EQ(_renderer.batches.size(), 1u);
    const auto &vertices = _renderer.batches[0].vertices;
    ASSERT_EQ(vertices.size(), 4u);

    for (const auto &vertex : vertices) { EXPECT_FLOAT_EQ(vertex.textured, 2); }

    const Shape2D &shape = ShapeOf(0);
    EXPECT_EQ(static_cast<int>(shape.box[2]), static_cast<int>(ShapeKind2D::Text));

    EXPECT_FLOAT_EQ(shape.widths[0], 0) << "no line around it";
    EXPECT_FLOAT_EQ(shape.widths[1], 0) << "in focus";

    // the distances reach over 8 pixels of the atlas, which are drawn at
    // 20 of 48
    EXPECT_NEAR(shape.widths[2], 8.0f * 20.0f / 48.0f, 0.001f);
  }

  TEST_F(UiTextRenderingTest, ATextOfBitmapsIsReadAsItIs)
  {
    ShowLabel("a");

    const auto &batch = _renderer.batches.at(0);
    EXPECT_TRUE(batch.shapes.empty());
    for (const auto &vertex : batch.vertices)
    {
      EXPECT_FLOAT_EQ(vertex.textured, 1);
      EXPECT_LT(vertex.shape, 0);
    }
  }

  TEST_F(UiTextRenderingTest, TheLineAroundAGlyphOfDistancesIsDrawnByTheShader)
  {
    ShowLabel(
      "ab",
      "font_family: title\n"
      "text_stroke_width: 3\n"
      "text_stroke_color: \"#ff0000\"\n");

    // no picture of its own, and no rectangle of its own
    const auto quads = _renderer.Quads();
    ASSERT_EQ(quads.size(), 2u);

    const Shape2D &shape = ShapeOf(0);
    EXPECT_FLOAT_EQ(shape.widths[0], 3);
    EXPECT_FLOAT_EQ(shape.colors[0][0], 1);
    EXPECT_FLOAT_EQ(shape.colors[0][1], 0);
    EXPECT_FLOAT_EQ(shape.colors[0][3], 1);

    for (const auto &[size, options] : _able.asked) { EXPECT_FLOAT_EQ(options.stroke, 0); }

    // both glyphs refer to the one shape
    EXPECT_EQ(_renderer.batches.at(0).shapes.size(), 1u);
  }

  TEST_F(UiTextRenderingTest, TheShadowOfATextOfDistancesIsDrawnByTheShader)
  {
    ShowLabel("a", "font_family: title\ntext_shadow: \"2px 3px 6px #000000\"\n");

    const auto quads = _renderer.Quads();
    ASSERT_EQ(quads.size(), 2u);

    // the same picture, moved, and put out of focus by the shader
    EXPECT_FLOAT_EQ(quads[0].left, quads[1].left + 2);
    EXPECT_FLOAT_EQ(quads[0].top, quads[1].top + 3);
    EXPECT_FLOAT_EQ(quads[0].texture_left, quads[1].texture_left);
    EXPECT_FLOAT_EQ(quads[0].color.r, 0);

    EXPECT_FLOAT_EQ(ShapeOf(0).widths[1], 6);
    EXPECT_FLOAT_EQ(ShapeOf(1).widths[1], 0);
  }

  TEST_F(UiTextRenderingTest, ReadsHowAFontIsKeptFromAFile)
  {
    WriteAsset("fonts/sign.ttf", "a font");

    ASSERT_GE(Show(
      "fonts:\n"
      "  - family: sign\n"
      "    src: assets://fonts/sign.ttf\n"
      "    rendering: sdf\n"
      "  - family: plain\n"
      "    src: assets://fonts/sign.ttf\n"
      "    rendering: bitmap\n"
      "root:\n"
      "  type: panel\n"
      "  children:\n"
      "    - type: label\n"
      "      text: a\n"
      "      font_family: sign\n"
      "    - type: label\n"
      "      text: a\n"
      "      font_family: plain\n"), 0) << _logger->Messages(LogLevel::Error);
    Frame();

    const auto &batches = _renderer.batches;
    ASSERT_EQ(batches.size(), 2u) << "two atlases";
    EXPECT_FLOAT_EQ(batches[0].vertices.at(0).textured, 2);
    EXPECT_FLOAT_EQ(batches[1].vertices.at(0).textured, 1);
  }
}
