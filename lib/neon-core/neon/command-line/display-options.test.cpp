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

  /// The options next to those of the runtime, as an application has them.
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

    bool Apply(const std::initializer_list<const char *> arguments)
    {
      std::vector<const char *> argv{"NeonRuntime"};
      argv.insert(argv.end(), arguments.begin(), arguments.end());

      if (!_command_line.Parse(static_cast<int>(argv.size()), argv.data()))
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
    EXPECT_THAT(help, HasSubstr("--render-scale NUMBER"));
    EXPECT_THAT(help, HasSubstr("--ui-scale NUMBER"));
    EXPECT_THAT(help, HasSubstr("--input SCRIPT"));
    EXPECT_THAT(help, HasSubstr("--input-script PATH"));
  }

  TEST_F(DisplayOptionsTest, LeavesTheSettingsAsTheyAreWithoutOptions)
  {
    ASSERT_TRUE(Apply({}));

    EXPECT_EQ(_settings.width, 1920);
    EXPECT_EQ(_settings.height, 1080);
    EXPECT_EQ(_settings.window_mode, WindowMode::Borderless);
    EXPECT_EQ(_settings.render_scale, 1.0);
    EXPECT_EQ(_settings.ui_scale, 1.0);
    EXPECT_TRUE(_settings.input_script.empty());
    EXPECT_TRUE(_settings.input_script_path.empty());
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

  TEST_F(DisplayOptionsTest, TakesTheRenderScaleWithoutAWindow)
  {
    ASSERT_TRUE(Apply({"--headless", "--render-scale", "1.25"}));
    EXPECT_EQ(_settings.render_scale, 1.25);
  }

  TEST_F(DisplayOptionsTest, RefusesARenderScaleWithAWindow)
  {
    EXPECT_FALSE(Apply({"--render-scale", "2"}));
    EXPECT_EQ(_error, "Option '--render-scale' needs '--headless'. A window takes the density of its display");
    EXPECT_EQ(_settings.render_scale, 1.0);
  }

  TEST_F(DisplayOptionsTest, RefusesARenderScaleThatIsOutOfRange)
  {
    for (const char *scale : {"0", "-1", "0.1", "9", "twice"})
    {
      _settings.render_scale = 1.0;
      EXPECT_FALSE(Apply({"--headless", "--render-scale", scale})) << scale;
      EXPECT_EQ(_error, "Option '--render-scale' needs a number from 0.25 to 8, such as 2");
      EXPECT_EQ(_settings.render_scale, 1.0);
    }
  }

  TEST_F(DisplayOptionsTest, TakesTheScaleOfTheUserInterfaceWithAndWithoutAWindow)
  {
    ASSERT_TRUE(Apply({"--ui-scale", "1.5"}));
    EXPECT_EQ(_settings.ui_scale, 1.5);

    ASSERT_TRUE(Apply({"--headless", "--ui-scale", "0.75"}));
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

  TEST_F(DisplayOptionsTest, TakesAScriptOfInputWithoutAWindow)
  {
    ASSERT_TRUE(Apply({"--headless", "--input", "1: pointer 10 10; 2: click"}));
    EXPECT_EQ(_settings.input_script, "1: pointer 10 10; 2: click");

    ASSERT_TRUE(Apply({"--headless", "--input-script", "assets://input/menu.input"}));
    EXPECT_EQ(_settings.input_script_path, "assets://input/menu.input");
  }

  TEST_F(DisplayOptionsTest, RefusesAScriptOfInputWithAWindow)
  {
    EXPECT_FALSE(Apply({"--input", "1: click"}));
    EXPECT_EQ(_error, "Option '--input' needs '--headless'. A window takes its input from devices");

    EXPECT_FALSE(Apply({"--input-script", "assets://a.input"}));
    EXPECT_EQ(_error, "Option '--input-script' needs '--headless'. A window takes its input from devices");
  }

  TEST_F(DisplayOptionsTest, RefusesTwoScriptsOfInput)
  {
    EXPECT_FALSE(Apply({"--headless", "--input", "1: click", "--input-script", "assets://a.input"}));
    EXPECT_EQ(_error, "Option '--input' and '--input-script' cannot be given together");
  }
} // namespace
