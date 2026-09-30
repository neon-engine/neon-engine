#include "ui-fixture.hpp"

#include <neon/ui/ui-document.hpp>
#include <neon/window/window-context.hpp>

// The user interface on a display: points, pixels, and density, the scale
// that follows from them and from what the player asked for, what a
// change of them does, and what the window is told: the shape of the
// cursor.

namespace
{
  using neon::Action;
  using neon::CursorShape;
  using neon::UiDocument;
  using neon::UiScaleMode;
  using neon::WindowMetrics;
  using neon::WindowSize;
  using neon::testing::LogLevel;
  using neon::testing::UiTest;

  /// A window of a size in points, whose density the test sets. It keeps
  /// the last shape of the cursor it was told.
  class FakeWindow final : public neon::WindowContext
  {
  public:
    WindowSize points{1920, 1080};
    WindowSize pixels{1920, 1080};
    std::uint64_t revision = 1;
    CursorShape cursor = CursorShape::Default;
    int cursor_changes = 0;

    void SignalToClose() override {}

    double GetDeltaTime() override
    {
      return 1.0 / 60.0;
    }

    void CenterCursor() override {}

    void SetWindowFocus(bool) override {}

    WindowSize GetDrawableSize() override
    {
      return pixels;
    }

    std::vector<std::string> GetVulkanInstanceExtensions() override
    {
      return {};
    }

    bool CreateVulkanSurface(void *, void *) override
    {
      return false;
    }

    WindowSize GetWindowSize() override
    {
      return points;
    }

    WindowMetrics GetMetrics() override
    {
      return {points.width, points.height, pixels.width, pixels.height};
    }

    [[nodiscard]] std::uint64_t GetMetricsRevision() override
    {
      return revision;
    }

    void SetCursorShape(const CursorShape shape) override
    {
      if (shape != cursor) { cursor_changes++; }
      cursor = shape;
    }
  };

  class UiDisplayTest : public UiTest
  {
  protected:
    FakeWindow _window;

    void SetUp() override
    {
      UiTest::SetUp();
      _ui->SetWindow(&_window);
    }

    /// A display of that many points, with that many pixels for each.
    void SetDisplay(const int point_width, const int point_height, const int density)
    {
      _window.points = {point_width, point_height};
      _window.pixels = {point_width * density, point_height * density};
      _window.revision++;
      _renderer.SetResolution(point_width * density, point_height * density);
    }

    void ShowBox(const std::string &top = "")
    {
      ASSERT_GE(Show(
        top +
        "ui: test\n"
        "reference_size: [1920, 1080]\n"
        "root:\n"
        "  type: panel\n"
        "  width: 100%\n"
        "  height: 100%\n"
        "  align_items: flex-start\n"
        "  children:\n"
        "    - type: button\n"
        "      name: box\n"
        "      text: Hi\n"
        "      width: 100\n"
        "      height: 50\n"
        "      padding: 0\n"
        "      title: A box\n"), 0)
        << _logger->Messages(LogLevel::Error);

      Frame();
    }

    /// The rectangle of the box as it was drawn, in pixels: the first
    /// that is not see-through, since the root has no colour.
    [[nodiscard]] neon::testing::RecordedQuad DrawnBox() const
    {
      for (const auto &quad : _renderer.Quads())
      {
        if (!quad.textured && quad.color.a > 0.9f) { return quad; }
      }
      return {};
    }
  };

  // the rule of the scale, with worked examples

  TEST(UiScaleRule, IsTheDensityTimesTheScaleOfThePlayerTimesTheFit)
  {
    UiDocument document;
    document.reference_width = 1920;
    document.reference_height = 1080;

    // a display of 1920 by 1080 points at one pixel a point: as it was made
    EXPECT_FLOAT_EQ(document.ScaleFor(1920.0f, 1080.0f, 1.0f, 1.0f), 1.0f);

    // a display of 3840 by 2160 pixels that is 1920 by 1080 points: the
    // same on the screen, with twice the pixels
    EXPECT_FLOAT_EQ(document.ScaleFor(1920.0f, 1080.0f, 2.0f, 1.0f), 2.0f);

    // a laptop of 1280 by 720 points at two pixels a point: the file is
    // fitted to 1280 by 720, and then drawn at two pixels a point
    EXPECT_FLOAT_EQ(document.ScaleFor(1280.0f, 720.0f, 2.0f, 1.0f), 2.0f * 1280.0f / 1920.0f);

    // the player asks for everything half as large again
    EXPECT_FLOAT_EQ(document.ScaleFor(1280.0f, 720.0f, 2.0f, 1.5f), 1.5f * 2.0f * 1280.0f / 1920.0f);

    // a tall display: the side that is short decides
    EXPECT_FLOAT_EQ(document.ScaleFor(1080.0f, 1920.0f, 1.0f, 1.0f), 1080.0f / 1920.0f);
  }

  TEST(UiScaleRule, WithoutAReferenceSizeOnlyTheDensityAndThePlayerCount)
  {
    UiDocument document;
    document.scale_mode = UiScaleMode::None;

    EXPECT_FLOAT_EQ(document.ScaleFor(1280.0f, 720.0f, 1.0f, 1.0f), 1.0f);
    EXPECT_FLOAT_EQ(document.ScaleFor(1280.0f, 720.0f, 2.0f, 1.0f), 2.0f);
    EXPECT_FLOAT_EQ(document.ScaleFor(640.0f, 480.0f, 2.0f, 1.25f), 2.5f);
  }

  TEST(UiScaleRule, ScalesByTheWidthOrTheHeightAlone)
  {
    UiDocument document;
    document.reference_width = 1920;
    document.reference_height = 1080;

    document.scale_mode = UiScaleMode::Width;
    EXPECT_FLOAT_EQ(document.ScaleFor(3440.0f, 1440.0f, 1.0f, 1.0f), 3440.0f / 1920.0f);

    document.scale_mode = UiScaleMode::Height;
    EXPECT_FLOAT_EQ(document.ScaleFor(3440.0f, 1440.0f, 1.0f, 1.0f), 1440.0f / 1080.0f);
  }

  // the display

  TEST_F(UiDisplayTest, DrawsInPixelsWhatIsLaidOutInPoints)
  {
    SetDisplay(1920, 1080, 2);
    ShowBox();

    // the box is 100 by 50 units, which are points here, and 200 by 100
    // pixels
    ExpectBox("box", 0.0f, 0.0f, 100.0f, 50.0f);

    const auto drawn = DrawnBox();
    EXPECT_FLOAT_EQ(drawn.Width(), 200.0f);
    EXPECT_FLOAT_EQ(drawn.Height(), 100.0f);

  }

  TEST_F(UiDisplayTest, FitsTheFileToASmallerDisplayAndThenDrawsItAtItsDensity)
  {
    SetDisplay(1280, 720, 2);
    ShowBox();

    // 100 units fit to 1280 of 1920 are 66.67 points, and 133.33 pixels
    ExpectBox("box", 0.0f, 0.0f, 100.0f, 50.0f);
    EXPECT_NEAR(DrawnBox().Width(), 133.33f, 0.5f);
  }

  TEST_F(UiDisplayTest, ThePlayerScalesOnTop)
  {
    SetDisplay(1920, 1080, 1);
    _ui->SetUserScale(1.5f);
    ShowBox();

    EXPECT_FLOAT_EQ(DrawnBox().Width(), 150.0f);
    EXPECT_FLOAT_EQ(_ui->GetUserScale(), 1.5f);

    // and can change it while the file is shown
    _ui->SetUserScale(2.0f);
    Frame();
    EXPECT_FLOAT_EQ(DrawnBox().Width(), 200.0f);
  }

  TEST_F(UiDisplayTest, ThePointerIsInPixels)
  {
    SetDisplay(1920, 1080, 2);
    ShowBox();

    // the box ends at 200 pixels
    ClickAt(150.0, 50.0);
    EXPECT_TRUE(_ui->WasClicked("box"));

    ClickAt(250.0, 50.0);
    EXPECT_FALSE(_ui->WasClicked("box"));
  }

  TEST_F(UiDisplayTest, AChangeOfTheDensityLaysOutAgainAndDrawsTheTextAnew)
  {
    SetDisplay(1920, 1080, 1);
    ShowBox();
    EXPECT_FLOAT_EQ(DrawnBox().Width(), 100.0f);

    const std::size_t fonts_before = _ui->GetFontCount();
    EXPECT_GE(fonts_before, 1u);

    // moved to a display of twice the density
    SetDisplay(1920, 1080, 2);
    Frame();
    EXPECT_FLOAT_EQ(DrawnBox().Width(), 200.0f);

    // the text is rasterised at the new size, and the old size is let go
    // of, so that moving back and forth does not pile up atlases
    EXPECT_EQ(_ui->GetFontCount(), fonts_before);

    SetDisplay(1920, 1080, 1);
    Frame();
    SetDisplay(1920, 1080, 2);
    Frame();
    SetDisplay(1920, 1080, 3);
    Frame();
    EXPECT_EQ(_ui->GetFontCount(), fonts_before);
    EXPECT_FLOAT_EQ(DrawnBox().Width(), 300.0f);
  }

  TEST_F(UiDisplayTest, AResizeLaysOutAgain)
  {
    SetDisplay(1920, 1080, 1);
    ASSERT_GE(Show(
      "ui: test\n"
      "reference_size: [1920, 1080]\n"
      "root:\n"
      "  type: panel\n"
      "  width: 100%\n"
      "  height: 100%\n"
      "  align_items: flex-start\n"
      "  children:\n"
      "    - {type: panel, name: half, width: 50%, height: 10, background_color: \"#ffffff\"}\n"), 0);
    Frame();
    ExpectBox("half", 0.0f, 0.0f, 960.0f, 10.0f);
    EXPECT_FLOAT_EQ(DrawnBox().Width(), 960.0f);

    // narrower: the file is fitted by its width, which makes it taller in
    // its own units, and half of it is 640 pixels
    SetDisplay(1280, 1080, 1);
    Frame();
    ExpectBox("half", 0.0f, 0.0f, 960.0f, 10.0f);
    EXPECT_FLOAT_EQ(DrawnBox().Width(), 640.0f);

    // wider: fitted by its height, and half of it is 1280 pixels of 2560
    SetDisplay(2560, 1080, 1);
    Frame();
    ExpectBox("half", 0.0f, 0.0f, 1280.0f, 10.0f);
    EXPECT_FLOAT_EQ(DrawnBox().Width(), 1280.0f);
  }

  // the cursor

  TEST_F(UiDisplayTest, TellsTheWindowTheShapeOfTheCursor)
  {
    SetDisplay(1920, 1080, 1);
    WriteAsset("ui/theme.css", "button { cursor: pointer; }\n");
    ShowBox("styles: [theme.css]\n");
    EXPECT_EQ(_window.cursor, CursorShape::Default);

    PointAt(50.0, 25.0);
    Frame();
    EXPECT_EQ(_window.cursor, CursorShape::Pointer);

    PointAt(500.0, 500.0);
    Frame();
    EXPECT_EQ(_window.cursor, CursorShape::Default);

    // the shape is told when it changes, and not in every frame
    const int changes = _window.cursor_changes;
    Frame();
    Frame();
    EXPECT_EQ(_window.cursor_changes, changes);
  }

  TEST_F(UiDisplayTest, TheCursorFollowsTheStyle)
  {
    SetDisplay(1920, 1080, 1);
    WriteAsset("ui/theme.css", "button { cursor: grab; }\nbutton:active { cursor: grabbing; }\n");
    ShowBox("styles: [theme.css]\n");

    PointAt(50.0, 25.0);
    Frame();
    EXPECT_EQ(_window.cursor, CursorShape::Grab);

    _input.state.SetAction(Action::Pointer_Primary);
    Frame();
    EXPECT_EQ(_window.cursor, CursorShape::Grabbing);
  }

  // the tooltip

  TEST_F(UiDisplayTest, ShowsTheTitleAfterThePointerRested)
  {
    SetDisplay(1920, 1080, 1);
    ShowBox();

    const auto glyphs = [this]
    {
      int count = 0;
      for (const auto &quad : _renderer.Quads())
      {
        if (quad.textured) { count++; }
      }
      return count;
    };

    PointAt(50.0, 25.0);
    Frame();
    EXPECT_EQ(glyphs(), 2) << "only the text of the button";

    // rests for half a second: nothing yet
    for (int i = 0; i < 30; i++)
    {
      _ui->Advance(1.0 / 60.0);
      Frame();
    }
    EXPECT_EQ(glyphs(), 2);

    for (int i = 0; i < 10; i++)
    {
      _ui->Advance(1.0 / 60.0);
      Frame();
    }
    EXPECT_EQ(glyphs(), 2 + 4) << "and 'A box' without its space";

    // moving away takes it away
    PointAt(500.0, 500.0);
    Frame();
    EXPECT_EQ(glyphs(), 2);
  }
}
