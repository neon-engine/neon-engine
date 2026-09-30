#include "window-metrics.hpp"

#include <memory>

#include <gtest/gtest.h>

#include <neon/testing/recording-logger.hpp>
#include <neon/window/headless-window-system.hpp>

// The arithmetic of points, pixels, and density, with the numbers of
// displays that exist.

namespace
{
  using neon::CursorShape;
  using neon::Headless_WindowSystem;
  using neon::WindowMetrics;
  using neon::testing::RecordingLogger;

  TEST(WindowMetricsTest, HasADensityOfTwoOnARetinaDisplay)
  {
    // a window of 1440 by 900 points with 2880 by 1800 pixels
    const WindowMetrics metrics{1440, 900, 2880, 1800};

    EXPECT_DOUBLE_EQ(metrics.ScaleX(), 2.0);
    EXPECT_DOUBLE_EQ(metrics.ScaleY(), 2.0);
    EXPECT_DOUBLE_EQ(metrics.Density(), 2.0);
  }

  TEST(WindowMetricsTest, PutsThePointerOfARetinaDisplayAtTwiceItsPlace)
  {
    const WindowMetrics metrics{1440, 900, 2880, 1800};

    EXPECT_DOUBLE_EQ(metrics.ToPixelsX(0.0), 0.0);
    EXPECT_DOUBLE_EQ(metrics.ToPixelsX(720.0), 1440.0);
    EXPECT_DOUBLE_EQ(metrics.ToPixelsY(450.0), 900.0);
    EXPECT_DOUBLE_EQ(metrics.ToPixelsX(1439.0), 2878.0);
    EXPECT_DOUBLE_EQ(metrics.ToPixelsY(899.0), 1798.0);
  }

  TEST(WindowMetricsTest, HasADensityOfOneAndAQuarterOnALaptopSetTo125Percent)
  {
    // a window of 1280 by 720 points with 1600 by 900 pixels
    const WindowMetrics metrics{1280, 720, 1600, 900};

    EXPECT_DOUBLE_EQ(metrics.ScaleX(), 1.25);
    EXPECT_DOUBLE_EQ(metrics.ScaleY(), 1.25);

    EXPECT_DOUBLE_EQ(metrics.ToPixelsX(640.0), 800.0);
    EXPECT_DOUBLE_EQ(metrics.ToPixelsY(360.0), 450.0);
    EXPECT_DOUBLE_EQ(metrics.ToPixelsX(100.0), 125.0);
    EXPECT_DOUBLE_EQ(metrics.ToPixelsY(3.0), 3.75);
  }

  TEST(WindowMetricsTest, LeavesThePointerWhereItIsAtADensityOfOne)
  {
    const WindowMetrics metrics{1920, 1080, 1920, 1080};

    EXPECT_DOUBLE_EQ(metrics.Density(), 1.0);
    EXPECT_DOUBLE_EQ(metrics.ToPixelsX(123.0), 123.0);
    EXPECT_DOUBLE_EQ(metrics.ToPixelsY(456.0), 456.0);
  }

  TEST(WindowMetricsTest, ScalesEachSideByItself)
  {
    // 150 %, where the platform rounded the height of 1350.5 down
    const WindowMetrics metrics{1601, 901, 2402, 1351};

    EXPECT_NEAR(metrics.ScaleX(), 1.50031, 0.0001);
    EXPECT_NEAR(metrics.ScaleY(), 1.49945, 0.0001);

    // the last point is the last pixel along both sides
    EXPECT_NEAR(metrics.ToPixelsX(1601.0), 2402.0, 1e-9);
    EXPECT_NEAR(metrics.ToPixelsY(901.0), 1351.0, 1e-9);
  }

  TEST(WindowMetricsTest, GoesFromPixelsBackToPoints)
  {
    const WindowMetrics metrics{1280, 720, 1600, 900};

    EXPECT_DOUBLE_EQ(metrics.ToPointsX(800.0), 640.0);
    EXPECT_DOUBLE_EQ(metrics.ToPointsY(metrics.ToPixelsY(333.0)), 333.0);
  }

  TEST(WindowMetricsTest, HasADensityOfOneForAWindowWithoutASize)
  {
    const WindowMetrics metrics{};

    EXPECT_DOUBLE_EQ(metrics.ScaleX(), 1.0);
    EXPECT_DOUBLE_EQ(metrics.ScaleY(), 1.0);
    EXPECT_DOUBLE_EQ(metrics.ToPixelsX(10.0), 10.0);
  }

  TEST(WindowMetricsTest, TellsTwoWindowsApart)
  {
    EXPECT_EQ((WindowMetrics{1, 2, 3, 4}), (WindowMetrics{1, 2, 3, 4}));
    EXPECT_NE((WindowMetrics{1, 2, 3, 4}), (WindowMetrics{1, 2, 3, 5}));
  }

  class HeadlessWindowMetricsTest : public ::testing::Test
  {
  protected:
    std::shared_ptr<RecordingLogger> _logger = std::make_shared<RecordingLogger>();
    SettingsConfig _settings{.width = 1280, .height = 720};
  };

  TEST_F(HeadlessWindowMetricsTest, HasADensityOfOneWithoutARenderScale)
  {
    Headless_WindowSystem window(_settings, _logger);

    const WindowMetrics metrics = window.GetMetrics();
    EXPECT_EQ(metrics, (WindowMetrics{1280, 720, 1280, 720}));
    EXPECT_DOUBLE_EQ(metrics.Density(), 1.0);
  }

  TEST_F(HeadlessWindowMetricsTest, DrawsMorePixelsThanPointsWithARenderScale)
  {
    _settings.render_scale = 1.25;
    Headless_WindowSystem window(_settings, _logger);

    EXPECT_EQ(window.GetWindowSize().width, 1280);
    EXPECT_EQ(window.GetWindowSize().height, 720);
    EXPECT_EQ(window.GetDrawableSize().width, 1600);
    EXPECT_EQ(window.GetDrawableSize().height, 900);
    EXPECT_DOUBLE_EQ(window.GetMetrics().Density(), 1.25);
  }

  TEST_F(HeadlessWindowMetricsTest, StandsInForARetinaDisplay)
  {
    _settings.width = 1440;
    _settings.height = 900;
    _settings.render_scale = 2.0;
    Headless_WindowSystem window(_settings, _logger);

    EXPECT_EQ(window.GetMetrics(), (WindowMetrics{1440, 900, 2880, 1800}));
  }

  TEST_F(HeadlessWindowMetricsTest, NeverChangesItsSize)
  {
    Headless_WindowSystem window(_settings, _logger);

    const auto before = window.GetMetricsRevision();
    window.Update();
    EXPECT_EQ(window.GetMetricsRevision(), before);
  }

  TEST_F(HeadlessWindowMetricsTest, RemembersTheShapeOfTheCursor)
  {
    Headless_WindowSystem window(_settings, _logger);
    EXPECT_EQ(window.GetCursorShape(), CursorShape::Default);

    window.SetCursorShape(CursorShape::Text);
    EXPECT_EQ(window.GetCursorShape(), CursorShape::Text);
  }
} // namespace
