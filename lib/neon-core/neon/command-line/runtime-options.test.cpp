#include "runtime-options.hpp"

#include <initializer_list>
#include <string>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace
{
  using neon::ApiVersion;
  using neon::CommandLine;
  using neon::RuntimeOptions;
  using ::testing::ElementsAre;
  using ::testing::IsEmpty;

  /// Takes arguments the way an application does: the options are declared,
  /// the arguments parsed, and the result carried over into the settings.
  class RuntimeOptionsTest : public ::testing::Test
  {
  protected:
    CommandLine _command_line{"NeonRuntime", "Runs a Neon Engine project."};
    RuntimeOptions _options;
    SettingsConfig _settings{.width = 1920, .height = 1080, .selected_api = RenderingApi::Vulkan};
    std::string _error;

    void SetUp() override
    {
      _options.Register(_command_line);
    }

    bool Parse(const std::initializer_list<const char *> arguments)
    {
      std::vector<const char *> argv{"NeonRuntime"};
      argv.insert(argv.end(), arguments.begin(), arguments.end());
      return _command_line.Parse(static_cast<int>(argv.size()), argv.data());
    }

    /// Returns what Apply returns. Fails the test when the parser refuses
    /// the arguments, which is not what these tests are about.
    bool Apply(const std::initializer_list<const char *> arguments)
    {
      if (!Parse(arguments))
      {
        ADD_FAILURE() << "The parser refused the arguments: " << _command_line.GetError();
        return false;
      }
      return _options.Apply(_command_line, _settings, _error);
    }
  };

  // Register

  TEST_F(RuntimeOptionsTest, HelpTextIsTheOneTheDevelopmentGuideShows)
  {
    EXPECT_EQ(
      _command_line.GetHelp(),
      "Runs a Neon Engine project.\n"
      "\n"
      "Usage: NeonRuntime [options]\n"
      "\n"
      "  --help                    Show this text\n"
      "  --scene PATH              Scene to start with, for example assets://scenes/demo.scene.yml\n"
      "  --ui PATH                 User interface to show on top, for example assets://ui/hud.ui.yml\n"
      "  --renderer vulkan         Renderer to draw with. Default: vulkan\n"
      "  --vulkan-version 1.N      Highest version of Vulkan to render with, for example 1.2. Default: 1.3\n"
      "\n"
      "Development:\n"
      "  --frames N                Stop after N frames\n"
      "  --screenshot PATH         Save the last frame as a PNG image, for example output://frame.png. "
      "Needs --frames or --screenshot-at\n"
      "  --screenshot-at N[,N...]  Save these frames instead of the last one, counted from 1. "
      "Each file gets its frame in its name, as in frame-0030.png. Needs --screenshot\n"
      "  --output-dir DIR          Folder of this machine that output:// stands for. Created when missing\n"
      "  --time-step SECONDS       Advance the game by this much time in every frame, for example 0.016667, "
      "so that a run gives the same frames every time\n"
      "  --headless-renderer       Render without a window, for screenshots and checks on a machine with no display\n"
      "  --spawn PATH              Spawn this prefab at the top of the world once the scene is read, as a script "
      "would, for example assets://prefabs/target.prefab.yml\n"
      "  --jit on|off              Compile the scripts as they run, or run them in LuaJIT's interpreter. Over "
      "scripting.jit of the settings, for comparing the two\n"
      "  --headless                Run as a dedicated server. Not available yet, see --headless-renderer\n");
  }

  TEST_F(RuntimeOptionsTest, UiNamesTheUserInterfaceToShow)
  {
    ASSERT_TRUE(Apply({"--ui", "assets://ui/hud.ui.yml"}));

    EXPECT_EQ(_settings.ui_path, "assets://ui/hud.ui.yml");
    EXPECT_EQ(_error, "");
  }

  TEST_F(RuntimeOptionsTest, UiCanBeWrittenWithAnEqualsSign)
  {
    ASSERT_TRUE(Apply({"--ui=assets://ui/hud.ui.yml"}));

    EXPECT_EQ(_settings.ui_path, "assets://ui/hud.ui.yml");
  }

  TEST_F(RuntimeOptionsTest, UiLeavesTheSceneAsItIs)
  {
    const std::string scene = _settings.scene_path;

    ASSERT_TRUE(Apply({"--ui", "assets://ui/hud.ui.yml"}));

    EXPECT_EQ(_settings.scene_path, scene);
  }

  TEST_F(RuntimeOptionsTest, RendererAcceptsVulkan)
  {
    EXPECT_TRUE(Parse({"--renderer", "vulkan"}));
  }

  TEST_F(RuntimeOptionsTest, RendererRefusesWhatIsNotARenderer)
  {
    EXPECT_FALSE(Parse({"--renderer", "opengl"}));
    EXPECT_EQ(_command_line.GetError(), "'opengl' is not a value of '--renderer'. It accepts vulkan");
  }

  TEST_F(RuntimeOptionsTest, HeadlessRendererTakesNoValue)
  {
    EXPECT_FALSE(Parse({"--headless-renderer=yes"}));
    EXPECT_EQ(_command_line.GetError(), "Option '--headless-renderer' takes no value");
  }

  TEST_F(RuntimeOptionsTest, EveryOptionButTheHeadlessOnesNeedsAValue)
  {
    for (const char *option : {"--ui", "--renderer", "--frames", "--screenshot", "--screenshot-at",
                               "--output-dir", "--time-step", "--vulkan-version", "--spawn", "--jit"})
    {
      EXPECT_FALSE(Parse({option})) << option;
      EXPECT_EQ(_command_line.GetError(), "Option '" + std::string(option) + "' needs a value");
    }
  }

  TEST_F(RuntimeOptionsTest, CanBeRegisteredTwice)
  {
    const std::string help = _command_line.GetHelp();

    _options.Register(_command_line);

    EXPECT_EQ(_command_line.GetHelp(), help);
  }

  // Apply, without arguments

  TEST_F(RuntimeOptionsTest, LeavesTheSettingsAsTheyAreWithoutArguments)
  {
    ASSERT_TRUE(Apply({}));

    EXPECT_EQ(_error, "");
    EXPECT_EQ(_settings.selected_api, RenderingApi::Vulkan);
    EXPECT_FALSE(_settings.headless_renderer);
    EXPECT_EQ(_settings.max_frames, 0u);
    EXPECT_EQ(_settings.time_step, 0.0);
    EXPECT_EQ(_settings.vulkan_version, (ApiVersion{1, 3}));
    EXPECT_EQ(_settings.output_directory, "");
    EXPECT_EQ(_settings.screenshot_path, "");
    EXPECT_EQ(_settings.ui_path, "");
    EXPECT_THAT(_settings.screenshot_frames, IsEmpty());
    EXPECT_EQ(_settings.width, 1920);
    EXPECT_EQ(_settings.height, 1080);
  }

  TEST_F(RuntimeOptionsTest, KeepsWhatTheApplicationSetWhenAnOptionIsNotGiven)
  {
    _settings.max_frames = 5;
    _settings.time_step = 0.5;
    _settings.output_directory = "somewhere";

    ASSERT_TRUE(Apply({}));

    EXPECT_EQ(_settings.max_frames, 5u);
    EXPECT_EQ(_settings.time_step, 0.5);
    EXPECT_EQ(_settings.output_directory, "somewhere");
  }

  // --renderer

  TEST_F(RuntimeOptionsTest, RendererSelectsVulkan)
  {
    ASSERT_TRUE(Apply({"--renderer=vulkan"}));

    EXPECT_EQ(_settings.selected_api, RenderingApi::Vulkan);
  }

  // --headless-renderer

  TEST_F(RuntimeOptionsTest, HeadlessRendererIsCarriedOver)
  {
    ASSERT_TRUE(Apply({"--headless-renderer"}));

    EXPECT_TRUE(_settings.headless_renderer);
  }

  TEST_F(RuntimeOptionsTest, HeadlessRendererIsTurnedOffWhenItIsNotGiven)
  {
    _settings.headless_renderer = true;

    ASSERT_TRUE(Apply({}));

    EXPECT_FALSE(_settings.headless_renderer);
  }

  // --headless

  TEST_F(RuntimeOptionsTest, HeadlessIsRefusedUntilThereIsADedicatedServer)
  {
    EXPECT_FALSE(Apply({"--headless"}));

    EXPECT_EQ(
      _error,
      "'--headless' is for a dedicated server, which does not exist yet (#144). "
      "To render without a window, use '--headless-renderer'");
  }

  TEST_F(RuntimeOptionsTest, HeadlessIsNotTheHeadlessRenderer)
  {
    EXPECT_FALSE(Apply({"--headless"}));

    EXPECT_FALSE(_settings.headless_renderer);
  }

  // --output-dir

  TEST_F(RuntimeOptionsTest, OutputDirectoryIsCarriedOverAsItWasGiven)
  {
    ASSERT_TRUE(Apply({"--output-dir", "../some where/shots"}));

    EXPECT_EQ(_settings.output_directory, "../some where/shots");
  }

  // --spawn

  TEST_F(RuntimeOptionsTest, SpawnIsCarriedOverAsItWasGiven)
  {
    ASSERT_TRUE(Apply({"--spawn", "assets://prefabs/target.prefab.yml"}));

    EXPECT_EQ(_settings.spawn_path, "assets://prefabs/target.prefab.yml");
  }

  TEST_F(RuntimeOptionsTest, SpawnsNothingUnlessAsked)
  {
    ASSERT_TRUE(Apply({}));

    EXPECT_TRUE(_settings.spawn_path.empty());
  }

  // --frames

  TEST_F(RuntimeOptionsTest, FramesAreCarriedOver)
  {
    ASSERT_TRUE(Apply({"--frames", "60"}));

    EXPECT_EQ(_settings.max_frames, 60u);
  }

  TEST_F(RuntimeOptionsTest, FramesAcceptOne)
  {
    ASSERT_TRUE(Apply({"--frames=1"}));

    EXPECT_EQ(_settings.max_frames, 1u);
  }

  class RuntimeOptionsBadFrames : public RuntimeOptionsTest, public ::testing::WithParamInterface<const char *> {};

  TEST_P(RuntimeOptionsBadFrames, AreRefused)
  {
    EXPECT_FALSE(Apply({"--frames", GetParam()}));

    EXPECT_EQ(_error, "Option '--frames' needs a whole number above zero");
    EXPECT_EQ(_settings.max_frames, 0u);
  }

  INSTANTIATE_TEST_SUITE_P(
    RuntimeOptions,
    RuntimeOptionsBadFrames,
    ::testing::Values("0", "-1", "abc", "1.5", "10abc", "1,2", "99999999999999999999999999"));

  // --vulkan-version

  TEST_F(RuntimeOptionsTest, VulkanVersionIsCarriedOver)
  {
    ASSERT_TRUE(Apply({"--vulkan-version", "1.2"}));

    EXPECT_EQ(_settings.vulkan_version, (ApiVersion{1, 2}));
  }

  TEST_F(RuntimeOptionsTest, VulkanVersionAcceptsAMinorOfTwoDigits)
  {
    ASSERT_TRUE(Apply({"--vulkan-version=1.10"}));

    EXPECT_EQ(_settings.vulkan_version, (ApiVersion{1, 10}));
  }

  // The renderer says whether a version is enough, not the command line,
  // so that a version that comes from elsewhere is checked the same way.
  TEST_F(RuntimeOptionsTest, VulkanVersionAcceptsAVersionBelowWhatTheRendererNeeds)
  {
    ASSERT_TRUE(Apply({"--vulkan-version", "1.0"}));

    EXPECT_EQ(_settings.vulkan_version, (ApiVersion{1, 0}));
  }

  TEST_F(RuntimeOptionsTest, VulkanVersionKeepsTheOneTheApplicationSetWhenItIsNotGiven)
  {
    _settings.vulkan_version = {1, 1};

    ASSERT_TRUE(Apply({}));

    EXPECT_EQ(_settings.vulkan_version, (ApiVersion{1, 1}));
  }

  class RuntimeOptionsBadVulkanVersion : public RuntimeOptionsTest,
                                         public ::testing::WithParamInterface<const char *> {};

  TEST_P(RuntimeOptionsBadVulkanVersion, IsRefused)
  {
    EXPECT_FALSE(Apply({"--vulkan-version", GetParam()}));

    EXPECT_EQ(_error, "Option '--vulkan-version' needs a version of Vulkan 1, such as 1.3");
    EXPECT_EQ(_settings.vulkan_version, (ApiVersion{1, 3}));
  }

  INSTANTIATE_TEST_SUITE_P(
    RuntimeOptions,
    RuntimeOptionsBadVulkanVersion,
    ::testing::Values("1", "2.0", "1.", ".3", "1.3.1", "1.-1", "v1.3", "1,3", "1.3a", "latest"));

  // --jit

  TEST_F(RuntimeOptionsTest, JitOffRunsTheScriptsInTheInterpreter)
  {
    ASSERT_TRUE(Apply({"--jit", "off"}));
    EXPECT_FALSE(_settings.script_jit);
  }

  TEST_F(RuntimeOptionsTest, JitOnCompilesOverASettingThatSaidOff)
  {
    _settings.script_jit = false;
    ASSERT_TRUE(Apply({"--jit=on"}));
    EXPECT_TRUE(_settings.script_jit);
  }

  TEST_F(RuntimeOptionsTest, JitLeftOutLeavesTheSetting)
  {
    _settings.script_jit = false;
    ASSERT_TRUE(Apply({}));
    EXPECT_FALSE(_settings.script_jit);
  }

  TEST_F(RuntimeOptionsTest, JitRefusesAnythingButOnAndOff)
  {
    EXPECT_FALSE(Parse({"--jit", "maybe"}));
    EXPECT_EQ(_command_line.GetError(), "'maybe' is not a value of '--jit'. It accepts on, off");
  }

  // --time-step

  TEST_F(RuntimeOptionsTest, TimeStepIsCarriedOver)
  {
    ASSERT_TRUE(Apply({"--time-step", "0.016667"}));

    EXPECT_DOUBLE_EQ(_settings.time_step, 0.016667);
  }

  TEST_F(RuntimeOptionsTest, TimeStepAcceptsAWholeNumber)
  {
    ASSERT_TRUE(Apply({"--time-step=2"}));

    EXPECT_DOUBLE_EQ(_settings.time_step, 2.0);
  }

  class RuntimeOptionsBadTimeStep : public RuntimeOptionsTest, public ::testing::WithParamInterface<const char *> {};

  TEST_P(RuntimeOptionsBadTimeStep, IsRefused)
  {
    EXPECT_FALSE(Apply({"--time-step", GetParam()}));

    EXPECT_EQ(_error, "Option '--time-step' needs a number of seconds above zero, such as 0.016667");
    EXPECT_EQ(_settings.time_step, 0.0);
  }

  INSTANTIATE_TEST_SUITE_P(
    RuntimeOptions,
    RuntimeOptionsBadTimeStep,
    ::testing::Values("0", "0.0", "-0.5", "abc", "0,5", "1e-3", "."));

  // --screenshot

  TEST_F(RuntimeOptionsTest, ScreenshotIsCarriedOverWithFrames)
  {
    ASSERT_TRUE(Apply({"--frames", "3", "--screenshot", "user://frame.png"}));

    EXPECT_EQ(_settings.screenshot_path, "user://frame.png");
    EXPECT_EQ(_settings.max_frames, 3u);
    EXPECT_THAT(_settings.screenshot_frames, IsEmpty());
  }

  TEST_F(RuntimeOptionsTest, ScreenshotDoesNotDependOnTheOrderOfTheOptions)
  {
    ASSERT_TRUE(Apply({"--screenshot", "user://frame.png", "--frames", "3"}));

    EXPECT_EQ(_settings.screenshot_path, "user://frame.png");
    EXPECT_EQ(_settings.max_frames, 3u);
  }

  TEST_F(RuntimeOptionsTest, ScreenshotIsRefusedWithoutFramesOrScreenshotAt)
  {
    EXPECT_FALSE(Apply({"--screenshot", "user://frame.png"}));

    EXPECT_EQ(_error, "Option '--screenshot' needs '--frames' or '--screenshot-at'");
    EXPECT_EQ(_settings.screenshot_path, "");
  }

  TEST_F(RuntimeOptionsTest, ScreenshotAcceptsTheFramesTheApplicationSet)
  {
    _settings.max_frames = 10;

    ASSERT_TRUE(Apply({"--screenshot", "user://frame.png"}));

    EXPECT_EQ(_settings.screenshot_path, "user://frame.png");
  }

  TEST_F(RuntimeOptionsTest, ScreenshotToOutputIsCarriedOverWithAnOutputDirectory)
  {
    ASSERT_TRUE(Apply({"--frames", "3", "--output-dir", "shots", "--screenshot", "output://frame.png"}));

    EXPECT_EQ(_settings.screenshot_path, "output://frame.png");
    EXPECT_EQ(_settings.output_directory, "shots");
  }

  TEST_F(RuntimeOptionsTest, ScreenshotToOutputIsRefusedWithoutAnOutputDirectory)
  {
    EXPECT_FALSE(Apply({"--frames", "3", "--screenshot", "output://frame.png"}));

    EXPECT_EQ(_error, "'output://frame.png' needs '--output-dir', which says where output:// is");
  }

  TEST_F(RuntimeOptionsTest, ScreenshotToOutputAcceptsTheOutputDirectoryTheApplicationSet)
  {
    _settings.output_directory = "shots";

    EXPECT_TRUE(Apply({"--frames", "3", "--screenshot", "output://frame.png"}));
  }

  TEST_F(RuntimeOptionsTest, ScreenshotPathIsNotCheckedAgainstThePathRules)
  {
    // the file system refuses it when the frame is written
    ASSERT_TRUE(Apply({"--frames", "3", "--screenshot", "frame.png"}));

    EXPECT_EQ(_settings.screenshot_path, "frame.png");
  }

  // --screenshot-at

  TEST_F(RuntimeOptionsTest, ScreenshotAtIsCarriedOver)
  {
    ASSERT_TRUE(Apply({"--screenshot", "user://frame.png", "--screenshot-at", "1,30,60"}));

    EXPECT_THAT(_settings.screenshot_frames, ElementsAre(1u, 30u, 60u));
    EXPECT_EQ(_settings.screenshot_path, "user://frame.png");
  }

  TEST_F(RuntimeOptionsTest, ScreenshotAtGivesTheSameFramesWhenAppliedTwice)
  {
    // the application applies the command line again after its settings
    // files, so that the command line wins
    ASSERT_TRUE(Apply({"--screenshot", "user://frame.png", "--screenshot-at", "1,30,60"}));
    ASSERT_TRUE(_options.Apply(_command_line, _settings, _error));

    EXPECT_THAT(_settings.screenshot_frames, ElementsAre(1u, 30u, 60u));
    EXPECT_EQ(_settings.max_frames, 60u);
  }

  TEST_F(RuntimeOptionsTest, ScreenshotAtAcceptsOneFrame)
  {
    ASSERT_TRUE(Apply({"--screenshot", "user://frame.png", "--screenshot-at=7"}));

    EXPECT_THAT(_settings.screenshot_frames, ElementsAre(7u));
    EXPECT_EQ(_settings.max_frames, 7u);
  }

  TEST_F(RuntimeOptionsTest, ScreenshotAtPutsTheFramesInRisingOrder)
  {
    ASSERT_TRUE(Apply({"--screenshot", "user://frame.png", "--screenshot-at", "60,1,30"}));

    EXPECT_THAT(_settings.screenshot_frames, ElementsAre(1u, 30u, 60u));
  }

  TEST_F(RuntimeOptionsTest, ScreenshotAtKeepsAFrameThatIsListedTwiceOnce)
  {
    ASSERT_TRUE(Apply({"--screenshot", "user://frame.png", "--screenshot-at", "30,1,30,1,30"}));

    EXPECT_THAT(_settings.screenshot_frames, ElementsAre(1u, 30u));
  }

  TEST_F(RuntimeOptionsTest, ScreenshotAtMakesTheRunAsLongAsItsHighestFrame)
  {
    ASSERT_TRUE(Apply({"--screenshot", "user://frame.png", "--screenshot-at", "10,30"}));

    EXPECT_EQ(_settings.max_frames, 30u);
  }

  TEST_F(RuntimeOptionsTest, ScreenshotAtLeavesALongerRunAsLongAsItIs)
  {
    ASSERT_TRUE(Apply({"--frames", "60", "--screenshot", "user://frame.png", "--screenshot-at", "10,30"}));

    EXPECT_EQ(_settings.max_frames, 60u);
    EXPECT_THAT(_settings.screenshot_frames, ElementsAre(10u, 30u));
  }

  TEST_F(RuntimeOptionsTest, ScreenshotAtAcceptsTheLastFrameOfTheRun)
  {
    ASSERT_TRUE(Apply({"--frames", "30", "--screenshot", "user://frame.png", "--screenshot-at", "30"}));

    EXPECT_EQ(_settings.max_frames, 30u);
  }

  TEST_F(RuntimeOptionsTest, ScreenshotAtIsRefusedWhenAFrameIsNeverReached)
  {
    EXPECT_FALSE(Apply({"--frames", "10", "--screenshot", "user://frame.png", "--screenshot-at", "5,11"}));

    EXPECT_EQ(_error, "Frame 11 of '--screenshot-at' is never reached, '--frames' stops after 10");
  }

  TEST_F(RuntimeOptionsTest, ScreenshotAtIsRefusedWithoutScreenshot)
  {
    EXPECT_FALSE(Apply({"--screenshot-at", "1,2"}));

    EXPECT_EQ(_error, "Option '--screenshot-at' needs '--screenshot'");
    EXPECT_THAT(_settings.screenshot_frames, IsEmpty());
  }

  TEST_F(RuntimeOptionsTest, ScreenshotAtIsRefusedWithoutScreenshotEvenWithFrames)
  {
    EXPECT_FALSE(Apply({"--frames", "10", "--screenshot-at", "1,2"}));

    EXPECT_EQ(_error, "Option '--screenshot-at' needs '--screenshot'");
  }

  TEST_F(RuntimeOptionsTest, ScreenshotAtToOutputIsRefusedWithoutAnOutputDirectory)
  {
    EXPECT_FALSE(Apply({"--screenshot", "output://frame.png", "--screenshot-at", "1"}));

    EXPECT_EQ(_error, "'output://frame.png' needs '--output-dir', which says where output:// is");
  }

  class RuntimeOptionsBadScreenshotAt : public RuntimeOptionsTest,
                                        public ::testing::WithParamInterface<const char *> {};

  TEST_P(RuntimeOptionsBadScreenshotAt, IsRefused)
  {
    EXPECT_FALSE(Apply({"--screenshot", "user://frame.png", "--screenshot-at", GetParam()}));

    EXPECT_EQ(_error, "Option '--screenshot-at' needs whole numbers above zero, separated by commas");
  }

  INSTANTIATE_TEST_SUITE_P(
    RuntimeOptions,
    RuntimeOptionsBadScreenshotAt,
    ::testing::Values(
      "0",
      "1,0",
      "-1",
      "abc",
      "1,abc",
      "1.5",
      "1,,2",
      ",1",
      "1,",
      ",",
      "1, 2",
      "1;2",
      "10abc",
      "99999999999999999999999999"));

  // everything together

  TEST_F(RuntimeOptionsTest, CarriesOverEveryOptionAtOnce)
  {
    ASSERT_TRUE(
      Apply({
        "--renderer", "vulkan",
        "--headless-renderer",
        "--frames", "60",
        "--output-dir", "shots",
        "--screenshot", "output://frame.png",
        "--screenshot-at", "10,30",
        "--time-step", "0.05"
        }));

    EXPECT_EQ(_error, "");
    EXPECT_EQ(_settings.selected_api, RenderingApi::Vulkan);
    EXPECT_TRUE(_settings.headless_renderer);
    EXPECT_EQ(_settings.max_frames, 60u);
    EXPECT_EQ(_settings.output_directory, "shots");
    EXPECT_EQ(_settings.screenshot_path, "output://frame.png");
    EXPECT_THAT(_settings.screenshot_frames, ElementsAre(10u, 30u));
    EXPECT_DOUBLE_EQ(_settings.time_step, 0.05);
  }
}
