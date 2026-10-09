#include "ui-fixture.hpp"

#include <cmath>

// Where the elements of a file end up, in frames of several sizes. What
// flexbox does with boxes is tested with the layout engine. Here it is what
// a file says that is followed, and how a user interface grows with the
// frame.

namespace
{
  using neon::testing::LogLevel;
  using neon::testing::RecordedQuad;
  using neon::testing::UiTest;

  class UiLayoutTest : public UiTest
  {
  protected:
    /// A user interface with something pinned to every corner, and a bar
    /// along the bottom.
    void ShowCorners(const std::string &top = "")
    {
      ASSERT_GE(Show(
        top +
        "root:\n"
        "  type: panel\n"
        "  name: root\n"
        "  width: 100%\n"
        "  height: 100%\n"
        "  children:\n"
        "    - type: panel\n"
        "      name: top-left\n"
        "      position: absolute\n"
        "      top: 16\n"
        "      left: 16\n"
        "      width: 200\n"
        "      height: 40\n"
        "      background_color: \"#ff0000\"\n"
        "    - type: panel\n"
        "      name: top-right\n"
        "      position: absolute\n"
        "      top: 16\n"
        "      right: 16\n"
        "      width: 200\n"
        "      height: 40\n"
        "      background_color: \"#00ff00\"\n"
        "    - type: panel\n"
        "      name: bottom-left\n"
        "      position: absolute\n"
        "      bottom: 16\n"
        "      left: 16\n"
        "      width: 200\n"
        "      height: 40\n"
        "      background_color: \"#0000ff\"\n"
        "    - type: panel\n"
        "      name: bottom-right\n"
        "      position: absolute\n"
        "      bottom: 16\n"
        "      right: 16\n"
        "      width: 200\n"
        "      height: 40\n"
        "      background_color: \"#ffff00\"\n"
        "    - type: panel\n"
        "      name: center\n"
        "      position: absolute\n"
        "      top: 0\n"
        "      right: 0\n"
        "      bottom: 0\n"
        "      left: 0\n"
        "      margin: auto\n"
        "      width: 50%\n"
        "      height: 100\n"
        "      background_color: \"#ff00ff\"\n"), 0) << _logger->Messages(LogLevel::Error);
    }

    /// The rectangle that was drawn for the background of an element,
    /// found by its color.
    [[nodiscard]] RecordedQuad QuadOf(const float red, const float green, const float blue) const
    {
      for (const auto &quad : _renderer.Quads())
      {
        if (!quad.textured && quad.color.r == red && quad.color.g == green && quad.color.b == blue)
        {
          return quad;
        }
      }

      ADD_FAILURE() << "Nothing was drawn in the color " << red << ", " << green << ", " << blue;
      return {};
    }

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
  };

  // the size of the frame

  TEST_F(UiLayoutTest, KeepsWhatIsPinnedToACornerAtItsCornerAtTheSizeItWasMadeFor)
  {
    ShowCorners();
    Frame();

    ExpectBox("root", 0, 0, 1920, 1080);
    ExpectBox("top-left", 16, 16, 200, 40);
    ExpectBox("top-right", 1704, 16, 200, 40);
    ExpectBox("bottom-left", 16, 1024, 200, 40);
    ExpectBox("bottom-right", 1704, 1024, 200, 40);
    ExpectBox("center", 480, 490, 960, 100);

    // a unit of the file is a pixel
    ExpectQuad(QuadOf(1, 0, 0), 16, 16, 200, 40);
    ExpectQuad(QuadOf(1, 1, 0), 1704, 1024, 200, 40);
  }

  struct FrameSize
  {
    int width;
    int height;

    /// Pixels for each unit of a file that was made for 1920 by 1080.
    float scale;
  };

  void PrintTo(const FrameSize &value, std::ostream *out)
  {
    *out << value.width << " by " << value.height;
  }

  class UiFrameSizeTest : public UiLayoutTest, public ::testing::WithParamInterface<FrameSize> {};

  TEST_P(UiFrameSizeTest, KeepsWhatIsPinnedToACornerAtItsCorner)
  {
    const auto [width, height, scale] = GetParam();
    _renderer.SetResolution(width, height);

    ShowCorners();
    Frame();

    // in units of the file, the frame is as large as it is in pixels,
    // divided by the scale
    const float units_wide = static_cast<float>(width) / scale;
    const float units_high = static_cast<float>(height) / scale;

    ExpectBox("root", 0, 0, units_wide, units_high);
    ExpectBox("top-left", 16, 16, 200, 40);
    ExpectBox("top-right", units_wide - 216, 16, 200, 40);
    ExpectBox("bottom-left", 16, units_high - 56, 200, 40);
    ExpectBox("bottom-right", units_wide - 216, units_high - 56, 200, 40);
    ExpectBox("center", units_wide / 4, (units_high - 100) / 2, units_wide / 2, 100);

    // in pixels, everything is as far from its corner as the scale says
    const RecordedQuad top_left = QuadOf(1, 0, 0);
    EXPECT_FLOAT_EQ(top_left.left, std::round(16 * scale));
    EXPECT_FLOAT_EQ(top_left.top, std::round(16 * scale));
    EXPECT_NEAR(top_left.Width(), 200 * scale, 1);
    EXPECT_NEAR(top_left.Height(), 40 * scale, 1);

    const RecordedQuad bottom_right = QuadOf(1, 1, 0);
    EXPECT_FLOAT_EQ(bottom_right.right, std::round(static_cast<float>(width) - 16 * scale));
    EXPECT_FLOAT_EQ(bottom_right.bottom, std::round(static_cast<float>(height) - 16 * scale));

    const RecordedQuad center = QuadOf(1, 0, 1);
    EXPECT_NEAR(center.left + center.Width() / 2, static_cast<float>(width) / 2, 1);
    EXPECT_NEAR(center.top + center.Height() / 2, static_cast<float>(height) / 2, 1);
  }

  TEST_P(UiFrameSizeTest, DrawsEveryEdgeAtAWholePixel)
  {
    const auto [width, height, scale] = GetParam();
    _renderer.SetResolution(width, height);

    ShowCorners();
    Frame();

    for (const auto &quad : _renderer.Quads())
    {
      EXPECT_FLOAT_EQ(quad.left, std::round(quad.left));
      EXPECT_FLOAT_EQ(quad.top, std::round(quad.top));
      EXPECT_FLOAT_EQ(quad.right, std::round(quad.right));
      EXPECT_FLOAT_EQ(quad.bottom, std::round(quad.bottom));
    }
  }

  TEST_P(UiFrameSizeTest, DrawsNothingOutsideTheFrame)
  {
    const auto [width, height, scale] = GetParam();
    _renderer.SetResolution(width, height);

    ShowCorners();
    Frame();

    ASSERT_EQ(_renderer.Quads().size(), 5u);

    for (const auto &quad : _renderer.Quads())
    {
      EXPECT_GE(quad.left, 0);
      EXPECT_GE(quad.top, 0);
      EXPECT_LE(quad.right, static_cast<float>(width));
      EXPECT_LE(quad.bottom, static_cast<float>(height));
    }
  }

  INSTANTIATE_TEST_SUITE_P(SeveralSizes, UiFrameSizeTest, ::testing::Values(
    FrameSize{1920, 1080, 1.0f},
    FrameSize{1280, 720, 1280.0f / 1920.0f},
    FrameSize{3840, 2160, 2.0f},
    // narrower than it was made for: the width decides
    FrameSize{1280, 1024, 1280.0f / 1920.0f},
    FrameSize{1080, 1920, 1080.0f / 1920.0f},
    // wider than it was made for: the height decides
    FrameSize{3440, 1440, 1440.0f / 1080.0f},
    FrameSize{2560, 1080, 1.0f},
    FrameSize{1366, 768, 768.0f / 1080.0f}));

  // scale

  TEST_F(UiLayoutTest, ScalesWithWhatTheFileSays)
  {
    _renderer.SetResolution(1280, 1024);

    const auto width_of_root = [this](const std::string &top)
    {
      const int document = Show(top + "root:\n  type: panel\n  name: root\n  width: 100%\n  height: 100%\n");
      EXPECT_GE(document, 0) << _logger->Messages(LogLevel::Error);

      Frame();
      const neon::UiRectangle box = Element("root").GetBox();

      _ui->Unload(document);
      return box;
    };

    // the width decides here, since it is what is short
    EXPECT_NEAR(width_of_root("").Width(), 1920, 0.01f);
    EXPECT_NEAR(width_of_root("").Height(), 1536, 0.01f);
    EXPECT_NEAR(width_of_root("scale: fit\n").Height(), 1536, 0.01f);

    EXPECT_NEAR(width_of_root("scale: width\n").Width(), 1920, 0.01f);
    EXPECT_NEAR(width_of_root("scale: width\n").Height(), 1536, 0.01f);

    EXPECT_NEAR(width_of_root("scale: height\n").Width(), 1350, 0.01f);
    EXPECT_NEAR(width_of_root("scale: height\n").Height(), 1080, 0.01f);

    // a unit is a pixel
    EXPECT_NEAR(width_of_root("scale: none\n").Width(), 1280, 0.01f);
    EXPECT_NEAR(width_of_root("scale: none\n").Height(), 1024, 0.01f);
  }

  TEST_F(UiLayoutTest, ScalesFromTheSizeTheFileWasMadeFor)
  {
    _renderer.SetResolution(1920, 1080);

    ShowCorners("reference_size: [960, 540]\n");
    Frame();

    ExpectBox("root", 0, 0, 960, 540);
    ExpectQuad(QuadOf(1, 0, 0), 32, 32, 400, 80);
  }

  TEST_F(UiLayoutTest, FollowsAFrameThatChangesItsSize)
  {
    ShowCorners();
    Frame();
    ExpectQuad(QuadOf(1, 1, 0), 1704, 1024, 200, 40);

    _renderer.SetResolution(960, 540);
    Frame();

    ExpectQuad(QuadOf(1, 1, 0), 852, 512, 100, 20);
  }

  TEST_F(UiLayoutTest, FilesOfDifferentScaleAreShownTogether)
  {
    ASSERT_GE(Show(
      "root:\n  type: panel\n  name: large\n  width: 100\n  height: 100\n  background_color: \"#ff0000\"\n",
      "ui/large.ui.yml"), 0);
    ASSERT_GE(Show(
      "reference_size: [960, 540]\n"
      "root:\n  type: panel\n  name: small\n  width: 100\n  height: 100\n  background_color: \"#00ff00\"\n",
      "ui/small.ui.yml"), 0);

    Frame();

    ExpectQuad(QuadOf(1, 0, 0), 0, 0, 100, 100);
    ExpectQuad(QuadOf(0, 1, 0), 0, 0, 200, 200);
  }

  // sizes

  TEST_F(UiLayoutTest, PercentRefersToTheParent)
  {
    ASSERT_GE(ShowUnderRoot(
      "- type: panel\n"
      "  name: half\n"
      "  width: 50%\n"
      "  height: 25%\n"
      "  padding: 20\n"
      "  box_sizing: border-box\n"
      "  children:\n"
      "    - type: panel\n"
      "      name: inside\n"
      "      width: 50%\n"
      "      height: 50%\n"), 0) << _logger->Messages(LogLevel::Error);

    Frame();

    ExpectBox("half", 0, 0, 960, 270);

    // of what is inside the padding
    ExpectBox("inside", 20, 20, 460, 115);
  }

  TEST_F(UiLayoutTest, ALabelIsAsLargeAsItsText)
  {
    ASSERT_GE(ShowUnderRoot(
      "- type: label\n  name: short\n  text: Start\n  align_self: flex-start\n"
      "- type: label\n  name: long\n  text: \"Health: 75\"\n  align_self: flex-start\n  font_size: 24\n"
      "- type: label\n  name: padded\n  text: Start\n  align_self: flex-start\n  padding: 4 8\n"
      "- type: label\n  name: empty\n  align_self: flex-start\n"), 0) << _logger->Messages(LogLevel::Error);

    Frame();

    // a character of the font of the tests is half its size wide
    ExpectBox("short", 0, 0, 40, 16);
    ExpectBox("long", 40, 0, 120, 24);
    ExpectBox("padded", 160, 0, 56, 24);
    ExpectBox("empty", 216, 0, 0, 0);
  }

  TEST_F(UiLayoutTest, ALabelBreaksItsLinesAtTheWidthItHas)
  {
    ASSERT_GE(ShowUnderRoot(
      "- type: panel\n"
      "  name: column\n"
      "  width: 80\n"
      "  flex_direction: column\n"
      "  align_self: flex-start\n"
      "  children:\n"
      "    - type: label\n"
      "      name: text\n"
      "      text: aaaa bbbb cccc\n"
      "    - type: label\n"
      "      name: below\n"
      "      text: dd\n"), 0) << _logger->Messages(LogLevel::Error);

    Frame();

    // `aaaa bbbb` is 72 wide and fits, `cccc` goes to the next line
    ExpectBox("text", 0, 0, 80, 32);
    ExpectBox("below", 0, 32, 80, 16);
    ExpectBox("column", 0, 0, 80, 48);
  }

  TEST_F(UiLayoutTest, TheHeightOfALineIsWhatTheFileSays)
  {
    ASSERT_GE(ShowUnderRoot(
      "- type: label\n  name: double\n  text: \"a\\nb\"\n  line_height: 2\n  align_self: flex-start\n"
      "- type: label\n  name: pixels\n  text: \"a\\nb\"\n  line_height: 20px\n  align_self: flex-start\n"),
      0) << _logger->Messages(LogLevel::Error);

    Frame();

    ExpectBox("double", 0, 0, 8, 64);
    ExpectBox("pixels", 8, 0, 8, 40);
  }

  TEST_F(UiLayoutTest, AnImageIsAsLargeAsItsFile)
  {
    _renderer.image_width = 48;
    _renderer.image_height = 32;

    ASSERT_GE(ShowUnderRoot(
      "- type: image\n  name: as-it-is\n  src: assets://ui/heart.png\n  align_self: flex-start\n"
      "- type: image\n  name: sized\n  src: assets://ui/heart.png\n  width: 96\n  height: 96\n"),
      0) << _logger->Messages(LogLevel::Error);

    Frame();

    ExpectBox("as-it-is", 0, 0, 48, 32);
    ExpectBox("sized", 48, 0, 96, 96);
  }

  TEST_F(UiLayoutTest, AButtonIsAsLargeAsItsTextAndItsPadding)
  {
    ASSERT_GE(ShowUnderRoot(
      "- type: button\n  name: start\n  text: Start\n  align_self: flex-start\n"), 0);

    Frame();

    ExpectBox("start", 0, 0, 72, 32);
  }

  // containers

  TEST_F(UiLayoutTest, StacksAColumnWithAGap)
  {
    ASSERT_GE(ShowUnderRoot(
      "- type: panel\n"
      "  name: menu\n"
      "  position: absolute\n"
      "  bottom: 32\n"
      "  right: 32\n"
      "  padding: 16\n"
      "  gap: 12\n"
      "  flex_direction: column\n"
      "  children:\n"
      "    - type: button\n"
      "      name: start\n"
      "      text: Start\n"
      "    - type: button\n"
      "      name: quit\n"
      "      text: Quit\n"), 0) << _logger->Messages(LogLevel::Error);

    Frame();

    // as wide as the wider button, and both buttons stretch to it
    ExpectBox("menu", 1920 - 32 - 104, 1080 - 32 - 108, 104, 108);
    ExpectBox("start", 1784 + 16, 940 + 16, 72, 32);
    ExpectBox("quit", 1784 + 16, 940 + 16 + 32 + 12, 72, 32);
  }

  TEST_F(UiLayoutTest, LaysOutARowThatSharesItsWidth)
  {
    ASSERT_GE(ShowUnderRoot(
      "- type: panel\n"
      "  name: bar\n"
      "  width: 1000\n"
      "  height: 50\n"
      "  gap: 10\n"
      "  children:\n"
      "    - type: panel\n"
      "      name: fixed\n"
      "      width: 100\n"
      "    - type: panel\n"
      "      name: one\n"
      "      flex: 1\n"
      "    - type: panel\n"
      "      name: two\n"
      "      flex: 2\n"), 0) << _logger->Messages(LogLevel::Error);

    Frame();

    // 880 are left over after the fixed one and the gaps
    ExpectBox("fixed", 0, 0, 100, 50);
    ExpectBox("one", 110, 0, 880.0f / 3, 50);
    ExpectBox("two", 120 + 880.0f / 3, 0, 1760.0f / 3, 50);
  }

  TEST_F(UiLayoutTest, BoxesThatTouchShareAnEdgeInPixels)
  {
    _renderer.SetResolution(1366, 768);

    ASSERT_GE(ShowUnderRoot(
      "- type: panel\n  flex: 1\n  background_color: \"#ff0000\"\n"
      "- type: panel\n  flex: 1\n  background_color: \"#00ff00\"\n"
      "- type: panel\n  flex: 1\n  background_color: \"#0000ff\"\n"), 0);

    Frame();

    const RecordedQuad first = QuadOf(1, 0, 0);
    const RecordedQuad second = QuadOf(0, 1, 0);
    const RecordedQuad third = QuadOf(0, 0, 1);

    // neither a gap nor a pixel that is drawn twice
    EXPECT_FLOAT_EQ(first.left, 0);
    EXPECT_FLOAT_EQ(first.right, second.left);
    EXPECT_FLOAT_EQ(second.right, third.left);
    EXPECT_FLOAT_EQ(third.right, 1366);
    EXPECT_FLOAT_EQ(third.bottom, 768);
  }

  TEST_F(UiLayoutTest, ElementsInsideElementsAreCountedFromTheFrame)
  {
    ASSERT_GE(ShowUnderRoot(
      "- type: panel\n"
      "  name: outer\n"
      "  margin: 100\n"
      "  padding: 10\n"
      "  border_width: 5\n"
      "  width: 400\n"
      "  height: 300\n"
      "  children:\n"
      "    - type: panel\n"
      "      name: inner\n"
      "      padding: 10\n"
      "      flex: 1\n"
      "      children:\n"
      "        - type: panel\n"
      "          name: innermost\n"
      "          position: absolute\n"
      "          right: 0\n"
      "          bottom: 0\n"
      "          width: 50\n"
      "          height: 50\n"), 0) << _logger->Messages(LogLevel::Error);

    Frame();

    ExpectBox("outer", 100, 100, 430, 330);
    ExpectBox("inner", 115, 115, 400, 300);
    ExpectBox("innermost", 465, 365, 50, 50);

    const neon::UiRectangle content = Element("outer").GetContentBox();
    EXPECT_FLOAT_EQ(content.left, 115);
    EXPECT_FLOAT_EQ(content.Width(), 400);

    const neon::UiRectangle padding = Element("outer").GetPaddingBox();
    EXPECT_FLOAT_EQ(padding.left, 105);
    EXPECT_FLOAT_EQ(padding.Width(), 420);
  }

  // what is not shown

  TEST_F(UiLayoutTest, WhatIsNotDisplayedTakesNoRoom)
  {
    ASSERT_GE(ShowUnderRoot(
      "- type: panel\n  name: first\n  width: 100\n"
      "- type: panel\n  name: gone\n  width: 100\n  display: none\n"
      "- type: panel\n  name: hidden\n  width: 100\n  hidden: true\n"
      "- type: panel\n  name: last\n  width: 100\n"), 0) << _logger->Messages(LogLevel::Error);

    Frame();

    ExpectBox("first", 0, 0, 100, 1080);
    ExpectBox("last", 100, 0, 100, 1080);

    EXPECT_TRUE(Element("gone").IsHidden());
    EXPECT_TRUE(Element("hidden").IsHidden());
    EXPECT_FALSE(Element("last").IsHidden());
  }

  TEST_F(UiLayoutTest, WhatIsBelowSomethingHiddenIsHiddenAsWell)
  {
    ASSERT_GE(ShowUnderRoot(
      "- type: panel\n"
      "  name: menu\n"
      "  hidden: true\n"
      "  children:\n"
      "    - type: button\n"
      "      name: start\n"
      "      text: Start\n"), 0) << _logger->Messages(LogLevel::Error);

    Frame();

    EXPECT_TRUE(Element("start").IsHidden());
    EXPECT_TRUE(_renderer.batches.empty());
  }

  // text in pixels

  TEST_P(UiFrameSizeTest, DrawsEveryCharacterAtTheSizeItHasInItsAtlas)
  {
    const auto [width, height, scale] = GetParam();
    _renderer.SetResolution(width, height);

    ASSERT_GE(ShowUnderRoot(
      "- type: label\n"
      "  text: \"Health: 75\"\n"
      "  position: absolute\n"
      "  top: 17\n"
      "  left: 17\n"
      "  font_size: 24\n"), 0) << _logger->Messages(LogLevel::Error);

    Frame();

    const auto quads = _renderer.Quads();
    ASSERT_EQ(quads.size(), 9u);

    int atlas_width = 0;
    int atlas_height = 0;
    ASSERT_TRUE(_renderer.GetTextureSize(quads[0].texture, atlas_width, atlas_height));

    // the font is drawn at the size it has on the screen, not at the size
    // the file names
    const float pixel_size = std::round(24 * scale);

    for (const auto &quad : quads)
    {
      EXPECT_TRUE(quad.textured);

      // a pixel of the atlas is a pixel of the frame, which is what keeps
      // a character as sharp as it was drawn
      EXPECT_NEAR((quad.texture_right - quad.texture_left) * static_cast<float>(atlas_width), quad.Width(), 0.001f);
      EXPECT_NEAR((quad.texture_bottom - quad.texture_top) * static_cast<float>(atlas_height), quad.Height(), 0.001f);

      EXPECT_FLOAT_EQ(quad.Width(), std::floor(pixel_size * 0.4f));
      EXPECT_FLOAT_EQ(quad.Height(), std::floor(pixel_size * 0.7f));

      EXPECT_FLOAT_EQ(quad.left, std::round(quad.left));
      EXPECT_FLOAT_EQ(quad.top, std::round(quad.top));
    }
  }
}
