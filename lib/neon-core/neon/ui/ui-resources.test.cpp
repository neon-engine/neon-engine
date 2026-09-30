#include "ui-resources.hpp"

#include <memory>

#include <gtest/gtest.h>

#include <neon/testing/fake-font-rasterizer.hpp>
#include <neon/testing/memory-file-system.hpp>
#include <neon/testing/mock-render-2d-context.hpp>
#include <neon/testing/recording-logger.hpp>

namespace
{
  using neon::No_Texture;
  using neon::UiFont;
  using neon::UiImage;
  using neon::UiResources;
  using neon::testing::FakeFontRasterizer;
  using neon::testing::LogLevel;
  using neon::testing::MemoryFileSystem;
  using neon::testing::RecordingLogger;
  using neon::testing::RecordingRenderer2D;

  class UiResourcesTest : public ::testing::Test
  {
  protected:
    std::shared_ptr<RecordingLogger> _logger = std::make_shared<RecordingLogger>();
    MemoryFileSystem _file_system{SettingsConfig{}, _logger};
    FakeFontRasterizer _rasterizer;
    RecordingRenderer2D _renderer;
    UiResources _resources{&_renderer, &_rasterizer, &_file_system, _logger};

    void SetUp() override
    {
      _file_system.Initialize();
      _file_system.AddNativeFile("/assets/fonts/regular.ttf", "a font");
      _file_system.AddNativeFile("/assets/fonts/bold.ttf", "a font");
      _file_system.AddNativeFile("/assets/fonts/empty.ttf", "");

      _resources.AddFace("sans-serif", 400, "assets://fonts/regular.ttf");
      _resources.AddFace("sans-serif", 700, "assets://fonts/bold.ttf");
    }
  };

  // fonts

  TEST_F(UiResourcesTest, ReadsNoFileUntilAFontIsAskedFor)
  {
    EXPECT_TRUE(_resources.HasFamily("sans-serif"));
    EXPECT_FALSE(_resources.HasFamily("serif"));

    EXPECT_EQ(_renderer.created, 0u);
    EXPECT_EQ(_rasterizer.rasterized, 0u);
    EXPECT_FALSE(_rasterizer.IsLoaded(0));
  }

  TEST_F(UiResourcesTest, MakesAFontAtASize)
  {
    const UiFont *font = _resources.GetFont("sans-serif", 400, 24);

    ASSERT_NE(font, nullptr);
    EXPECT_NE(font->texture, No_Texture);
    EXPECT_FLOAT_EQ(font->atlas.GetPixelSize(), 24);
    EXPECT_NE(font->atlas.Find(U'A'), nullptr);

    int width = 0;
    int height = 0;
    ASSERT_TRUE(_renderer.GetTextureSize(font->texture, width, height));
    EXPECT_EQ(width, font->atlas.GetWidth());
    EXPECT_EQ(height, font->atlas.GetHeight());

    EXPECT_TRUE(_logger->Contains(LogLevel::Info, "Loaded the font assets://fonts/regular.ttf"));
  }

  TEST_F(UiResourcesTest, MakesAFontAtASizeOnce)
  {
    const UiFont *first = _resources.GetFont("sans-serif", 400, 24);
    const std::size_t rasterized = _rasterizer.rasterized;

    const UiFont *second = _resources.GetFont("sans-serif", 400, 24);

    EXPECT_EQ(first, second);
    EXPECT_EQ(_renderer.created, 1u);
    EXPECT_EQ(_rasterizer.rasterized, rasterized);
  }

  TEST_F(UiResourcesTest, EverySizeAndEveryWeightIsAFontOfItsOwn)
  {
    const UiFont *small = _resources.GetFont("sans-serif", 400, 12);
    const UiFont *large = _resources.GetFont("sans-serif", 400, 24);
    const UiFont *bold = _resources.GetFont("sans-serif", 700, 24);

    EXPECT_NE(small, large);
    EXPECT_NE(large, bold);
    EXPECT_NE(small->texture, large->texture);
    EXPECT_NE(large->texture, bold->texture);
    EXPECT_EQ(_renderer.created, 3u);

    // a font stays where it is while others are made
    EXPECT_EQ(_resources.GetFont("sans-serif", 400, 12), small);
  }

  TEST_F(UiResourcesTest, TakesTheWeightThatIsNearest)
  {
    const UiFont *regular = _resources.GetFont("sans-serif", 400, 24);
    const UiFont *bold = _resources.GetFont("sans-serif", 700, 24);

    EXPECT_EQ(_resources.GetFont("sans-serif", 100, 24), regular);
    EXPECT_EQ(_resources.GetFont("sans-serif", 500, 24), regular);
    EXPECT_EQ(_resources.GetFont("sans-serif", 600, 24), bold);
    EXPECT_EQ(_resources.GetFont("sans-serif", 900, 24), bold);

    EXPECT_EQ(_renderer.created, 2u);
  }

  TEST_F(UiResourcesTest, AFamilyWithOneWeightIsDrawnWithIt)
  {
    _resources.AddFace("title", 700, "assets://fonts/bold.ttf");

    EXPECT_NE(_resources.GetFont("title", 400, 24), nullptr);
  }

  TEST_F(UiResourcesTest, SaysOnceThatAFamilyIsNotKnown)
  {
    EXPECT_EQ(_resources.GetFont("serif", 400, 24), nullptr);
    EXPECT_EQ(_resources.GetFont("serif", 400, 24), nullptr);
    EXPECT_EQ(_resources.GetFont("serif", 700, 12), nullptr);

    EXPECT_EQ(_logger->Count(LogLevel::Error), 1u) << _logger->Messages(LogLevel::Error);
    EXPECT_TRUE(_logger->Contains(
      LogLevel::Error, "The font family 'serif' is not known, text that asks for it is not drawn"));
  }

  TEST_F(UiResourcesTest, ReadsAFileThatIsMissingOnce)
  {
    _resources.AddFace("gone", 400, "assets://fonts/gone.ttf");

    EXPECT_EQ(_resources.GetFont("gone", 400, 24), nullptr);
    EXPECT_EQ(_resources.GetFont("gone", 400, 24), nullptr);
    EXPECT_EQ(_resources.GetFont("gone", 400, 12), nullptr);

    EXPECT_EQ(_logger->Count(LogLevel::Error), 1u) << _logger->Messages(LogLevel::Error);
    EXPECT_TRUE(_logger->Contains(LogLevel::Error, "The font assets://fonts/gone.ttf cannot be read"));
    EXPECT_EQ(_renderer.created, 0u);
  }

  TEST_F(UiResourcesTest, HandsAFileThatIsNoFontToTheRasterizerOnce)
  {
    _resources.AddFace("empty", 400, "assets://fonts/empty.ttf");

    EXPECT_EQ(_resources.GetFont("empty", 400, 24), nullptr);
    EXPECT_EQ(_resources.GetFont("empty", 400, 24), nullptr);

    EXPECT_EQ(_logger->Count(LogLevel::Error), 1u) << _logger->Messages(LogLevel::Error);
    EXPECT_TRUE(_logger->Contains(LogLevel::Error, "assets://fonts/empty.ttf is not a font that can be used"));
  }

  TEST_F(UiResourcesTest, TriesASizeThatCannotBeDrawnOnce)
  {
    EXPECT_EQ(_resources.GetFont("sans-serif", 400, 2000), nullptr);
    const std::size_t rasterized = _rasterizer.rasterized;

    EXPECT_EQ(_resources.GetFont("sans-serif", 400, 2000), nullptr);

    EXPECT_EQ(_rasterizer.rasterized, rasterized);
    EXPECT_EQ(_logger->Count(LogLevel::Error), 1u) << _logger->Messages(LogLevel::Error);
    EXPECT_TRUE(_logger->Contains(
      LogLevel::Error,
      "The font assets://fonts/regular.ttf cannot be drawn at 2000 pixels: the characters of a font of size "
      "2000 do not fit into an image of 4096 by 4096"));

    // the sizes that can be drawn still are
    EXPECT_NE(_resources.GetFont("sans-serif", 400, 24), nullptr);
  }

  TEST_F(UiResourcesTest, AsksOnceForATextureTheRendererDoesNotTake)
  {
    _renderer.refuses_pixels = true;

    EXPECT_EQ(_resources.GetFont("sans-serif", 400, 24), nullptr);
    EXPECT_EQ(_resources.GetFont("sans-serif", 400, 24), nullptr);

    EXPECT_EQ(_renderer.created, 1u);
    EXPECT_EQ(_logger->Count(LogLevel::Error), 1u) << _logger->Messages(LogLevel::Error);
    EXPECT_TRUE(_logger->Contains(
      LogLevel::Error, "The renderer took no texture for the font assets://fonts/regular.ttf at 24 pixels"));
  }

  TEST_F(UiResourcesTest, HasNoFontOfASizeThatIsNotAboveZero)
  {
    EXPECT_EQ(_resources.GetFont("sans-serif", 400, 0), nullptr);
    EXPECT_EQ(_resources.GetFont("sans-serif", 400, -4), nullptr);
    EXPECT_EQ(_renderer.created, 0u);
  }

  TEST_F(UiResourcesTest, AFaceIsReplacedUntilItIsDrawnWith)
  {
    _resources.AddFace("title", 400, "assets://fonts/gone.ttf");
    _resources.AddFace("title", 400, "assets://fonts/bold.ttf");

    EXPECT_NE(_resources.GetFont("title", 400, 24), nullptr);

    // from here on it stays, since the sizes that were made refer to it
    _resources.AddFace("title", 400, "assets://fonts/gone.ttf");

    EXPECT_NE(_resources.GetFont("title", 400, 12), nullptr);
    EXPECT_EQ(_logger->Count(LogLevel::Error), 0u) << _logger->Messages(LogLevel::Error);
  }

  // images

  TEST_F(UiResourcesTest, LoadsAnImageWithItsSize)
  {
    _renderer.image_width = 48;
    _renderer.image_height = 32;

    const UiImage image = _resources.GetImage("assets://ui/heart.png");

    EXPECT_NE(image.texture, No_Texture);
    EXPECT_EQ(image.texture, _renderer.TextureOf("assets://ui/heart.png"));
    EXPECT_EQ(image.width, 48);
    EXPECT_EQ(image.height, 32);
  }

  TEST_F(UiResourcesTest, LoadsAnImageOnce)
  {
    const UiImage first = _resources.GetImage("assets://ui/heart.png");
    const UiImage second = _resources.GetImage("assets://ui/heart.png");
    const UiImage other = _resources.GetImage("assets://ui/star.png");

    EXPECT_EQ(first.texture, second.texture);
    EXPECT_NE(first.texture, other.texture);
    EXPECT_EQ(_renderer.loaded, 2u);
  }

  TEST_F(UiResourcesTest, LoadsAnImageThatCannotBeUsedOnce)
  {
    _renderer.missing = {"assets://ui/missing.png"};

    for (int frame = 0; frame < 3; frame++)
    {
      const UiImage image = _resources.GetImage("assets://ui/missing.png");

      EXPECT_EQ(image.texture, No_Texture);
      EXPECT_EQ(image.width, 0);
      EXPECT_EQ(image.height, 0);
    }

    EXPECT_EQ(_renderer.loaded, 1u);
    EXPECT_EQ(_logger->Count(LogLevel::Error), 1u) << _logger->Messages(LogLevel::Error);
    EXPECT_TRUE(_logger->Contains(
      LogLevel::Error, "The image assets://ui/missing.png cannot be used, what asks for it is drawn without it"));
  }

  // cleaning up

  TEST_F(UiResourcesTest, ReleasesWhatItMade)
  {
    (void) _resources.GetFont("sans-serif", 400, 12);
    (void) _resources.GetFont("sans-serif", 400, 24);
    (void) _resources.GetFont("sans-serif", 700, 24);
    (void) _resources.GetImage("assets://ui/heart.png");
    ASSERT_EQ(_renderer.TextureCount(), 4u);

    _resources.CleanUp();

    EXPECT_EQ(_renderer.TextureCount(), 0u);
    EXPECT_EQ(_renderer.destroyed, 4u);
    EXPECT_FALSE(_rasterizer.IsLoaded(0));
    EXPECT_FALSE(_rasterizer.IsLoaded(1));
  }

  TEST_F(UiResourcesTest, ReleasesNothingOfWhatItCouldNotMake)
  {
    _renderer.missing = {"assets://ui/missing.png"};
    (void) _resources.GetImage("assets://ui/missing.png");
    (void) _resources.GetFont("serif", 400, 24);

    _resources.CleanUp();
    _resources.CleanUp();

    EXPECT_EQ(_renderer.destroyed, 0u);
  }

  TEST_F(UiResourcesTest, MakesEverythingAgainAfterItWasCleanedUp)
  {
    (void) _resources.GetFont("sans-serif", 400, 24);
    (void) _resources.GetImage("assets://ui/heart.png");
    _resources.CleanUp();

    // the faces are still known
    EXPECT_TRUE(_resources.HasFamily("sans-serif"));

    const UiFont *font = _resources.GetFont("sans-serif", 400, 24);
    const UiImage image = _resources.GetImage("assets://ui/heart.png");

    ASSERT_NE(font, nullptr);
    EXPECT_NE(font->texture, No_Texture);
    EXPECT_NE(image.texture, No_Texture);
    EXPECT_EQ(_renderer.TextureCount(), 2u);
  }

  TEST_F(UiResourcesTest, TriesAgainAfterItWasCleanedUpWhatCouldNotBeLoaded)
  {
    _renderer.missing = {"assets://ui/late.png"};
    (void) _resources.GetImage("assets://ui/late.png");

    _resources.CleanUp();
    _renderer.missing.clear();

    EXPECT_NE(_resources.GetImage("assets://ui/late.png").texture, No_Texture);
  }
}
