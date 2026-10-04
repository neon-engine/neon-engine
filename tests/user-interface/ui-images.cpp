#include "ui-fixture.hpp"

#include <neon/testing/fake-images.hpp>

// Images as artists make them: files of pixels with smaller copies, images
// of shapes that are drawn at the size they have on the screen, variants
// for screens of several densities, and parts of an atlas. And where an
// image goes in its box.

namespace
{
  using neon::No_Texture;
  using neon::ShapeKind2D;
  using neon::TextureFilter2D;
  using neon::UiResources;
  using neon::testing::FakeImageDecoder;
  using neon::testing::FakeVectorImageRasterizer;
  using neon::testing::LogLevel;
  using neon::testing::RecordedQuad;
  using neon::testing::UiTest;
  using ::testing::ElementsAre;

  class UiImageTest : public UiTest
  {
  protected:
    FakeImageDecoder _decoder;
    FakeVectorImageRasterizer _vectors;

    void SetUp() override
    {
      UiTest::SetUp();
      UseDecoders();

      WriteAsset("ui/heart.png", FakeImageDecoder::AnImage(64, 64));
      WriteAsset("ui/wide.png", FakeImageDecoder::AnImage(200, 100));
      WriteAsset("ui/tile.png", FakeImageDecoder::AnImage(40, 20));
      WriteAsset("ui/frame.png", FakeImageDecoder::AnImage(24, 24));
      WriteAsset("ui/gem-1x.png", FakeImageDecoder::AnImage(32, 32));
      WriteAsset("ui/gem-2x.png", FakeImageDecoder::AnImage(64, 64));
      WriteAsset("ui/gem-4x.png", FakeImageDecoder::AnImage(128, 128));
      WriteAsset("ui/shield.svg", FakeVectorImageRasterizer::Shapes(24, 24));
      WriteAsset("ui/icons.png", FakeImageDecoder::AnImage(64, 32));
      WriteAsset(
        "ui/icons.atlas.yml",
        "atlas: icons\n"
        "image: assets://ui/icons.png\n"
        "regions:\n"
        "  - name: coin\n"
        "    x: 16\n"
        "    y: 0\n"
        "    width: 16\n"
        "    height: 16\n"
        "  - name: panel\n"
        "    x: 32\n"
        "    y: 8\n"
        "    width: 32\n"
        "    height: 24\n"
        "    slice: [4, 6]\n");
    }

    void UseDecoders()
    {
      _ui->SetImageDecoder(&_decoder);
      _ui->SetVectorImageRasterizer(&_vectors);
    }

    /// Starts again, with nothing loaded.
    void StartAgain()
    {
      _ui->CleanUp();
      Create({});
      UseDecoders();
      _logger->Clear();
    }

    void ShowImage(const std::string &properties)
    {
      ASSERT_GE(ShowUnderRoot(
        "- type: image\n"
        "  name: picture\n"
        "  position: absolute\n"
        "  left: 100\n"
        "  top: 100\n" + Indented(properties, "  ")), 0) << _logger->Messages(LogLevel::Error);
      Frame();
    }

    /// A box of 200 by 100 at 100, 100.
    void ShowBox(const std::string &properties)
    {
      ASSERT_GE(ShowUnderRoot(
        "- type: panel\n"
        "  name: box\n"
        "  position: absolute\n"
        "  left: 100\n"
        "  top: 100\n"
        "  width: 200\n"
        "  height: 100\n" + Indented(properties, "  ")), 0) << _logger->Messages(LogLevel::Error);
      Frame();
    }

    static void ExpectQuad(
      const RecordedQuad &quad,
      const float left,
      const float top,
      const float width,
      const float height)
    {
      EXPECT_NEAR(quad.left, left, 0.001f);
      EXPECT_NEAR(quad.top, top, 0.001f);
      EXPECT_NEAR(quad.Width(), width, 0.001f);
      EXPECT_NEAR(quad.Height(), height, 0.001f);
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

    [[nodiscard]] std::vector<std::string> ProblemsOf(const std::string &element)
    {
      _logger->Clear();
      EXPECT_EQ(ShowUnderRoot(element), -1);

      auto errors = Errors();
      if (!errors.empty()) { errors.pop_back(); }
      return errors;
    }
  };

  // files of pixels

  TEST_F(UiImageTest, AnImageHasSmallerCopies)
  {
    ShowImage("src: assets://ui/heart.png\nwidth: 32\nheight: 32\n");

    const auto quads = _renderer.Quads();
    ASSERT_EQ(quads.size(), 1u);
    ASSERT_NE(quads[0].texture, No_Texture);

    // so that an image that is drawn smaller than it is has no rough edges
    EXPECT_TRUE(_renderer.OptionsOf(quads[0].texture).has_smaller_copies);
    EXPECT_FALSE(_renderer.OptionsOf(quads[0].texture).repeats);

    EXPECT_EQ(_decoder.decoded, 1u);
    EXPECT_EQ(_renderer.loaded, 0u) << "the renderer reads no file";
    ExpectBox("picture", 100, 100, 32, 32);
  }

  TEST_F(UiImageTest, TheAtlasOfAFontHasNone)
  {
    ASSERT_GE(ShowUnderRoot("- type: label\n  text: a\n"), 0);
    Frame();

    // a glyph is drawn at the size it was made for, and what is next to it
    // in the atlas must not show
    EXPECT_FALSE(_renderer.OptionsOf(_renderer.Quads().at(0).texture).has_smaller_copies);
  }

  TEST_F(UiImageTest, WithoutADecoderTheRendererIsAskedForAnImage)
  {
    _ui->SetImageDecoder(nullptr);

    ShowImage("src: assets://ui/heart.png\n");

    EXPECT_EQ(_renderer.loaded, 1u);
    EXPECT_EQ(_decoder.decoded, 0u);
    EXPECT_EQ(_renderer.Quads().size(), 1u);
  }

  TEST_F(UiImageTest, ReadsAnImageOnce)
  {
    ASSERT_GE(ShowUnderRoot(
      "- type: image\n  src: assets://ui/heart.png\n"
      "- type: image\n  src: assets://ui/heart.png\n  width: 20\n  height: 20\n"
      "- type: panel\n  width: 10\n  height: 10\n  background_image: assets://ui/heart.png\n"), 0);

    Frame();
    Frame();
    Frame();

    EXPECT_EQ(_decoder.decoded, 1u);
    EXPECT_EQ(_renderer.created, 1u);
    EXPECT_EQ(_renderer.Quads().size(), 3u);
  }

  TEST_F(UiImageTest, SaysOnceWhichElementAsksForAnImageThatCannotBeUsed)
  {
    WriteAsset("ui/notes.png", "these are notes");

    ASSERT_GE(ShowUnderRoot(
      "- type: image\n  name: first\n  src: assets://ui/notes.png\n"
      "- type: image\n  name: second\n  src: assets://ui/notes.png\n"
      "- type: image\n  name: third\n  src: assets://ui/missing.png\n"), 0);

    Frame();
    Frame();
    Frame();

    // once for each file, with the element that asked first
    EXPECT_EQ(_logger->Count(LogLevel::Error), 2u) << _logger->Messages(LogLevel::Error);
    EXPECT_TRUE(_logger->Contains(
      LogLevel::Error,
      "The image assets://ui/notes.png cannot be used, image 'first' is drawn without it: it is no image of "
      "the tests"));
    EXPECT_TRUE(_logger->Contains(
      LogLevel::Error, "The image assets://ui/missing.png cannot be read, image 'third' is drawn without it"));

    EXPECT_EQ(_decoder.decoded, 1u);
    EXPECT_TRUE(_renderer.batches.empty());
  }

  // images of shapes

  TEST_F(UiImageTest, AnImageOfShapesIsDrawnAtTheSizeItHasOnTheScreen)
  {
    ShowImage("src: assets://ui/shield.svg\nwidth: 48\nheight: 48\n");

    ASSERT_EQ(_vectors.drawn.size(), 1u);
    EXPECT_EQ(_vectors.drawn[0], (std::pair{48, 48}));

    const auto quads = _renderer.Quads();
    ASSERT_EQ(quads.size(), 1u);
    ExpectQuad(quads[0], 100, 100, 48, 48);
    ExpectPart(quads[0], 0, 0, 1, 1);

    // a pixel of the texture is a pixel of the frame
    int width = 0;
    int height = 0;
    ASSERT_TRUE(_renderer.GetTextureSize(quads[0].texture, width, height));
    EXPECT_EQ(width, 48);
    EXPECT_EQ(height, 48);
  }

  TEST_F(UiImageTest, AnImageOfShapesIsDrawnLargerOnALargerFrame)
  {
    _renderer.SetResolution(3840, 2160);
    ShowImage("src: assets://ui/shield.svg\nwidth: 48\nheight: 48\n");

    // drawn again at the size, and not scaled from a smaller picture
    ASSERT_EQ(_vectors.drawn.size(), 1u);
    EXPECT_EQ(_vectors.drawn[0], (std::pair{96, 96}));
    ExpectQuad(_renderer.Quads().at(0), 200, 200, 96, 96);
  }

  TEST_F(UiImageTest, AnImageOfShapesIsNotDrawnAgainInEveryFrame)
  {
    ShowImage("src: assets://ui/shield.svg\nwidth: 48\nheight: 48\n");

    const int texture = _renderer.Quads().at(0).texture;

    for (int frame = 0; frame < 10; frame++) { Frame(); }

    EXPECT_EQ(_vectors.drawn.size(), 1u);
    EXPECT_EQ(_vectors.loaded, 1u);
    EXPECT_EQ(_renderer.created, 1u);
    EXPECT_EQ(_ui->GetResources().GetVectorPicturesDrawn(), 1u);
    EXPECT_EQ(_renderer.Quads().at(0).texture, texture) << "the picture that was drawn in the first frame";
  }

  TEST_F(UiImageTest, AnImageOfShapesIsKeptAtEverySizeItIsShownAt)
  {
    ASSERT_GE(ShowUnderRoot(
      "- type: image\n  src: assets://ui/shield.svg\n  width: 32\n  height: 32\n"
      "- type: image\n  src: assets://ui/shield.svg\n  width: 128\n  height: 128\n"
      "- type: image\n  src: assets://ui/shield.svg\n  width: 32\n  height: 32\n"), 0);

    Frame();
    Frame();
    Frame();

    // read once, and drawn once for each size
    EXPECT_EQ(_vectors.loaded, 1u);
    ASSERT_EQ(_vectors.drawn.size(), 2u);
    EXPECT_EQ(_vectors.drawn[0], (std::pair{32, 32}));
    EXPECT_EQ(_vectors.drawn[1], (std::pair{128, 128}));

    const auto quads = _renderer.Quads();
    ASSERT_EQ(quads.size(), 3u);
    EXPECT_NE(quads[0].texture, quads[1].texture);
    EXPECT_EQ(quads[0].texture, quads[2].texture);
  }

  TEST_F(UiImageTest, AnImageOfShapesWithoutASizeIsAsLargeAsItSays)
  {
    ShowImage("src: assets://ui/shield.svg\n");

    ExpectBox("picture", 100, 100, 24, 24);
    EXPECT_EQ(_vectors.drawn.back(), (std::pair{24, 24}));
  }

  TEST_F(UiImageTest, AnImageOfShapesThatChangesItsSizeIsDrawnAgainOnceItHasSettled)
  {
    ASSERT_GE(ShowUnderRoot(
      "- type: button\n"
      "  name: start\n"
      "  position: absolute\n"
      "  left: 0\n"
      "  top: 0\n"
      "  padding: 0\n"
      "  children:\n"
      "    - type: image\n"
      "      name: picture\n"
      "      src: assets://ui/shield.svg\n"
      "      width: \"{size}\"\n"
      "      height: 40\n"), -1) << "a size does not follow a value";

    // the size of a button changes with its state
    StartAgain();
    ASSERT_GE(ShowUnderRoot(
      "- type: image\n"
      "  name: picture\n"
      "  position: absolute\n"
      "  left: 0\n"
      "  top: 0\n"
      "  pointer_events: auto\n"
      "  src: assets://ui/shield.svg\n"
      "  width: 40\n"
      "  height: 40\n"
      "  hover:\n"
      "    width: 80\n"
      "    height: 80\n"), 0) << _logger->Messages(LogLevel::Error);

    Frame();
    ASSERT_EQ(_vectors.drawn.size(), 1u);
    const int small = _renderer.Quads().at(0).texture;

    PointAt(10, 10);
    Frame();

    // larger at once, with the picture there is
    ExpectQuad(_renderer.Quads().at(0), 0, 0, 80, 80);
    EXPECT_EQ(_renderer.Quads().at(0).texture, small);
    EXPECT_EQ(_vectors.drawn.size(), 1u);

    // and with one of its own once the size has been the same for a few
    // frames
    for (int frame = 0; frame < UiResources::kSettle_Frames; frame++) { Frame(); }

    ASSERT_EQ(_vectors.drawn.size(), 2u);
    EXPECT_EQ(_vectors.drawn[1], (std::pair{80, 80}));
    EXPECT_NE(_renderer.Quads().at(0).texture, small);

    for (int frame = 0; frame < 10; frame++) { Frame(); }
    EXPECT_EQ(_vectors.drawn.size(), 2u);
  }

  TEST_F(UiImageTest, KeepsAFewSizesOfAnImageOfShapes)
  {
    std::string elements;
    for (int i = 0; i < 7; i++)
    {
      const std::string size = std::to_string(16 + 8 * i);
      elements += "- type: image\n  src: assets://ui/shield.svg\n  width: " + size + "\n  height: " + size + "\n"
                  "  hidden: \"{hide_" + std::to_string(i) + "}\"\n";
      _ui->SetFlag("hide_" + std::to_string(i), i != 0);
    }

    ASSERT_GE(ShowUnderRoot(elements), 0) << _logger->Messages(LogLevel::Error);

    // one size after the other, each for long enough to be drawn
    for (int i = 0; i < 7; i++)
    {
      for (int other = 0; other < 7; other++) { _ui->SetFlag("hide_" + std::to_string(other), other != i); }
      for (int frame = 0; frame <= UiResources::kSettle_Frames; frame++) { Frame(); }

      EXPECT_LE(_renderer.TextureCount(), UiResources::kMax_Vector_Pictures) << "at size " << i;
    }

    EXPECT_EQ(_vectors.drawn.size(), 7u);
    EXPECT_EQ(_renderer.destroyed, 3u) << "what was not drawn with for longest made room";
    EXPECT_EQ(_renderer.TextureCount(), UiResources::kMax_Vector_Pictures);
  }

  TEST_F(UiImageTest, SaysOnceThatAnImageOfShapesCannotBeDrawn)
  {
    _vectors.refuses = true;

    ShowImage("src: assets://ui/shield.svg\nwidth: 48\nheight: 48\n");
    Frame();
    Frame();
    Frame();
    Frame();

    EXPECT_EQ(Errors().size(), 1u) << _logger->Messages(LogLevel::Error);
    EXPECT_TRUE(_logger->Contains(
      LogLevel::Error,
      "The image assets://ui/shield.svg cannot be drawn at 48 by 48 pixels, image 'picture' is drawn with "
      "what there is of it"));
    EXPECT_TRUE(_renderer.batches.empty());
  }

  TEST_F(UiImageTest, SaysOnceThatAnImageOfShapesCannotBeRead)
  {
    WriteAsset("ui/broken.svg", "no shapes");

    ShowImage("src: assets://ui/broken.svg\nwidth: 48\nheight: 48\n");
    Frame();
    Frame();

    EXPECT_EQ(Errors().size(), 1u) << _logger->Messages(LogLevel::Error);
    EXPECT_TRUE(_logger->Contains(
      LogLevel::Error,
      "The image assets://ui/broken.svg cannot be used, image 'picture' is drawn without it: it is no image "
      "of shapes of the tests"));
    EXPECT_EQ(_vectors.loaded, 1u);
  }

  TEST_F(UiImageTest, SaysOnceThatNothingDrawsImagesOfShapes)
  {
    _ui->SetVectorImageRasterizer(nullptr);

    ShowImage("src: assets://ui/shield.svg\nwidth: 48\nheight: 48\n");
    Frame();
    Frame();

    EXPECT_EQ(Errors().size(), 1u) << _logger->Messages(LogLevel::Error);
    EXPECT_TRUE(_logger->Contains(
      LogLevel::Error,
      "The image assets://ui/shield.svg is made of shapes, which nothing here draws. image 'picture' is "
      "drawn without it"));
  }

  TEST_F(UiImageTest, ReleasesThePicturesOfAnImageOfShapes)
  {
    ShowImage("src: assets://ui/shield.svg\nwidth: 48\nheight: 48\n");
    ASSERT_EQ(_renderer.TextureCount(), 1u);

    _ui->CleanUp();

    EXPECT_EQ(_renderer.TextureCount(), 0u);
  }

  // screens of several densities

  TEST_F(UiImageTest, TakesTheImageThatWasMadeForTheScreen)
  {
    const std::string image =
      "src:\n"
      "  - { src: assets://ui/gem-1x.png, scale: 1 }\n"
      "  - { src: assets://ui/gem-2x.png, scale: 2 }\n"
      "  - { src: assets://ui/gem-4x.png, scale: 4 }\n";

    ShowImage(image);

    int width = 0;
    int height = 0;
    ASSERT_TRUE(_renderer.GetTextureSize(_renderer.Quads().at(0).texture, width, height));
    EXPECT_EQ(width, 32);
    ExpectBox("picture", 100, 100, 32, 32);
    ExpectQuad(_renderer.Quads().at(0), 100, 100, 32, 32);
    EXPECT_EQ(_decoder.decoded, 1u) << "the others are not read";

    // at twice the size of the frame
    StartAgain();
    _renderer.SetResolution(3840, 2160);
    ShowImage(image);

    ASSERT_TRUE(_renderer.GetTextureSize(_renderer.Quads().at(0).texture, width, height));
    EXPECT_EQ(width, 64);

    // as large in units of the file as the first one, and a pixel of the
    // image is a pixel of the frame
    ExpectBox("picture", 100, 100, 32, 32);
    ExpectQuad(_renderer.Quads().at(0), 200, 200, 64, 64);

    // between two, the denser one
    StartAgain();
    _renderer.SetResolution(5120, 2880);
    ShowImage(image);

    ASSERT_TRUE(_renderer.GetTextureSize(_renderer.Quads().at(0).texture, width, height));
    EXPECT_EQ(width, 128);
    ExpectBox("picture", 100, 100, 32, 32);
  }

  TEST_F(UiImageTest, OneImageIsMadeForAScaleOfOne)
  {
    _renderer.SetResolution(3840, 2160);
    ShowImage("src: assets://ui/gem-1x.png\n");

    ExpectBox("picture", 100, 100, 32, 32);
    ExpectQuad(_renderer.Quads().at(0), 200, 200, 64, 64);
  }

  TEST_F(UiImageTest, AnImageThatIsToldAnotherPathShowsThatImage)
  {
    ShowImage("src: assets://ui/heart.png\n");
    ExpectBox("picture", 100, 100, 64, 64);
    const auto before = _renderer.Quads().at(0).texture;

    ASSERT_TRUE(_ui->SetField(_ui->FindByName("picture"), "src", std::string("assets://ui/wide.png")));
    Frame();

    // the other image, with the size it has
    ExpectBox("picture", 100, 100, 200, 100);
    ASSERT_EQ(_renderer.Quads().size(), 1u);
    EXPECT_NE(_renderer.Quads().at(0).texture, before);
    int width = 0;
    int height = 0;
    ASSERT_TRUE(_renderer.GetTextureSize(_renderer.Quads().at(0).texture, width, height));
    EXPECT_EQ(width, 200);

    // and one of a list of images is told a single one
    StartAgain();
    ShowImage(
      "src:\n"
      "  - { src: assets://ui/gem-1x.png, scale: 1 }\n"
      "  - { src: assets://ui/gem-2x.png, scale: 2 }\n");
    ASSERT_TRUE(_ui->SetField(_ui->FindByName("picture"), "src", std::string("assets://ui/tile.png")));
    Frame();
    ExpectBox("picture", 100, 100, 40, 20);
  }

  TEST_F(UiImageTest, SaysWhatIsWrongWithAListOfImages)
  {
    EXPECT_THAT(
      ProblemsOf(
        "- type: image\n"
        "  name: picture\n"
        "  src:\n"
        "    - { scale: 2 }\n"
        "    - { src: assets://ui/gem-1x.png, scale: 0 }\n"
        "    - { src: assets://ui/gem-2x.png, density: 2 }\n"),
      ElementsAre(
        "assets://ui/test.ui.yml:10: image 1 of image 'picture' has no 'src', where the virtual path of an "
        "image was expected",
        "assets://ui/test.ui.yml:11: 'scale' of image 2 of image 'picture' is 0, where a number above 0 was "
        "expected",
        "assets://ui/test.ui.yml:12: 'density' is not known to image 3 of image 'picture'. Known are: src, "
        "scale"));

    EXPECT_THAT(
      ProblemsOf("- type: image\n  name: picture\n  src: []\n"),
      ElementsAre(
        "assets://ui/test.ui.yml:9: 'src' of image 'picture' is a list without an image, where at least one "
        "such as { src: assets://ui/heart.png, scale: 1 } was expected"));
  }

  // parts of an atlas

  TEST_F(UiImageTest, DrawsAPartOfAnAtlasByItsName)
  {
    ShowImage("src: assets://ui/icons.atlas.yml#coin\n");

    // as large as the part, which is 16 by 16 of an image of 64 by 32
    ExpectBox("picture", 100, 100, 16, 16);

    const auto quads = _renderer.Quads();
    ASSERT_EQ(quads.size(), 1u);
    ExpectQuad(quads[0], 100, 100, 16, 16);
    ExpectPart(quads[0], 0.25f, 0.0f, 0.5f, 0.5f);

    int width = 0;
    int height = 0;
    ASSERT_TRUE(_renderer.GetTextureSize(quads[0].texture, width, height));
    EXPECT_EQ(width, 64) << "the image of the atlas";
  }

  TEST_F(UiImageTest, ThePartsOfAnAtlasShareOneTextureAndOneCall)
  {
    ASSERT_GE(ShowUnderRoot(
      "- type: image\n  src: assets://ui/icons.atlas.yml#coin\n"
      "- type: image\n  src: assets://ui/icons.atlas.yml#panel\n"
      "- type: image\n  src: assets://ui/icons.atlas.yml#coin\n"), 0);

    Frame();
    Frame();

    EXPECT_EQ(_renderer.batches.size(), 1u);
    EXPECT_EQ(_renderer.created, 1u);
    EXPECT_EQ(_decoder.decoded, 1u);

    const auto quads = _renderer.Quads();
    ASSERT_EQ(quads.size(), 3u);
    ExpectPart(quads[1], 0.5f, 0.25f, 1.0f, 1.0f);
  }

  TEST_F(UiImageTest, FitsAPartOfAnAtlasIntoItsBox)
  {
    // the part is 32 by 24, in a box of 100 by 100
    ShowImage("src: assets://ui/icons.atlas.yml#panel\nwidth: 100\nheight: 100\nobject_fit: cover\n");

    // as high as the box and wider: three quarters of its width are
    // shown, from its middle
    const auto quads = _renderer.Quads();
    ASSERT_EQ(quads.size(), 1u);
    ExpectQuad(quads[0], 100, 100, 100, 100);
    ExpectPart(quads[0], 0.5f + 0.0625f, 0.25f, 1.0f - 0.0625f, 1.0f);
  }

  TEST_F(UiImageTest, APartOfAnAtlasSaysHowItIsDrawnInNineParts)
  {
    ShowBox("border_image_source: assets://ui/icons.atlas.yml#panel\n");

    const auto quads = _renderer.Quads();
    ASSERT_EQ(quads.size(), 9u);

    // 4 at the top and the bottom, 6 at the sides, in pixels of the part
    ExpectQuad(quads[0], 100, 100, 6, 4);
    ExpectPart(quads[0], 0.5f, 0.25f, 0.5f + 6.0f / 64.0f, 0.25f + 4.0f / 32.0f);

    ExpectQuad(quads[4], 106, 104, 188, 92);
    ExpectQuad(quads[8], 294, 196, 6, 4);
    ExpectPart(quads[8], 1.0f - 6.0f / 64.0f, 1.0f - 4.0f / 32.0f, 1.0f, 1.0f);
  }

  TEST_F(UiImageTest, WhatTheFileSaysAboutNinePartsWinsOverTheAtlas)
  {
    ShowBox("border_image_source: assets://ui/icons.atlas.yml#panel\nborder_image_slice: 8\n");

    ExpectQuad(_renderer.Quads().at(0), 100, 100, 8, 8);
  }

  TEST_F(UiImageTest, SaysOnceThatAnAtlasHasNoSuchPart)
  {
    ASSERT_GE(ShowUnderRoot(
      "- type: image\n  name: first\n  src: assets://ui/icons.atlas.yml#gem\n"
      "- type: image\n  name: second\n  src: assets://ui/icons.atlas.yml#gem\n"), 0);

    Frame();
    Frame();

    EXPECT_EQ(Errors().size(), 1u) << _logger->Messages(LogLevel::Error);
    EXPECT_TRUE(_logger->Contains(
      LogLevel::Error,
      "The atlas assets://ui/icons.atlas.yml has no region 'gem', image 'first' is drawn without it. Known "
      "are: coin, panel"));
  }

  TEST_F(UiImageTest, SaysWhatIsWrongWithAnAtlas)
  {
    WriteAsset(
      "ui/wrong.atlas.yml",
      "atlas: wrong\n"
      "version: 2\n"
      "image: assets://ui/icons.png\n"
      "scale: 0\n"
      "size: 4\n"
      "regions:\n"
      "  - name: a\n"
      "    x: 0\n"
      "    y: 0\n"
      "    width: 16\n"
      "  - x: 0\n"
      "    y: 0\n"
      "    width: 8\n"
      "    height: 8\n"
      "  - name: b\n"
      "    x: -4\n"
      "    y: 0\n"
      "    width: 8\n"
      "    height: 8\n"
      "    slice: wide\n"
      "    colour: red\n");

    ShowImage("src: assets://ui/wrong.atlas.yml#a\n");
    Frame();
    Frame();

    EXPECT_THAT(
      Errors(),
      ElementsAre(
        "assets://ui/wrong.atlas.yml:2: the atlas has version 2, and this engine reads up to version 1",
        "assets://ui/wrong.atlas.yml:4: 'scale' of the atlas is 0, where a number above 0 was expected",
        "assets://ui/wrong.atlas.yml:7: region 'a' has no 'height', where a number of pixels was expected",
        "assets://ui/wrong.atlas.yml:11: region 2 has no 'name', where what it is asked for by was expected",
        "assets://ui/wrong.atlas.yml:15: region 'b' is at -4, 0 with a size of 8 by 8, where a place that is "
        "not below 0 and a size above 0 were expected",
        "assets://ui/wrong.atlas.yml:20: 'slice' of region 'b' is text, where one to four numbers of pixels "
        "that are not below 0 were expected: top, right, bottom, left",
        "assets://ui/wrong.atlas.yml:21: 'colour' is not known to region 'b'. Known are: name, x, y, width, "
        "height, slice",
        "assets://ui/wrong.atlas.yml:5: 'size' is not known to the atlas. Known are: atlas, version, image, "
        "scale, regions",
        "The atlas assets://ui/wrong.atlas.yml has 8 problems and is not used"));

    EXPECT_TRUE(_renderer.batches.empty());
  }

  TEST_F(UiImageTest, SaysWhenAPartReachesOutOfItsImage)
  {
    WriteAsset(
      "ui/large.atlas.yml",
      "image: assets://ui/icons.png\n"
      "regions:\n"
      "  - name: a\n"
      "    x: 48\n"
      "    y: 16\n"
      "    width: 32\n"
      "    height: 16\n");

    ShowImage("src: assets://ui/large.atlas.yml#a\n");

    EXPECT_THAT(
      Errors(),
      ElementsAre(
        "assets://ui/large.atlas.yml:3: region 'a' reaches to 80, 32, and the image assets://ui/icons.png is "
        "64 by 32",
        "The atlas assets://ui/large.atlas.yml has 1 problem and is not used"));
  }

  TEST_F(UiImageTest, AnAtlasForADenserScreenHasSmallerParts)
  {
    WriteAsset(
      "ui/dense.atlas.yml",
      "image: assets://ui/icons.png\n"
      "scale: 2\n"
      "regions:\n"
      "  - name: a\n"
      "    x: 0\n"
      "    y: 0\n"
      "    width: 32\n"
      "    height: 16\n");

    ShowImage("src: assets://ui/dense.atlas.yml#a\n");

    ExpectBox("picture", 100, 100, 16, 8);
  }

  // where an image goes in its box

  TEST_F(UiImageTest, PlacesAnImageThatKeepsItsShape)
  {
    // an image of 200 by 100 in a box of 100 by 100: 100 by 50
    ShowImage(
      "src: assets://ui/wide.png\nwidth: 100\nheight: 100\nobject_fit: contain\nobject_position: \"center top\"\n");
    ExpectQuad(_renderer.Quads().at(0), 100, 100, 100, 50);

    StartAgain();
    ShowImage(
      "src: assets://ui/wide.png\nwidth: 100\nheight: 100\nobject_fit: contain\nobject_position: bottom\n");
    ExpectQuad(_renderer.Quads().at(0), 100, 150, 100, 50);

    StartAgain();
    ShowImage(
      "src: assets://ui/wide.png\nwidth: 100\nheight: 100\nobject_fit: contain\nobject_position: \"0 10px\"\n");
    ExpectQuad(_renderer.Quads().at(0), 100, 110, 100, 50);
  }

  TEST_F(UiImageTest, ChoosesWhatOfAnImageThatCoversItsBoxIsShown)
  {
    ShowImage(
      "src: assets://ui/wide.png\nwidth: 100\nheight: 100\nobject_fit: cover\nobject_position: left\n");
    ExpectPart(_renderer.Quads().at(0), 0, 0, 0.5f, 1);

    StartAgain();
    ShowImage(
      "src: assets://ui/wide.png\nwidth: 100\nheight: 100\nobject_fit: cover\nobject_position: \"100% 0\"\n");
    ExpectPart(_renderer.Quads().at(0), 0.5f, 0, 1, 1);
  }

  TEST_F(UiImageTest, DrawsAnImageAsLargeAsItIs)
  {
    // 64 by 64 in a box of 200 by 100, in its middle
    ShowImage("src: assets://ui/heart.png\nwidth: 200\nheight: 100\nobject_fit: none\n");

    ExpectQuad(_renderer.Quads().at(0), 168, 118, 64, 64);
    ExpectPart(_renderer.Quads().at(0), 0, 0, 1, 1);
  }

  TEST_F(UiImageTest, CutsOffAnImageThatIsLargerThanItsBox)
  {
    // 200 by 100 in a box of 100 by 50: the middle of it
    ShowImage("src: assets://ui/wide.png\nwidth: 100\nheight: 50\nobject_fit: none\n");

    ExpectQuad(_renderer.Quads().at(0), 100, 100, 100, 50);
    ExpectPart(_renderer.Quads().at(0), 0.25f, 0.25f, 0.75f, 0.75f);

    StartAgain();
    ShowImage(
      "src: assets://ui/wide.png\nwidth: 100\nheight: 50\nobject_fit: none\nobject_position: \"right bottom\"\n");
    ExpectPart(_renderer.Quads().at(0), 0.5f, 0.5f, 1, 1);
  }

  TEST_F(UiImageTest, MakesAnImageSmallerAndNeverLarger)
  {
    ShowImage("src: assets://ui/heart.png\nwidth: 200\nheight: 100\nobject_fit: scale-down\n");
    ExpectQuad(_renderer.Quads().at(0), 168, 118, 64, 64);

    StartAgain();
    ShowImage("src: assets://ui/wide.png\nwidth: 100\nheight: 100\nobject_fit: scale-down\n");
    ExpectQuad(_renderer.Quads().at(0), 100, 125, 100, 50);
  }

  TEST_F(UiImageTest, CutsAnImageOffAtTheRoundCornersOfItsElement)
  {
    ShowImage("src: assets://ui/heart.png\nwidth: 64\nheight: 64\nborder_radius: 16\n");

    ASSERT_EQ(_renderer.batches.size(), 1u);
    ASSERT_EQ(_renderer.batches[0].shapes.size(), 1u);

    const auto &shape = _renderer.batches[0].shapes[0];
    EXPECT_EQ(static_cast<int>(shape.box[2]), static_cast<int>(ShapeKind2D::Fill));
    EXPECT_FLOAT_EQ(shape.box[0], 32);
    EXPECT_FLOAT_EQ(shape.radii[0], 16);

    EXPECT_TRUE(_renderer.Quads().at(0).textured);
  }

  // pixel by pixel

  TEST_F(UiImageTest, DrawsAnImagePixelByPixel)
  {
    ASSERT_GE(ShowUnderRoot(
      "- type: image\n  src: assets://ui/heart.png\n"
      "- type: image\n  src: assets://ui/heart.png\n  image_rendering: pixelated\n"
      "- type: image\n  src: assets://ui/heart.png\n  image_rendering: auto\n"), 0);
    Frame();

    // a call of its own, since a call reads its texture one way
    ASSERT_EQ(_renderer.batches.size(), 3u);
    EXPECT_EQ(_renderer.batches[0].filter, TextureFilter2D::Smooth);
    EXPECT_EQ(_renderer.batches[1].filter, TextureFilter2D::Pixelated);
    EXPECT_EQ(_renderer.batches[2].filter, TextureFilter2D::Smooth);

    EXPECT_EQ(_renderer.batches[0].texture, _renderer.batches[1].texture) << "of the one texture";
    EXPECT_EQ(_renderer.created, 1u);
  }

  TEST_F(UiImageTest, ABackgroundAndABorderAreDrawnPixelByPixelAsWell)
  {
    ShowBox(
      "background_image: assets://ui/heart.png\n"
      "border_image_source: assets://ui/frame.png\n"
      "border_image_slice: 8\n"
      "image_rendering: pixelated\n");

    for (const auto &batch : _renderer.batches) { EXPECT_EQ(batch.filter, TextureFilter2D::Pixelated); }
  }

  // backgrounds

  TEST_F(UiImageTest, ABackgroundIsStretchedOverTheBoxUnlessItIsToldOtherwise)
  {
    ShowBox("background_image: assets://ui/tile.png\n");

    const auto quads = _renderer.Quads();
    ASSERT_EQ(quads.size(), 1u);
    ExpectQuad(quads[0], 100, 100, 200, 100);
    ExpectPart(quads[0], 0, 0, 1, 1);
  }

  TEST_F(UiImageTest, DrawsABackgroundAgainAndAgainAtItsOwnSize)
  {
    // 40 by 20 over 200 by 100: five across and five down
    ShowBox("background_image: assets://ui/tile.png\nbackground_size: auto\n");

    const auto quads = _renderer.Quads();
    ASSERT_EQ(quads.size(), 25u);

    ExpectQuad(quads[0], 100, 100, 40, 20);
    ExpectQuad(quads[1], 140, 100, 40, 20);
    ExpectQuad(quads[5], 100, 120, 40, 20);
    ExpectQuad(quads[24], 260, 180, 40, 20);

    for (const auto &quad : quads) { ExpectPart(quad, 0, 0, 1, 1); }
    EXPECT_EQ(_renderer.batches.size(), 1u);
  }

  TEST_F(UiImageTest, DrawsABackgroundOnce)
  {
    ShowBox(
      "background_image: assets://ui/tile.png\n"
      "background_size: auto\n"
      "background_repeat: no-repeat\n"
      "background_position: \"right bottom\"\n");

    const auto quads = _renderer.Quads();
    ASSERT_EQ(quads.size(), 1u);
    ExpectQuad(quads[0], 260, 180, 40, 20);
  }

  TEST_F(UiImageTest, DrawsABackgroundAlongOneSide)
  {
    ShowBox(
      "background_image: assets://ui/tile.png\n"
      "background_size: auto\n"
      "background_repeat: repeat-x\n"
      "background_position: \"0 50%\"\n");

    const auto quads = _renderer.Quads();
    ASSERT_EQ(quads.size(), 5u);
    for (const auto &quad : quads) { EXPECT_FLOAT_EQ(quad.top, 140); }

    StartAgain();
    ShowBox(
      "background_image: assets://ui/tile.png\n"
      "background_size: auto\n"
      "background_repeat: repeat-y\n"
      "background_position: center\n");

    const auto down = _renderer.Quads();
    ASSERT_EQ(down.size(), 5u);
    for (const auto &quad : down) { EXPECT_FLOAT_EQ(quad.left, 180); }
  }

  TEST_F(UiImageTest, CutsOffTheTilesThatReachOutOfTheBox)
  {
    // from 20 left of the box and 10 above it
    ShowBox(
      "background_image: assets://ui/tile.png\n"
      "background_size: auto\n"
      "background_position: \"-20px -10px\"\n");

    const auto quads = _renderer.Quads();
    ASSERT_EQ(quads.size(), 36u);

    // the right and the lower half of the first
    ExpectQuad(quads[0], 100, 100, 20, 10);
    ExpectPart(quads[0], 0.5f, 0.5f, 1, 1);

    // and the left and the upper half of the last
    ExpectQuad(quads[35], 280, 190, 20, 10);
    ExpectPart(quads[35], 0, 0, 0.5f, 0.5f);

    for (const auto &quad : quads)
    {
      EXPECT_GE(quad.left, 100);
      EXPECT_LE(quad.right, 300);
      EXPECT_GE(quad.top, 100);
      EXPECT_LE(quad.bottom, 200);
    }
  }

  TEST_F(UiImageTest, GivesABackgroundASize)
  {
    ShowBox(
      "background_image: assets://ui/tile.png\n"
      "background_size: \"100 50%\"\n"
      "background_repeat: no-repeat\n");
    ExpectQuad(_renderer.Quads().at(0), 100, 100, 100, 50);

    // a side that is left out follows the other one
    StartAgain();
    ShowBox(
      "background_image: assets://ui/tile.png\n"
      "background_size: 100\n"
      "background_repeat: no-repeat\n");
    ExpectQuad(_renderer.Quads().at(0), 100, 100, 100, 50);

    StartAgain();
    ShowBox(
      "background_image: assets://ui/tile.png\n"
      "background_size: \"auto 80\"\n"
      "background_repeat: no-repeat\n");
    ExpectQuad(_renderer.Quads().at(0), 100, 100, 160, 80);
  }

  TEST_F(UiImageTest, ABackgroundCoversItsBoxOrFitsIntoIt)
  {
    // the image is twice as wide as high, as the box is, so both fill it.
    // In a box that is as wide as high they differ
    ASSERT_GE(ShowUnderRoot(
      "- type: panel\n"
      "  position: absolute\n"
      "  left: 100\n"
      "  top: 100\n"
      "  width: 100\n"
      "  height: 100\n"
      "  background_image: assets://ui/tile.png\n"
      "  background_size: contain\n"
      "  background_repeat: no-repeat\n"
      "  background_position: center\n"
      "- type: panel\n"
      "  position: absolute\n"
      "  left: 300\n"
      "  top: 100\n"
      "  width: 100\n"
      "  height: 100\n"
      "  background_image: assets://ui/tile.png\n"
      "  background_size: cover\n"
      "  background_repeat: no-repeat\n"
      "  background_position: center\n"), 0);
    Frame();

    const auto quads = _renderer.Quads();
    ASSERT_EQ(quads.size(), 2u);

    ExpectQuad(quads[0], 100, 125, 100, 50);
    ExpectPart(quads[0], 0, 0, 1, 1);

    // 200 by 100, of which the middle is shown
    ExpectQuad(quads[1], 300, 100, 100, 100);
    ExpectPart(quads[1], 0.25f, 0, 0.75f, 1);
  }

  TEST_F(UiImageTest, ABackgroundThatIsAPartOfAnAtlasIsDrawnAgainAndAgainAsWell)
  {
    ShowBox(
      "background_image: assets://ui/icons.atlas.yml#coin\n"
      "background_size: \"50 50\"\n");

    const auto quads = _renderer.Quads();
    ASSERT_EQ(quads.size(), 8u);

    // every tile shows the part, and nothing of what is next to it
    for (const auto &quad : quads) { ExpectPart(quad, 0.25f, 0, 0.5f, 0.5f); }
  }

  TEST_F(UiImageTest, StretchesABackgroundThatWouldBeDrawnTooOften)
  {
    ASSERT_GE(ShowUnderRoot(
      "- type: panel\n"
      "  name: floor\n"
      "  width: 1900\n"
      "  height: 1000\n"
      "  background_image: assets://ui/tile.png\n"
      "  background_size: \"4 4\"\n"), 0);

    Frame();
    Frame();
    Frame();

    EXPECT_EQ(_renderer.Quads().size(), 1u);
    EXPECT_EQ(_logger->Count(LogLevel::Warn), 1u) << _logger->Messages(LogLevel::Warn);
    EXPECT_TRUE(_logger->Contains(
      LogLevel::Warn,
      "'background_image' of panel 'floor' would be drawn more than 4096 times, and is stretched over the "
      "element in place of that"));
  }

  TEST_F(UiImageTest, ABackgroundOfShapesIsDrawnAtTheSizeOfItsBox)
  {
    ShowBox("background_image: assets://ui/shield.svg\n");

    ASSERT_EQ(_vectors.drawn.size(), 1u);
    EXPECT_EQ(_vectors.drawn[0], (std::pair{200, 100}));
  }

  // an image in nine parts

  TEST_F(UiImageTest, DrawsTheEdgesOfABorderImageAgainAndAgain)
  {
    // corners of 8 in an image of 24, which leaves edges of 8. Drawn with
    // corners of 8, an edge is 8 long: 23 along 184 and 10.5 along 84
    ShowBox(
      "border_image_source: assets://ui/frame.png\n"
      "border_image_slice: 8\n"
      "border_image_repeat: repeat\n");

    const auto quads = _renderer.Quads();

    // four corners, 2 by 23 and 2 by 11 for the edges, 23 by 11 for the
    // middle
    ASSERT_EQ(quads.size(), 4u + 46u + 22u + 253u);

    // the corner at the left top, and the first tile of the top edge
    ExpectQuad(quads[0], 100, 100, 8, 8);
    ExpectQuad(quads[1], 108, 100, 8, 8);
    ExpectPart(quads[1], 8.0f / 24.0f, 0, 16.0f / 24.0f, 8.0f / 24.0f);

    for (const auto &quad : quads)
    {
      EXPECT_LE(quad.Width(), 8.001f);
      EXPECT_LE(quad.Height(), 8.001f);
      EXPECT_GE(quad.left, 100);
      EXPECT_LE(quad.right, 300);
      EXPECT_GE(quad.top, 100);
      EXPECT_LE(quad.bottom, 200);
    }
  }

  TEST_F(UiImageTest, TilesAreLaidOutFromTheMiddleOfASide)
  {
    ASSERT_GE(ShowUnderRoot(
      "- type: panel\n"
      "  position: absolute\n"
      "  left: 100\n"
      "  top: 100\n"
      "  width: 36\n"
      "  height: 16\n"
      "  border_image_source: assets://ui/frame.png\n"
      "  border_image_slice: 8\n"
      "  border_image_repeat: repeat\n"), 0);
    Frame();

    // The top edge is 20 long, which takes three tiles of 8, of which the
    // first and the last are cut off by 2.
    const auto quads = _renderer.Quads();
    ASSERT_GE(quads.size(), 5u);

    ExpectQuad(quads[1], 108, 100, 6, 8);
    ExpectPart(quads[1], 10.0f / 24.0f, 0, 16.0f / 24.0f, 8.0f / 24.0f);

    ExpectQuad(quads[2], 114, 100, 8, 8);

    ExpectQuad(quads[3], 122, 100, 6, 8);
    ExpectPart(quads[3], 8.0f / 24.0f, 0, 14.0f / 24.0f, 8.0f / 24.0f);
  }

  TEST_F(UiImageTest, MakesTheTilesOfABorderImageFitAWholeNumberOfTimes)
  {
    ASSERT_GE(ShowUnderRoot(
      "- type: panel\n"
      "  position: absolute\n"
      "  left: 100\n"
      "  top: 100\n"
      "  width: 36\n"
      "  height: 16\n"
      "  border_image_source: assets://ui/frame.png\n"
      "  border_image_slice: 8\n"
      "  border_image_repeat: round\n"), 0);
    Frame();

    // 20 takes two and a half tiles of 8, which is rounded to three tiles
    // of a little less than 7
    const auto quads = _renderer.Quads();
    ASSERT_EQ(quads.size(), 4u + 3u + 3u);

    ExpectQuad(quads[1], 108, 100, 20.0f / 3.0f, 8);
    ExpectPart(quads[1], 8.0f / 24.0f, 0, 16.0f / 24.0f, 8.0f / 24.0f);
    ExpectQuad(quads[3], 108 + 40.0f / 3.0f, 100, 20.0f / 3.0f, 8);
  }

  TEST_F(UiImageTest, StretchesABorderImageUnlessItIsToldOtherwise)
  {
    ShowBox("border_image_source: assets://ui/frame.png\nborder_image_slice: 8\n");

    EXPECT_EQ(_renderer.Quads().size(), 9u);
  }

  // what is wrong

  TEST_F(UiImageTest, SaysWhatIsWrongWithAPropertyOfAnImage)
  {
    const auto problem = [this](const std::string &property)
    {
      return ProblemsOf("- type: panel\n  name: box\n  " + property + "\n");
    };

    EXPECT_THAT(problem("image_rendering: crisp"), ElementsAre(
                  "assets://ui/test.ui.yml:9: 'image_rendering' of panel 'box' is 'crisp', where one of these "
                  "was expected: auto, pixelated"));

    EXPECT_THAT(problem("object_fit: stretch"), ElementsAre(
                  "assets://ui/test.ui.yml:9: 'object_fit' of panel 'box' is 'stretch', where one of these was "
                  "expected: fill, contain, cover, none, scale-down"));

    EXPECT_THAT(problem("object_position: middle"), ElementsAre(
                  "assets://ui/test.ui.yml:9: 'object_position' of panel 'box' is 'middle', where one or two "
                  "of left, center, right, top, bottom, a number of pixels, or a percentage was expected"));

    EXPECT_THAT(problem("background_size: large"), ElementsAre(
                  "assets://ui/test.ui.yml:9: 'background_size' of panel 'box' is 'large', where auto, cover, "
                  "contain, or one to two values, each a number of pixels, a percentage, or auto was "
                  "expected"));

    EXPECT_THAT(problem("background_repeat: space"), ElementsAre(
                  "assets://ui/test.ui.yml:9: 'background_repeat' of panel 'box' is 'space', where one of "
                  "these was expected: repeat, no-repeat, repeat-x, repeat-y"));

    EXPECT_THAT(problem("border_image_repeat: space"), ElementsAre(
                  "assets://ui/test.ui.yml:9: 'border_image_repeat' of panel 'box' is 'space', where one of "
                  "these was expected: stretch, repeat, round"));
  }
}
