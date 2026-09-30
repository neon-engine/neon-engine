#include "ui-document.hpp"

#include <gtest/gtest.h>

namespace
{
  using neon::UiDocument;
  using neon::UiScaleMode;

  struct ScaleCase
  {
    UiScaleMode mode;
    int width;
    int height;
    float scale;
  };

  class UiDocumentScaleTest : public ::testing::TestWithParam<ScaleCase> {};

  TEST_P(UiDocumentScaleTest, ScalesAFileThatWasMadeFor1920By1080)
  {
    UiDocument document;
    document.scale_mode = GetParam().mode;

    EXPECT_FLOAT_EQ(document.ScaleFor(GetParam().width, GetParam().height), GetParam().scale);
  }

  INSTANTIATE_TEST_SUITE_P(EveryMode, UiDocumentScaleTest, ::testing::Values(
    ScaleCase{UiScaleMode::Fit, 1920, 1080, 1.0f},
    ScaleCase{UiScaleMode::Fit, 3840, 2160, 2.0f},
    ScaleCase{UiScaleMode::Fit, 960, 540, 0.5f},
    // the side that is short decides
    ScaleCase{UiScaleMode::Fit, 1280, 1024, 1280.0f / 1920.0f},
    ScaleCase{UiScaleMode::Fit, 3440, 1440, 1440.0f / 1080.0f},
    ScaleCase{UiScaleMode::Fit, 1080, 1920, 1080.0f / 1920.0f},
    ScaleCase{UiScaleMode::Width, 1280, 1024, 1280.0f / 1920.0f},
    ScaleCase{UiScaleMode::Width, 3440, 1440, 3440.0f / 1920.0f},
    ScaleCase{UiScaleMode::Height, 1280, 1024, 1024.0f / 1080.0f},
    ScaleCase{UiScaleMode::Height, 3440, 1440, 1440.0f / 1080.0f},
    ScaleCase{UiScaleMode::None, 1280, 1024, 1.0f},
    ScaleCase{UiScaleMode::None, 3840, 2160, 1.0f}));

  TEST(UiDocument, IsMadeFor1920By1080UnlessItSaysOtherwise)
  {
    const UiDocument document;

    EXPECT_FLOAT_EQ(document.reference_width, 1920);
    EXPECT_FLOAT_EQ(document.reference_height, 1080);
    EXPECT_EQ(document.scale_mode, UiScaleMode::Fit);
    EXPECT_FALSE(document.modal);
    EXPECT_EQ(document.id, -1);
  }

  TEST(UiDocument, ScalesFromTheSizeItWasMadeFor)
  {
    UiDocument document;
    document.reference_width = 1280;
    document.reference_height = 720;

    EXPECT_FLOAT_EQ(document.ScaleFor(1280, 720), 1.0f);
    EXPECT_FLOAT_EQ(document.ScaleFor(1920, 1080), 1.5f);
  }

  TEST(UiDocument, DoesNotDivideByAFrameOrASizeOfNothing)
  {
    UiDocument document;
    EXPECT_FLOAT_EQ(document.ScaleFor(0, 0), 1.0f);
    EXPECT_FLOAT_EQ(document.ScaleFor(1920, 0), 1.0f);

    document.reference_width = 0;
    EXPECT_FLOAT_EQ(document.ScaleFor(1920, 1080), 1.0f);

    document.reference_width = 1920;
    document.reference_height = -1;
    EXPECT_FLOAT_EQ(document.ScaleFor(1920, 1080), 1.0f);
  }
}
