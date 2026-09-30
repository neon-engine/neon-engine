#include "sdl2-window-system.hpp"

#include <string>

#include <gtest/gtest.h>

#include <neon/testing/sdl2-without-display.hpp>

// What of the window needs no renderer: what SDL is told before it starts,
// the flags of the window, its sizes, and the shapes of the cursor. The
// window itself asks for Vulkan, which the video driver that draws nowhere
// does not have.

namespace
{
  using neon::CursorShape;
  using neon::SDL2_WindowSystem;
  using neon::WindowMetrics;
  using neon::testing::Sdl2WithoutDisplay;

  TEST(SDL2WindowSystemTest, AsksForAllPixelsOfADisplayOfHighDensityInEveryMode)
  {
    for (const WindowMode mode : {WindowMode::Windowed, WindowMode::Borderless, WindowMode::Fullscreen})
    {
      const int flags = SDL2_WindowSystem::WindowFlagsOf(SettingsConfig{.window_mode = mode});

      EXPECT_NE(flags & SDL_WINDOW_ALLOW_HIGHDPI, 0) << static_cast<int>(mode);
      EXPECT_NE(flags & SDL_WINDOW_SHOWN, 0) << static_cast<int>(mode);
    }
  }

  TEST(SDL2WindowSystemTest, CoversTheDisplayOnlyWhenAskedTo)
  {
    const int windowed = SDL2_WindowSystem::WindowFlagsOf(SettingsConfig{.window_mode = WindowMode::Windowed});
    const int borderless = SDL2_WindowSystem::WindowFlagsOf(SettingsConfig{.window_mode = WindowMode::Borderless});
    const int fullscreen = SDL2_WindowSystem::WindowFlagsOf(SettingsConfig{.window_mode = WindowMode::Fullscreen});

    EXPECT_EQ(windowed & SDL_WINDOW_FULLSCREEN_DESKTOP, 0);

    EXPECT_EQ(borderless & SDL_WINDOW_FULLSCREEN_DESKTOP, SDL_WINDOW_FULLSCREEN_DESKTOP);

    EXPECT_EQ(fullscreen & SDL_WINDOW_FULLSCREEN, SDL_WINDOW_FULLSCREEN);
    EXPECT_NE(fullscreen & SDL_WINDOW_FULLSCREEN_DESKTOP, SDL_WINDOW_FULLSCREEN_DESKTOP);
  }

  TEST(SDL2WindowSystemTest, DeclaresTheProcessAwareOfTheDensityOfEveryMonitor)
  {
    SDL2_WindowSystem::SetHints();

    const char *scaling = SDL_GetHint(SDL_HINT_WINDOWS_DPI_SCALING);
    ASSERT_NE(scaling, nullptr);
    EXPECT_EQ(std::string(scaling), "1");
  }

  TEST(SDL2WindowSystemTest, AsksForTextOfAnyLengthFromAnInputMethod)
  {
    SDL2_WindowSystem::SetHints();

    const char *extended = SDL_GetHint(SDL_HINT_IME_SUPPORT_EXTENDED_TEXT);
    ASSERT_NE(extended, nullptr);
    EXPECT_EQ(std::string(extended), "1");
  }

  TEST(SDL2WindowSystemTest, KnowsACursorOfTheSystemForEveryShape)
  {
    EXPECT_EQ(SDL2_WindowSystem::SystemCursorOf(CursorShape::Default), SDL_SYSTEM_CURSOR_ARROW);
    EXPECT_EQ(SDL2_WindowSystem::SystemCursorOf(CursorShape::Pointer), SDL_SYSTEM_CURSOR_HAND);
    EXPECT_EQ(SDL2_WindowSystem::SystemCursorOf(CursorShape::Text), SDL_SYSTEM_CURSOR_IBEAM);
    EXPECT_EQ(SDL2_WindowSystem::SystemCursorOf(CursorShape::Wait), SDL_SYSTEM_CURSOR_WAIT);
    EXPECT_EQ(SDL2_WindowSystem::SystemCursorOf(CursorShape::Progress), SDL_SYSTEM_CURSOR_WAITARROW);
    EXPECT_EQ(SDL2_WindowSystem::SystemCursorOf(CursorShape::Crosshair), SDL_SYSTEM_CURSOR_CROSSHAIR);
    EXPECT_EQ(SDL2_WindowSystem::SystemCursorOf(CursorShape::Move), SDL_SYSTEM_CURSOR_SIZEALL);
    EXPECT_EQ(SDL2_WindowSystem::SystemCursorOf(CursorShape::NotAllowed), SDL_SYSTEM_CURSOR_NO);
    EXPECT_EQ(SDL2_WindowSystem::SystemCursorOf(CursorShape::EwResize), SDL_SYSTEM_CURSOR_SIZEWE);
    EXPECT_EQ(SDL2_WindowSystem::SystemCursorOf(CursorShape::NsResize), SDL_SYSTEM_CURSOR_SIZENS);
    EXPECT_EQ(SDL2_WindowSystem::SystemCursorOf(CursorShape::NeswResize), SDL_SYSTEM_CURSOR_SIZENESW);
    EXPECT_EQ(SDL2_WindowSystem::SystemCursorOf(CursorShape::NwseResize), SDL_SYSTEM_CURSOR_SIZENWSE);

    // SDL has no hand that grabs
    EXPECT_EQ(SDL2_WindowSystem::SystemCursorOf(CursorShape::Grab), SDL_SYSTEM_CURSOR_HAND);
    EXPECT_EQ(SDL2_WindowSystem::SystemCursorOf(CursorShape::Grabbing), SDL_SYSTEM_CURSOR_HAND);

    EXPECT_EQ(SDL2_WindowSystem::SystemCursorOf(CursorShape::None), -1);
  }

  class SDL2WindowMetricsTest : public Sdl2WithoutDisplay {};

  TEST_F(SDL2WindowMetricsTest, ReadsTheSizeOfTheWindowInPoints)
  {
    // the window of the test has 640 by 360 points
    const WindowMetrics metrics = SDL2_WindowSystem::MetricsOf(_window, 1280, 720);

    EXPECT_EQ(metrics.point_width, 640);
    EXPECT_EQ(metrics.point_height, 360);
    EXPECT_EQ(metrics.pixel_width, 1280);
    EXPECT_EQ(metrics.pixel_height, 720);
    EXPECT_DOUBLE_EQ(metrics.Density(), 2.0);
  }

  TEST_F(SDL2WindowMetricsTest, FollowsAWindowThatWasResized)
  {
    SDL_SetWindowSize(_window, 800, 600);

    const WindowMetrics metrics = SDL2_WindowSystem::MetricsOf(_window, 1000, 750);

    EXPECT_EQ(metrics.point_width, 800);
    EXPECT_EQ(metrics.point_height, 600);
    EXPECT_DOUBLE_EQ(metrics.ScaleX(), 1.25);
    EXPECT_DOUBLE_EQ(metrics.ScaleY(), 1.25);
  }

  TEST_F(SDL2WindowMetricsTest, HasNoSizeWithoutAWindow)
  {
    EXPECT_EQ(SDL2_WindowSystem::MetricsOf(nullptr, 1280, 720), WindowMetrics{});
  }
} // namespace
