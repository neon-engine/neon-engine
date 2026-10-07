#include "display-options.hpp"

#include <initializer_list>
#include <string>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "runtime-options.hpp"

namespace
{
  using neon::CommandLine;
  using neon::DisplayOptions;
  using neon::RuntimeOptions;
  using ::testing::HasSubstr;

  /// The two sets the runtime owns, and nothing of the editor's.
  class DisplayOptionsTest : public ::testing::Test
  {
  protected:
    CommandLine _command_line{"NeonRuntime", "Runs a Neon Engine project."};
    RuntimeOptions _runtime_options;
    DisplayOptions _options;
    SettingsConfig _settings{
      .width = 1920,
      .height = 1080,
      .selected_api = RenderingApi::Vulkan,
      .window_mode = WindowMode::Borderless
    };
    std::string _error;

    void SetUp() override
    {
      _runtime_options.Register(_command_line);
      _options.Register(_command_line);
    }

    bool Parse(const std::initializer_list<const char *> arguments)
    {
      std::vector<const char *> argv{"NeonRuntime"};
      argv.insert(argv.end(), arguments.begin(), arguments.end());
      return _command_line.Parse(static_cast<int>(argv.size()), argv.data());
    }

    bool Apply(const std::initializer_list<const char *> arguments)
    {
      if (!Parse(arguments))
      {
        ADD_FAILURE() << "The parser refused the arguments: " << _command_line.GetError();
        return false;
      }

      return _runtime_options.Apply(_command_line, _settings, _error) &&
             _options.Apply(_command_line, _settings, _error);
    }
  };

  TEST_F(DisplayOptionsTest, ListsItsOptionsInTheHelpText)
  {
    const std::string help = _command_line.GetHelp();

    EXPECT_THAT(help, HasSubstr("Display:\n"));
    EXPECT_THAT(help, HasSubstr("--window-size WxH"));
    EXPECT_THAT(help, HasSubstr("--window-mode MODE"));
    EXPECT_THAT(help, HasSubstr("--ui-scale NUMBER"));
  }

  TEST_F(DisplayOptionsTest, TheOptionsOfTheHeadlessRendererAreNotHere)
  {
    for (const char *option : {"--render-scale", "--input", "--input-script"})
    {
      EXPECT_FALSE(Parse({option, "value"})) << option;
      EXPECT_EQ(_command_line.GetError(), "Unknown option '" + std::string(option) + "'");
    }
  }

  TEST_F(DisplayOptionsTest, LeavesTheSettingsAsTheyAreWithoutOptions)
  {
    ASSERT_TRUE(Apply({}));

    EXPECT_EQ(_settings.width, 1920);
    EXPECT_EQ(_settings.height, 1080);
    EXPECT_EQ(_settings.window_mode, WindowMode::Borderless);
    EXPECT_EQ(_settings.ui_scale, 1.0);
  }

  TEST_F(DisplayOptionsTest, TakesTheSizeOfTheWindowAndShowsAWindowOfThatSize)
  {
    ASSERT_TRUE(Apply({"--window-size", "1280x800"}));

    EXPECT_EQ(_settings.width, 1280);
    EXPECT_EQ(_settings.height, 800);
    EXPECT_EQ(_settings.window_mode, WindowMode::Windowed);
  }

  TEST_F(DisplayOptionsTest, RefusesASizeThatIsNone)
  {
    for (const char *size : {"1280", "1280x", "x720", "0x720", "1280x-1", "wide", "1280x720x3", "12.5x10"})
    {
      CommandLine command_line{"NeonRuntime", ""};
      DisplayOptions options;
      options.Register(command_line);

      const std::string argument = std::string("--window-size=") + size;
      const char *argv[] = {"NeonRuntime", argument.c_str()};
      ASSERT_TRUE(command_line.Parse(2, argv)) << size;

      SettingsConfig settings{.width = 1, .height = 2};
      std::string error;
      EXPECT_FALSE(options.Apply(command_line, settings, error)) << size;
      EXPECT_EQ(error, "Option '--window-size' needs a width and a height above zero, such as 1280x720");
      EXPECT_EQ(settings.width, 1);
      EXPECT_EQ(settings.height, 2);
    }
  }

  TEST_F(DisplayOptionsTest, TakesTheModeOfTheWindow)
  {
    ASSERT_TRUE(Apply({"--window-mode", "windowed"}));
    EXPECT_EQ(_settings.window_mode, WindowMode::Windowed);

    ASSERT_TRUE(Apply({"--window-mode", "fullscreen"}));
    EXPECT_EQ(_settings.window_mode, WindowMode::Fullscreen);

    ASSERT_TRUE(Apply({"--window-mode=borderless"}));
    EXPECT_EQ(_settings.window_mode, WindowMode::Borderless);
  }

  TEST_F(DisplayOptionsTest, TakesTheMostFramesASecond)
  {
    ASSERT_TRUE(Apply({"--max-fps", "144"}));
    EXPECT_EQ(_settings.max_fps, 144);

    ASSERT_TRUE(Apply({"--max-fps=0"}));
    EXPECT_EQ(_settings.max_fps, 0) << "0 is no limit";

    for (const char *wrong: {"29", "301", "-1", "60.5", "fast"})
    {
      EXPECT_FALSE(Apply({"--max-fps", wrong})) << wrong;
      EXPECT_EQ(_error, "Option '--max-fps' needs 0 for no limit, or a number from 30 to 300") << wrong;
    }
  }

  TEST_F(DisplayOptionsTest, TakesTheSamplesATextureIsReadWithFromTheSide)
  {
    EXPECT_EQ(_settings.anisotropy, 8) << "unless something says otherwise";

    ASSERT_TRUE(Apply({"--anisotropy", "16"}));
    EXPECT_EQ(_settings.anisotropy, 16);

    ASSERT_TRUE(Apply({"--anisotropy=1"}));
    EXPECT_EQ(_settings.anisotropy, 1) << "1 is none";

    for (const char *wrong: {"0", "3", "32", "2.5", "sharp"})
    {
      EXPECT_FALSE(Apply({"--anisotropy", wrong})) << wrong;
      EXPECT_EQ(_error, "Option '--anisotropy' needs 1 for none, 2, 4, 8, or 16") << wrong;
    }
  }

  TEST_F(DisplayOptionsTest, TakesTheSizeTexturesAreKeptAt)
  {
    EXPECT_DOUBLE_EQ(_settings.texture_scale, 1.0) << "unless something says otherwise";

    ASSERT_TRUE(Apply({"--texture-scale", "0.25"}));
    EXPECT_DOUBLE_EQ(_settings.texture_scale, 0.25);

    ASSERT_TRUE(Apply({"--texture-scale=1"}));
    EXPECT_DOUBLE_EQ(_settings.texture_scale, 1.0);

    for (const char *wrong: {"0", "2", "0.3", "0.0625", "small"})
    {
      EXPECT_FALSE(Apply({"--texture-scale", wrong})) << wrong;
      EXPECT_EQ(_error, "Option '--texture-scale' needs 1, 0.5, 0.25, or 0.125") << wrong;
    }
  }

  TEST_F(DisplayOptionsTest, TakesWhetherAFrameWaitsForTheScreen)
  {
    ASSERT_TRUE(Apply({"--vsync", "off"}));
    EXPECT_FALSE(_settings.vertical_sync);

    ASSERT_TRUE(Apply({"--vsync=on"}));
    EXPECT_TRUE(_settings.vertical_sync);

    EXPECT_FALSE(Apply({"--vsync", "sometimes"}));
    EXPECT_EQ(_error, "Option '--vsync' needs on or off");
  }

  TEST_F(DisplayOptionsTest, TheModeWinsOverTheWindowASizeShows)
  {
    ASSERT_TRUE(Apply({"--window-size", "1920x1080", "--window-mode", "fullscreen"}));

    EXPECT_EQ(_settings.width, 1920);
    EXPECT_EQ(_settings.height, 1080);
    EXPECT_EQ(_settings.window_mode, WindowMode::Fullscreen);

    ASSERT_TRUE(Apply({"--window-mode", "fullscreen", "--window-size", "1920x1080"}));
    EXPECT_EQ(_settings.window_mode, WindowMode::Fullscreen);
  }

  TEST_F(DisplayOptionsTest, RefusesAModeThatIsNone)
  {
    for (const char *mode : {"maximised", "Windowed", "full"})
    {
      EXPECT_FALSE(Apply({"--window-mode", mode})) << mode;
      EXPECT_EQ(_error, "Option '--window-mode' needs windowed, borderless, or fullscreen");
      EXPECT_EQ(_settings.window_mode, WindowMode::Borderless);
    }
  }

  TEST_F(DisplayOptionsTest, TakesTheScaleOfTheUserInterface)
  {
    ASSERT_TRUE(Apply({"--ui-scale", "1.5"}));
    EXPECT_EQ(_settings.ui_scale, 1.5);

    ASSERT_TRUE(Apply({"--ui-scale", "0.75"}));
    EXPECT_EQ(_settings.ui_scale, 0.75);
  }

  TEST_F(DisplayOptionsTest, RefusesAScaleOfTheUserInterfaceThatIsOutOfRange)
  {
    for (const char *scale : {"0", "-2", "8.5", "large"})
    {
      EXPECT_FALSE(Apply({"--ui-scale", scale})) << scale;
      EXPECT_EQ(_error, "Option '--ui-scale' needs a number from 0.25 to 8, such as 1.5");
      EXPECT_EQ(_settings.ui_scale, 1.0);
    }
  }
} // namespace
