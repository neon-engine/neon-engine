#include "ui-fixture.hpp"

// What a user interface hands to the renderer: which rectangles, in which
// order, in how many draw calls, and cut off where.

namespace
{
  using neon::ClipRectangle;
  using neon::No_Texture;
  using neon::testing::LogLevel;
  using neon::testing::RecordedQuad;
  using neon::testing::UiTest;

  class UiDrawingTest : public UiTest
  {
  protected:
    static void ExpectQuad(
      const RecordedQuad &quad,
      const float left,
      const float top,
      const float width,
      const float height)
    {
      EXPECT_FLOAT_EQ(quad.left, left);
      EXPECT_FLOAT_EQ(quad.top, top);
      EXPECT_FLOAT_EQ(quad.Width(), width);
      EXPECT_FLOAT_EQ(quad.Height(), height);
    }

    static void ExpectColor(
      const RecordedQuad &quad,
      const float red,
      const float green,
      const float blue,
      const float alpha)
    {
      EXPECT_NEAR(quad.color.r, red, 0.002f);
      EXPECT_NEAR(quad.color.g, green, 0.002f);
      EXPECT_NEAR(quad.color.b, blue, 0.002f);
      EXPECT_NEAR(quad.color.a, alpha, 0.002f);
    }

    static void ExpectPart(
      const RecordedQuad &quad,
      const float left,
      const float top,
      const float right,
      const float bottom)
    {
      EXPECT_NEAR(quad.texture_left, left, 0.0001f);
      EXPECT_NEAR(quad.texture_top, top, 0.0001f);
      EXPECT_NEAR(quad.texture_right, right, 0.0001f);
      EXPECT_NEAR(quad.texture_bottom, bottom, 0.0001f);
    }

    /// A box of 200 by 100 at 100, 100 with the properties that are given.
    void ShowBox(const std::string &properties, const std::string &children = "")
    {
      std::string element =
        "- type: panel\n"
        "  name: box\n"
        "  position: absolute\n"
        "  left: 100\n"
        "  top: 100\n"
        "  box_sizing: border-box\n"
        "  width: 200\n"
        "  height: 100\n" + Indented(properties, "  ");

      if (!children.empty()) { element += "  children:\n" + Indented(children, "    "); }

      ASSERT_GE(ShowUnderRoot(element), 0) << _logger->Messages(LogLevel::Error);
      Frame();
    }
  };

  // nothing to draw

  TEST_F(UiDrawingTest, DrawsNothingWithoutAFile)
  {
    Frame();

    EXPECT_TRUE(_renderer.batches.empty());
    EXPECT_EQ(_ui->GetDrawCalls(), 0u);
    EXPECT_EQ(_renderer.created + _renderer.loaded, 0u) << "and asks for no texture";
  }

  TEST_F(UiDrawingTest, DrawsNothingForElementsThatHaveNothingToShow)
  {
    ASSERT_GE(ShowUnderRoot("- type: panel\n  width: 100\n- type: label\n- type: panel\n"), 0);
    Frame();

    EXPECT_TRUE(_renderer.batches.empty());
  }

  TEST_F(UiDrawingTest, DrawsNothingOfWhatIsHidden)
  {
    ShowBox("background_color: \"#ff0000\"\nhidden: true\n");

    EXPECT_TRUE(_renderer.batches.empty());
  }

  TEST_F(UiDrawingTest, DrawsNothingOfWhatIsNotToBeSeen)
  {
    ShowBox("background_color: \"#ff0000\"\nopacity: 0\n", "- type: label\n  text: Paused\n");
    EXPECT_TRUE(_renderer.batches.empty());
  }

  TEST_F(UiDrawingTest, DrawsNothingOfWhatLiesOutsideTheFrame)
  {
    ASSERT_GE(ShowUnderRoot(
      "- type: panel\n"
      "  position: absolute\n"
      "  left: -300\n"
      "  top: 100\n"
      "  width: 200\n"
      "  height: 100\n"
      "  background_color: \"#ff0000\"\n"
      "- type: panel\n"
      "  position: absolute\n"
      "  left: 100\n"
      "  top: 1080\n"
      "  width: 200\n"
      "  height: 100\n"
      "  background_color: \"#ff0000\"\n"), 0);
    Frame();

    EXPECT_TRUE(_renderer.batches.empty());
  }

  // a box

  TEST_F(UiDrawingTest, DrawsTheBackgroundOverTheBorderBox)
  {
    ShowBox("background_color: \"#ff8000cc\"\npadding: 10\nborder_width: 0\n");

    const auto quads = _renderer.Quads();
    ASSERT_EQ(quads.size(), 1u);

    ExpectQuad(quads[0], 100, 100, 200, 100);
    ExpectColor(quads[0], 1, 0.502f, 0, 0.8f);
    EXPECT_FALSE(quads[0].textured);
    EXPECT_EQ(quads[0].texture, No_Texture);
    EXPECT_FALSE(quads[0].clipped);
  }

  TEST_F(UiDrawingTest, DrawsTheFourSidesOfABorderWithoutDrawingAPixelTwice)
  {
    ShowBox("border_width: 2 4 6 8\nborder_color: \"#00ff00\"\n");

    const auto quads = _renderer.Quads();
    ASSERT_EQ(quads.size(), 4u);

    // the top and the bottom run the whole width
    ExpectQuad(quads[0], 100, 100, 200, 2);
    ExpectQuad(quads[1], 100, 194, 200, 6);

    // the sides lie between them: left, then right
    ExpectQuad(quads[2], 100, 102, 8, 92);
    ExpectQuad(quads[3], 296, 102, 4, 92);

    for (const auto &quad : quads) { ExpectColor(quad, 0, 1, 0, 1); }
  }

  TEST_F(UiDrawingTest, ABorderWithoutAColourHasTheColourOfTheText)
  {
    ShowBox("border_width: 2\ncolor: \"#0000ff\"\n");

    const auto quads = _renderer.Quads();
    ASSERT_EQ(quads.size(), 4u);
    ExpectColor(quads[0], 0, 0, 1, 1);
  }

  TEST_F(UiDrawingTest, DrawsTheBackgroundBehindTheBorder)
  {
    ShowBox("background_color: \"#ff0000\"\nborder: \"2px solid #00ff00\"\n");

    const auto quads = _renderer.Quads();
    ASSERT_EQ(quads.size(), 5u);

    ExpectColor(quads[0], 1, 0, 0, 1);
    ExpectQuad(quads[0], 100, 100, 200, 100);
    ExpectColor(quads[1], 0, 1, 0, 1);
  }

  TEST_F(UiDrawingTest, DrawsAnOutlineAroundTheBorderBox)
  {
    ShowBox("outline_width: 3\noutline_offset: 2\noutline_color: \"#ffff00\"\n");

    const auto quads = _renderer.Quads();
    ASSERT_EQ(quads.size(), 4u);

    // 5 outside the box on every side, and 3 wide
    ExpectQuad(quads[0], 95, 95, 210, 3);
    ExpectQuad(quads[1], 95, 202, 210, 3);
    ExpectQuad(quads[2], 95, 98, 3, 104);
    ExpectQuad(quads[3], 302, 98, 3, 104);

    ExpectColor(quads[0], 1, 1, 0, 1);
  }

  TEST_F(UiDrawingTest, AnOutlineTakesNoRoom)
  {
    ASSERT_GE(ShowUnderRoot(
      "- type: panel\n  name: a\n  width: 100\n  outline_width: 10\n"
      "- type: panel\n  name: b\n  width: 100\n"), 0);
    Frame();

    ExpectBox("a", 0, 0, 100, 1080);
    ExpectBox("b", 100, 0, 100, 1080);
  }

  // opacity

  TEST_F(UiDrawingTest, OpacityLetsWhatIsBehindShowThrough)
  {
    ShowBox("background_color: \"#ff000080\"\nopacity: 0.5\n");

    ExpectColor(_renderer.Quads().at(0), 1, 0, 0, 0.251f);
  }

  TEST_F(UiDrawingTest, OpacityReachesEverythingBelowAnElement)
  {
    ShowBox(
      "opacity: 0.5\n",
      "- type: panel\n"
      "  width: 10\n"
      "  background_color: \"#ff0000\"\n"
      "  opacity: 0.5\n"
      "  children:\n"
      "    - type: label\n"
      "      text: a\n"
      "      color: \"#00ff00\"\n");

    const auto quads = _renderer.Quads();
    ASSERT_EQ(quads.size(), 2u);

    ExpectColor(quads[0], 1, 0, 0, 0.25f);
    ExpectColor(quads[1], 0, 1, 0, 0.25f);
  }

  // text

  TEST_F(UiDrawingTest, DrawsEveryCharacterOfATextFromTheAtlasOfItsFont)
  {
    ASSERT_GE(ShowUnderRoot(
      "- type: label\n"
      "  text: Hi you\n"
      "  position: absolute\n"
      "  left: 100\n"
      "  top: 100\n"
      "  font_size: 20\n"
      "  color: \"#ff8000\"\n"), 0);
    Frame();

    const auto quads = _renderer.Quads();

    // the space draws nothing
    ASSERT_EQ(quads.size(), 5u);
    ASSERT_EQ(_renderer.batches.size(), 1u);

    const int atlas = quads[0].texture;
    EXPECT_NE(atlas, No_Texture);

    // a character of the font of the tests is 8 by 14 at the size 20,
    // starts 1 right of the pen, and the pen moves by 10
    ExpectQuad(quads[0], 101, 102, 8, 14);
    ExpectQuad(quads[1], 111, 102, 8, 14);
    ExpectQuad(quads[2], 131, 102, 8, 14);
    ExpectQuad(quads[4], 151, 102, 8, 14);

    for (const auto &quad : quads)
    {
      EXPECT_TRUE(quad.textured);
      EXPECT_EQ(quad.texture, atlas);
      ExpectColor(quad, 1, 0.502f, 0, 1);
    }

    // two characters are two places of the atlas
    EXPECT_NE(quads[0].texture_left, quads[1].texture_left);
  }

  TEST_F(UiDrawingTest, AlignsTextInItsBox)
  {
    ASSERT_GE(ShowUnderRoot(
      "- type: label\n"
      "  text: ab\n"
      "  position: absolute\n"
      "  left: 100\n"
      "  top: 100\n"
      "  width: 100\n"
      "  font_size: 20\n"
      "  text_align: right\n"
      "- type: label\n"
      "  text: ab\n"
      "  position: absolute\n"
      "  left: 100\n"
      "  top: 200\n"
      "  width: 100\n"
      "  font_size: 20\n"
      "  text_align: center\n"), 0);
    Frame();

    const auto quads = _renderer.Quads();
    ASSERT_EQ(quads.size(), 4u);

    EXPECT_FLOAT_EQ(quads[0].left, 181);
    EXPECT_FLOAT_EQ(quads[2].left, 141);
  }

  TEST_F(UiDrawingTest, TheTextOfAButtonIsInItsMiddle)
  {
    ASSERT_GE(ShowUnderRoot(
      "- type: button\n"
      "  text: ab\n"
      "  position: absolute\n"
      "  left: 100\n"
      "  top: 100\n"
      "  box_sizing: border-box\n"
      "  width: 200\n"
      "  height: 100\n"
      "  padding: 0\n"
      "  font_size: 20\n"), 0);
    Frame();

    const auto quads = _renderer.Quads();
    ASSERT_EQ(quads.size(), 3u);

    // the text is 20 by 20, in a box of 200 by 100
    EXPECT_FLOAT_EQ(quads[1].left, 100 + 90 + 1);
    EXPECT_FLOAT_EQ(quads[1].top, 100 + 40 + 2);
  }

  TEST_F(UiDrawingTest, EverySizeOfAFontHasAnAtlasThatIsMadeOnce)
  {
    ASSERT_GE(ShowUnderRoot(
      "- type: label\n  text: small\n  font_size: 12\n"
      "- type: label\n  text: large\n  font_size: 24\n"
      "- type: label\n  text: large again\n  font_size: 24\n"
      "- type: label\n  text: bold\n  font_size: 24\n  font_weight: bold\n"), 0);

    Frame();
    Frame();
    Frame();

    EXPECT_EQ(_renderer.created, 3u);
    EXPECT_EQ(_renderer.TextureCount(), 3u);
  }

  TEST_F(UiDrawingTest, TakesTheWeightOfAFontThatIsNearest)
  {
    ASSERT_GE(ShowUnderRoot(
      "- type: label\n  text: a\n  font_weight: 300\n"
      "- type: label\n  text: b\n  font_weight: 500\n"
      "- type: label\n  text: c\n  font_weight: 600\n"
      "- type: label\n  text: d\n  font_weight: 900\n"), 0);
    Frame();

    const auto quads = _renderer.Quads();
    ASSERT_EQ(quads.size(), 4u);

    // 300 and 500 are nearer to 400, 600 and 900 to 700
    EXPECT_EQ(quads[0].texture, quads[1].texture);
    EXPECT_EQ(quads[2].texture, quads[3].texture);
    EXPECT_NE(quads[0].texture, quads[2].texture);
  }

  TEST_F(UiDrawingTest, DrawsWithAFontTheFileNames)
  {
    WriteAsset("fonts/title.ttf", "a font");

    ASSERT_GE(Show(
      "fonts:\n"
      "  - family: title\n"
      "    src: assets://fonts/title.ttf\n"
      "root:\n"
      "  type: panel\n"
      "  children:\n"
      "    - type: label\n"
      "      text: a\n"
      "    - type: label\n"
      "      text: b\n"
      "      font_family: title\n"), 0) << _logger->Messages(LogLevel::Error);
    Frame();

    const auto quads = _renderer.Quads();
    ASSERT_EQ(quads.size(), 2u);
    EXPECT_NE(quads[0].texture, quads[1].texture);
    EXPECT_TRUE(Errors().empty()) << _logger->Messages(LogLevel::Error);
  }

  // what cannot be loaded

  TEST_F(UiDrawingTest, SaysOnceThatAFontFamilyIsNotKnown)
  {
    ASSERT_GE(ShowUnderRoot(
      "- type: label\n  name: a\n  text: a\n  font_family: missing\n  background_color: \"#ff0000\"\n"
      "  width: 50\n"
      "- type: label\n  text: b\n  font_family: missing\n"), 0);

    Frame();
    Frame();
    Frame();

    EXPECT_EQ(Errors().size(), 1u) << _logger->Messages(LogLevel::Error);
    EXPECT_TRUE(_logger->Contains(
      LogLevel::Error, "The font family 'missing' is not known, text that asks for it is not drawn"));

    // the element is drawn without its text
    ASSERT_EQ(_renderer.Quads().size(), 1u);
    EXPECT_FALSE(_renderer.Quads()[0].textured);
  }

  TEST_F(UiDrawingTest, ReadsAFontThatIsMissingOnce)
  {
    WriteAsset("fonts/broken.ttf", "");

    ASSERT_GE(Show(
      "fonts:\n"
      "  - family: gone\n"
      "    src: assets://fonts/gone.ttf\n"
      "  - family: broken\n"
      "    src: assets://fonts/broken.ttf\n"
      "root:\n"
      "  type: panel\n"
      "  children:\n"
      "    - type: label\n"
      "      text: a\n"
      "      font_family: gone\n"
      "    - type: label\n"
      "      text: b\n"
      "      font_family: broken\n"), 0) << _logger->Messages(LogLevel::Error);

    Frame();
    Frame();
    Frame();

    // The file system says that the file is missing, and the user
    // interface that the font cannot be read. Neither is said again.
    EXPECT_EQ(_logger->Count(LogLevel::Error), 2u) << _logger->Messages(LogLevel::Error);
    EXPECT_TRUE(_logger->Contains(LogLevel::Error, "The font assets://fonts/gone.ttf cannot be read"));
    EXPECT_TRUE(_logger->Contains(LogLevel::Error, "assets://fonts/broken.ttf is not a font that can be used"));
    EXPECT_TRUE(_renderer.batches.empty());
  }

  TEST_F(UiDrawingTest, AsksOnceForATextureTheRendererDoesNotTake)
  {
    _renderer.refuses_pixels = true;

    ASSERT_GE(ShowUnderRoot("- type: label\n  text: a\n"), 0);

    Frame();
    Frame();
    Frame();

    EXPECT_EQ(_renderer.created, 1u);
    EXPECT_EQ(Errors().size(), 1u) << _logger->Messages(LogLevel::Error);
    EXPECT_TRUE(_logger->Contains(LogLevel::Error, "The renderer took no texture for the font"));
    EXPECT_EQ(_rasterizer.rasterized, _rasterizer.rasterized);
  }

  TEST_F(UiDrawingTest, DrawsTheCharactersOfAFontOnce)
  {
    ASSERT_GE(ShowUnderRoot("- type: label\n  text: a\n"), 0);

    Frame();
    const std::size_t after_the_first = _rasterizer.rasterized;
    EXPECT_GT(after_the_first, 300u);

    Frame();
    Frame();

    EXPECT_EQ(_rasterizer.rasterized, after_the_first);
  }

  TEST_F(UiDrawingTest, LoadsAnImageThatIsMissingOnce)
  {
    _renderer.missing = {"assets://ui/missing.png"};

    ASSERT_GE(ShowUnderRoot(
      "- type: image\n"
      "  src: assets://ui/missing.png\n"
      "  width: 50\n"
      "  height: 50\n"
      "  background_color: \"#ff0000\"\n"
      "- type: panel\n"
      "  width: 50\n"
      "  background_image: assets://ui/missing.png\n"), 0);

    Frame();
    Frame();
    Frame();

    EXPECT_EQ(_renderer.loaded, 1u);
    EXPECT_EQ(Errors().size(), 1u) << _logger->Messages(LogLevel::Error);
    EXPECT_TRUE(_logger->Contains(
      LogLevel::Error, "The image assets://ui/missing.png cannot be used, what asks for it is drawn without it"));

    // what is around the image is drawn all the same
    ASSERT_EQ(_renderer.Quads().size(), 1u);
    EXPECT_FALSE(_renderer.Quads()[0].textured);
  }

  // images

  TEST_F(UiDrawingTest, DrawsAnImageOverItsContentBox)
  {
    ASSERT_GE(ShowUnderRoot(
      "- type: image\n"
      "  src: assets://ui/heart.png\n"
      "  position: absolute\n"
      "  left: 100\n"
      "  top: 100\n"
      "  width: 32\n"
      "  height: 32\n"
      "  padding: 4\n"), 0);
    Frame();

    const auto quads = _renderer.Quads();
    ASSERT_EQ(quads.size(), 1u);

    ExpectQuad(quads[0], 104, 104, 32, 32);
    ExpectPart(quads[0], 0, 0, 1, 1);
    ExpectColor(quads[0], 1, 1, 1, 1);
    EXPECT_TRUE(quads[0].textured);
    EXPECT_EQ(quads[0].texture, _renderer.TextureOf("assets://ui/heart.png"));
  }

  TEST_F(UiDrawingTest, AnImageThatIsUsedTwiceIsLoadedOnce)
  {
    ASSERT_GE(ShowUnderRoot(
      "- type: image\n  src: assets://ui/heart.png\n"
      "- type: image\n  src: assets://ui/heart.png\n"
      "- type: panel\n  width: 10\n  background_image: assets://ui/heart.png\n"), 0);
    Frame();
    Frame();

    EXPECT_EQ(_renderer.loaded, 1u);
    EXPECT_EQ(_renderer.Quads().size(), 3u);
  }

  TEST_F(UiDrawingTest, FitsAnImageIntoItsBox)
  {
    // twice as wide as it is high, in a box that is square
    _renderer.image_width = 200;
    _renderer.image_height = 100;

    const std::string image =
      "- type: image\n"
      "  src: assets://ui/wide.png\n"
      "  position: absolute\n"
      "  left: 100\n"
      "  top: 100\n"
      "  width: 100\n"
      "  height: 100\n";

    int document = ShowUnderRoot(image + "  object_fit: fill\n");
    Frame();
    ExpectQuad(_renderer.Quads().at(0), 100, 100, 100, 100);
    ExpectPart(_renderer.Quads().at(0), 0, 0, 1, 1);
    _ui->Unload(document);

    // the whole image, as large as it fits, in the middle
    document = ShowUnderRoot(image + "  object_fit: contain\n");
    Frame();
    ExpectQuad(_renderer.Quads().at(0), 100, 125, 100, 50);
    ExpectPart(_renderer.Quads().at(0), 0, 0, 1, 1);
    _ui->Unload(document);

    // the whole box, with the sides of the image cut off
    document = ShowUnderRoot(image + "  object_fit: cover\n");
    Frame();
    ExpectQuad(_renderer.Quads().at(0), 100, 100, 100, 100);
    ExpectPart(_renderer.Quads().at(0), 0.25f, 0, 0.75f, 1);
    _ui->Unload(document);
  }

  TEST_F(UiDrawingTest, StretchesABackgroundImageOverTheBox)
  {
    ShowBox("background_image: assets://ui/paper.png\nbackground_color: \"#ff0000\"\n");

    const auto quads = _renderer.Quads();
    ASSERT_EQ(quads.size(), 2u);

    EXPECT_FALSE(quads[0].textured);
    EXPECT_TRUE(quads[1].textured);
    ExpectQuad(quads[1], 100, 100, 200, 100);
    ExpectPart(quads[1], 0, 0, 1, 1);
  }

  TEST_F(UiDrawingTest, DrawsABorderImageInNineParts)
  {
    _renderer.image_width = 64;
    _renderer.image_height = 32;

    ShowBox("border_image_source: assets://ui/frame.png\nborder_image_slice: 8 16\n");

    const auto quads = _renderer.Quads();
    ASSERT_EQ(quads.size(), 9u);

    // the corners are as large as they are in the image
    ExpectQuad(quads[0], 100, 100, 16, 8);
    ExpectPart(quads[0], 0, 0, 0.25f, 0.25f);

    ExpectQuad(quads[2], 284, 100, 16, 8);
    ExpectPart(quads[2], 0.75f, 0, 1, 0.25f);

    ExpectQuad(quads[6], 100, 192, 16, 8);
    ExpectPart(quads[6], 0, 0.75f, 0.25f, 1);

    ExpectQuad(quads[8], 284, 192, 16, 8);
    ExpectPart(quads[8], 0.75f, 0.75f, 1, 1);

    // an edge is stretched along its side
    ExpectQuad(quads[1], 116, 100, 168, 8);
    ExpectPart(quads[1], 0.25f, 0, 0.75f, 0.25f);

    ExpectQuad(quads[3], 100, 108, 16, 84);
    ExpectPart(quads[3], 0, 0.25f, 0.25f, 0.75f);

    // and the middle both ways
    ExpectQuad(quads[4], 116, 108, 168, 84);
    ExpectPart(quads[4], 0.25f, 0.25f, 0.75f, 0.75f);

    for (const auto &quad : quads) { EXPECT_TRUE(quad.textured); }
    EXPECT_EQ(_renderer.batches.size(), 1u);
  }

  TEST_F(UiDrawingTest, DrawsTheCornersOfABorderImageAsWideAsTheFileSays)
  {
    _renderer.image_width = 64;
    _renderer.image_height = 64;

    ShowBox(
      "border_image_source: assets://ui/frame.png\n"
      "border_image_slice: 16\n"
      "border_image_width: 32\n");

    const auto quads = _renderer.Quads();
    ASSERT_EQ(quads.size(), 9u);

    ExpectQuad(quads[0], 100, 100, 32, 32);
    ExpectPart(quads[0], 0, 0, 0.25f, 0.25f);
    ExpectQuad(quads[4], 132, 132, 136, 36);
  }

  TEST_F(UiDrawingTest, TheCornersOfABorderImageGrowWithTheFrame)
  {
    _renderer.SetResolution(3840, 2160);
    _renderer.image_width = 64;
    _renderer.image_height = 64;

    ShowBox("border_image_source: assets://ui/frame.png\nborder_image_slice: 16\n");

    ExpectQuad(_renderer.Quads().at(0), 200, 200, 32, 32);
  }

  TEST_F(UiDrawingTest, CornersOfABorderImageThatDoNotFitAreMadeSmaller)
  {
    _renderer.image_width = 64;
    _renderer.image_height = 64;

    ShowBox(
      "border_image_source: assets://ui/frame.png\n"
      "border_image_slice: 16\n"
      "border_image_width: 100\n");

    const auto quads = _renderer.Quads();

    // the box is 100 high, which leaves 50 for each corner, and nothing
    // for what is between them
    ASSERT_EQ(quads.size(), 6u);
    ExpectQuad(quads[0], 100, 100, 50, 50);
    ExpectQuad(quads[1], 150, 100, 100, 50);
    ExpectQuad(quads[5], 250, 150, 50, 50);
  }

  // what is cut off

  TEST_F(UiDrawingTest, CutsOffWhatSticksOutOfABoxThatHidesIt)
  {
    ShowBox(
      "overflow: hidden\nborder_width: 5\nbackground_color: \"#ff0000\"\n",
      "- type: panel\n"
      "  flex_shrink: 0\n"
      "  width: 500\n"
      "  background_color: \"#00ff00\"\n");

    const auto quads = _renderer.Quads();
    ASSERT_EQ(quads.size(), 6u);

    // the box itself is drawn as a whole
    EXPECT_FALSE(quads[0].clipped);
    EXPECT_FALSE(quads[4].clipped);

    // what is inside is cut off at the padding box
    EXPECT_TRUE(quads[5].clipped);
    EXPECT_EQ(quads[5].clip, (ClipRectangle{105, 105, 190, 90}));
    ExpectQuad(quads[5], 105, 105, 500, 90);

    EXPECT_EQ(_renderer.batches.size(), 2u);
  }

  TEST_F(UiDrawingTest, WhatIsCutOffTwiceIsCutOffAtWhatBothLeave)
  {
    ShowBox(
      "overflow: hidden\n",
      "- type: panel\n"
      "  position: absolute\n"
      "  left: 150\n"
      "  top: -20\n"
      "  width: 100\n"
      "  height: 60\n"
      "  overflow: hidden\n"
      "  children:\n"
      "    - type: panel\n"
      "      flex_shrink: 0\n"
      "      width: 300\n"
      "      background_color: \"#00ff00\"\n");

    const auto quads = _renderer.Quads();
    ASSERT_EQ(quads.size(), 1u);

    EXPECT_TRUE(quads[0].clipped);
    EXPECT_EQ(quads[0].clip, (ClipRectangle{250, 100, 50, 40}));
  }

  TEST_F(UiDrawingTest, StopsCuttingOffBehindTheBox)
  {
    ASSERT_GE(ShowUnderRoot(
      "- type: panel\n"
      "  width: 100\n"
      "  overflow: hidden\n"
      "  children:\n"
      "    - type: panel\n"
      "      width: 50\n"
      "      background_color: \"#ff0000\"\n"
      "- type: panel\n"
      "  width: 100\n"
      "  background_color: \"#00ff00\"\n"), 0);
    Frame();

    const auto quads = _renderer.Quads();
    ASSERT_EQ(quads.size(), 2u);

    EXPECT_TRUE(quads[0].clipped);
    EXPECT_FALSE(quads[1].clipped);
  }

  TEST_F(UiDrawingTest, DrawsNothingOfWhatIsCutOffAltogether)
  {
    ShowBox(
      "overflow: hidden\n",
      "- type: panel\n"
      "  position: absolute\n"
      "  left: 300\n"
      "  top: 0\n"
      "  width: 100\n"
      "  height: 50\n"
      "  background_color: \"#00ff00\"\n");

    EXPECT_TRUE(_renderer.batches.empty());
  }

  TEST_F(UiDrawingTest, WhatIsCutOffFollowsTheScale)
  {
    _renderer.SetResolution(960, 540);

    ShowBox("overflow: hidden\n", "- type: panel\n  width: 10\n  background_color: \"#00ff00\"\n");

    EXPECT_EQ(_renderer.Quads().at(0).clip, (ClipRectangle{50, 50, 100, 50}));
  }

  // the order

  TEST_F(UiDrawingTest, DrawsAnElementBeforeWhatIsInsideIt)
  {
    ShowBox(
      "background_color: \"#ff0000\"\n",
      "- type: panel\n"
      "  width: 50\n"
      "  background_color: \"#00ff00\"\n"
      "  children:\n"
      "    - type: panel\n"
      "      width: 10\n"
      "      background_color: \"#0000ff\"\n"
      "- type: panel\n"
      "  width: 50\n"
      "  background_color: \"#ffff00\"\n");

    const auto quads = _renderer.Quads();
    ASSERT_EQ(quads.size(), 4u);

    ExpectColor(quads[0], 1, 0, 0, 1);
    ExpectColor(quads[1], 0, 1, 0, 1);
    ExpectColor(quads[2], 0, 0, 1, 1);
    ExpectColor(quads[3], 1, 1, 0, 1);
  }

  TEST_F(UiDrawingTest, DrawsWhatIsAboveLater)
  {
    ASSERT_GE(ShowUnderRoot(
      "- type: panel\n  width: 10\n  background_color: \"#ff0000\"\n  z_index: 2\n"
      "- type: panel\n  width: 10\n  background_color: \"#00ff00\"\n"
      "- type: panel\n  width: 10\n  background_color: \"#0000ff\"\n  z_index: 1\n"
      "- type: panel\n  width: 10\n  background_color: \"#ffff00\"\n  z_index: -1\n"
      "- type: panel\n  width: 10\n  background_color: \"#ff00ff\"\n"), 0);
    Frame();

    const auto quads = _renderer.Quads();
    ASSERT_EQ(quads.size(), 5u);

    ExpectColor(quads[0], 1, 1, 0, 1);

    // what is equally high keeps the order of the file
    ExpectColor(quads[1], 0, 1, 0, 1);
    ExpectColor(quads[2], 1, 0, 1, 1);

    ExpectColor(quads[3], 0, 0, 1, 1);
    ExpectColor(quads[4], 1, 0, 0, 1);

    // where an element is placed does not depend on when it is drawn
    EXPECT_FLOAT_EQ(quads[4].left, 0);
    EXPECT_FLOAT_EQ(quads[0].left, 30);
  }

  TEST_F(UiDrawingTest, DrawsTheFileThatWasShownLastOnTop)
  {
    ASSERT_GE(Show(
      "root:\n  type: panel\n  width: 10\n  height: 10\n  background_color: \"#ff0000\"\n", "ui/a.ui.yml"), 0);
    const int second = Show(
      "root:\n  type: panel\n  width: 10\n  height: 10\n  background_color: \"#00ff00\"\n", "ui/b.ui.yml");
    ASSERT_GE(Show(
      "root:\n  type: panel\n  width: 10\n  height: 10\n  background_color: \"#0000ff\"\n", "ui/c.ui.yml"), 0);

    Frame();

    auto quads = _renderer.Quads();
    ASSERT_EQ(quads.size(), 3u);
    ExpectColor(quads[0], 1, 0, 0, 1);
    ExpectColor(quads[1], 0, 1, 0, 1);
    ExpectColor(quads[2], 0, 0, 1, 1);

    _ui->Unload(second);
    Frame();

    quads = _renderer.Quads();
    ASSERT_EQ(quads.size(), 2u);
    ExpectColor(quads[0], 1, 0, 0, 1);
    ExpectColor(quads[1], 0, 0, 1, 1);
  }

  // states

  TEST_F(UiDrawingTest, DrawsAButtonInTheStateItIsIn)
  {
    ASSERT_GE(ShowUnderRoot(
      "- type: button\n"
      "  name: start\n"
      "  width: 100\n"
      "  height: 50\n"
      "  background_color: \"#000000\"\n"
      "  hover:\n"
      "    background_color: \"#ff0000\"\n"
      "  active:\n"
      "    background_color: \"#00ff00\"\n"
      "  focus:\n"
      "    outline_width: 0\n"
      "    border: \"2px solid #0000ff\"\n"), 0) << _logger->Messages(LogLevel::Error);

    Frame();
    ASSERT_EQ(_renderer.Quads().size(), 1u);
    ExpectColor(_renderer.Quads()[0], 0, 0, 0, 1);

    // in the frame the pointer arrives in
    PointAt(50, 25);
    Frame();
    ExpectColor(_renderer.Quads().at(0), 1, 0, 0, 1);

    _input.state.SetAction(neon::Action::Pointer_Primary);
    Frame();

    // held down, which gives it the focus as well
    const auto quads = _renderer.Quads();
    ASSERT_EQ(quads.size(), 5u);
    ExpectColor(quads[0], 0, 1, 0, 1);
    ExpectColor(quads[1], 0, 0, 1, 1);

    // the border is added to the box, and the box has grown by it
    ExpectQuad(quads[0], 0, 0, 136, 70);
  }

  // draw calls

  TEST_F(UiDrawingTest, DrawsBoxesAndTextOfOneFontInOneCall)
  {
    ASSERT_GE(ShowUnderRoot(
      "- type: panel\n"
      "  padding: 16\n"
      "  gap: 8\n"
      "  flex_direction: column\n"
      "  background_color: \"#101418cc\"\n"
      "  border: \"2px solid #4c566a\"\n"
      "  children:\n"
      "    - type: label\n"
      "      text: \"Health: 75\"\n"
      "    - type: bar\n"
      "      value: 3\n"
      "      max: 4\n"
      "    - type: button\n"
      "      text: Start\n"
      "    - type: button\n"
      "      text: Quit\n"
      "    - type: label\n"
      "      text: Paused\n"), 0) << _logger->Messages(LogLevel::Error);
    Frame();

    EXPECT_EQ(_renderer.batches.size(), 1u);
    EXPECT_EQ(_ui->GetDrawCalls(), 1u);
    EXPECT_GT(_renderer.Quads().size(), 30u);
    EXPECT_NE(_renderer.batches[0].texture, No_Texture);
  }

  TEST_F(UiDrawingTest, HandsEveryRectangleOverAsTwoTriangles)
  {
    ShowBox("background_color: \"#ff0000\"\nborder_width: 2\n");

    ASSERT_EQ(_renderer.batches.size(), 1u);
    const auto &batch = _renderer.batches[0];

    // five rectangles: the background and the four sides of the border
    ASSERT_EQ(batch.vertices.size(), 20u);
    ASSERT_EQ(batch.indices.size(), 30u);

    for (std::size_t quad = 0; quad < 5; quad++)
    {
      const auto first = static_cast<std::uint32_t>(quad * 4);
      const std::vector<std::uint32_t> expected = {first, first + 1, first + 2, first, first + 2, first + 3};
      const std::vector<std::uint32_t> drawn(
        batch.indices.begin() + static_cast<std::ptrdiff_t>(quad * 6),
        batch.indices.begin() + static_cast<std::ptrdiff_t>(quad * 6 + 6));

      EXPECT_EQ(drawn, expected);
    }

    // the corners of the background, from the left top one around
    EXPECT_FLOAT_EQ(batch.vertices[0].x, 100);
    EXPECT_FLOAT_EQ(batch.vertices[0].y, 100);
    EXPECT_FLOAT_EQ(batch.vertices[1].x, 300);
    EXPECT_FLOAT_EQ(batch.vertices[1].y, 100);
    EXPECT_FLOAT_EQ(batch.vertices[2].x, 300);
    EXPECT_FLOAT_EQ(batch.vertices[2].y, 200);
    EXPECT_FLOAT_EQ(batch.vertices[3].x, 100);
    EXPECT_FLOAT_EQ(batch.vertices[3].y, 200);

    EXPECT_FLOAT_EQ(batch.vertices[0].textured, 0);
    EXPECT_FLOAT_EQ(batch.translate_x, 0);
    EXPECT_FLOAT_EQ(batch.translate_y, 0);
  }

  TEST_F(UiDrawingTest, StartsANewCallWhereTheTextureChanges)
  {
    ASSERT_GE(ShowUnderRoot(
      "- type: label\n  text: a\n"
      "- type: image\n  src: assets://ui/heart.png\n"
      "- type: panel\n  width: 10\n  background_color: \"#ff0000\"\n"
      "- type: image\n  src: assets://ui/heart.png\n"
      "- type: image\n  src: assets://ui/star.png\n"
      "- type: label\n  text: b\n"
      "- type: label\n  text: c\n"), 0);
    Frame();

    // the text, the two hearts with the box between them, the star, and
    // the rest of the text
    ASSERT_EQ(_renderer.batches.size(), 4u);
    EXPECT_EQ(_ui->GetDrawCalls(), 4u);

    EXPECT_EQ(_renderer.batches[0].vertices.size(), 4u);
    EXPECT_EQ(_renderer.batches[1].vertices.size(), 12u);
    EXPECT_EQ(_renderer.batches[1].texture, _renderer.TextureOf("assets://ui/heart.png"));
    EXPECT_EQ(_renderer.batches[2].texture, _renderer.TextureOf("assets://ui/star.png"));
    EXPECT_EQ(_renderer.batches[3].vertices.size(), 8u);
    EXPECT_EQ(_renderer.batches[3].texture, _renderer.batches[0].texture);
  }

  TEST_F(UiDrawingTest, ACallOfBoxesAloneHasNoTexture)
  {
    ShowBox("background_color: \"#ff0000\"\n");

    ASSERT_EQ(_renderer.batches.size(), 1u);
    EXPECT_EQ(_renderer.batches[0].texture, No_Texture);
  }

  TEST_F(UiDrawingTest, SeveralFilesShareTheirCalls)
  {
    ASSERT_GE(Show("root:\n  type: label\n  text: a\n  background_color: \"#ff0000\"\n", "ui/a.ui.yml"), 0);
    ASSERT_GE(Show("root:\n  type: label\n  text: b\n  background_color: \"#00ff00\"\n", "ui/b.ui.yml"), 0);
    Frame();

    EXPECT_EQ(_renderer.batches.size(), 1u);
    EXPECT_EQ(_renderer.Quads().size(), 4u);
  }

  TEST_F(UiDrawingTest, DrawsTheSameInEveryFrame)
  {
    ASSERT_GE(ShowUnderRoot(
      "- type: button\n  text: Start\n  autofocus: true\n"
      "- type: label\n  text: \"Health: {health}\"\n"), 0);
    _ui->SetNumber("health", 75);

    Frame();
    const auto first = _renderer.Quads();

    Frame();
    const auto second = _renderer.Quads();

    ASSERT_EQ(first.size(), second.size());
    for (std::size_t i = 0; i < first.size(); i++)
    {
      EXPECT_FLOAT_EQ(first[i].left, second[i].left);
      EXPECT_FLOAT_EQ(first[i].top, second[i].top);
      EXPECT_FLOAT_EQ(first[i].right, second[i].right);
      EXPECT_FLOAT_EQ(first[i].bottom, second[i].bottom);
    }
  }

  // cleaning up

  TEST_F(UiDrawingTest, ReleasesItsTexturesWhenItIsCleanedUp)
  {
    ASSERT_GE(ShowUnderRoot(
      "- type: label\n  text: a\n"
      "- type: label\n  text: b\n  font_size: 24\n"
      "- type: image\n  src: assets://ui/heart.png\n"), 0);
    Frame();
    ASSERT_EQ(_renderer.TextureCount(), 3u);

    _ui->CleanUp();

    EXPECT_EQ(_renderer.TextureCount(), 0u);
    EXPECT_EQ(_renderer.destroyed, 3u);
    EXPECT_FALSE(_rasterizer.IsLoaded(0));
  }

  TEST_F(UiDrawingTest, KeepsItsTexturesWhenAFileIsTakenAway)
  {
    const int document = ShowUnderRoot("- type: label\n  text: a\n");
    Frame();

    _ui->Unload(document);
    Frame();

    // the next file that is shown draws with them
    EXPECT_EQ(_renderer.TextureCount(), 1u);
    EXPECT_TRUE(_renderer.batches.empty());

    ASSERT_GE(ShowUnderRoot("- type: label\n  text: b\n"), 0);
    Frame();
    EXPECT_EQ(_renderer.created, 1u);
  }
}
