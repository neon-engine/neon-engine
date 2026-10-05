#include "editor-options.hpp"

#include <initializer_list>
#include <string>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "display-options.hpp"
#include "runtime-options.hpp"

namespace
{
  using neon::ApiVersion;
  using neon::CommandLine;
  using neon::EditorOptions;
  using neon::DisplayOptions;
  using neon::RuntimeOptions;
  using ::testing::ElementsAre;
  using ::testing::IsEmpty;

  /// Takes arguments the way NeonRuntime does until the editor exists: the
  /// three sets are declared, the arguments parsed, and the result carried
  /// over into the settings.
  class EditorOptionsTest : public ::testing::Test
  {
  protected:
    CommandLine _command_line{"NeonRuntime", "Runs a Neon Engine project."};
    RuntimeOptions _runtime_options;
    DisplayOptions _display_options;
    EditorOptions _options;
    SettingsConfig _settings{.width = 1920, .height = 1080, .selected_api = RenderingApi::Vulkan};
    std::string _error;

    void SetUp() override
    {
      _runtime_options.Register(_command_line);
      _display_options.Register(_command_line);
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
      return _runtime_options.Apply(_command_line, _settings, _error) &&
             _display_options.Apply(_command_line, _settings, _error) &&
             _options.Apply(_command_line, _settings, _error);
    }
  };

  // Register

  TEST_F(EditorOptionsTest, HelpTextIsTheOneTheDevelopmentGuideShows)
  {
    EXPECT_EQ(
      _command_line.GetHelp(),
      "Runs a Neon Engine project.\n"
      "\n"
      "Usage: NeonRuntime [options]\n"
      "\n"
      "  --help                    Show this text\n"
      "  --renderer vulkan         Renderer to draw with. Default: vulkan\n"
      "  --vulkan-version 1.N      Highest version of Vulkan to render with, for example 1.2. Default: 1.3\n"
      "\n"
      "Display:\n"
      "  --window-size WxH         Size of the window in points, for example 1280x720. Shows a window of that "
      "size in place of one that covers the display, unless --window-mode says otherwise\n"
      "  --window-mode MODE        How the window is shown: windowed, borderless, or fullscreen, over window.mode "
      "of the settings\n"
      "  --vsync on|off            Whether a frame waits for the screen before it is shown, over rendering.vsync of "
      "the settings\n"
      "  --max-fps NUMBER          Most frames a second, from 30 to 300, or 0 for as many as can be drawn, over "
      "rendering.max_fps of the settings\n"
      "  --ui-scale NUMBER         Makes the user interface larger or smaller, for example 1.5\n"
      "\n"
      "Editor:\n"
      "  --scene PATH              Scene to start with in place of the entry scene of the project, "
      "for example assets://scenes/demo.scene.yml\n"
      "  --ui PATH                 User interface to show on top, for example assets://ui/hud.ui.yml\n"
      "  --frames N                Stop after N frames\n"
      "  --screenshot PATH         Save the last frame as a PNG image, for example output://frame.png. "
      "Needs --frames or --screenshot-at\n"
      "  --screenshot-at N[,N...]  Save these frames instead of the last one, counted from 1. "
      "Each file gets its frame in its name, as in frame-0030.png. Needs --screenshot\n"
      "  --output-dir DIR          Folder of this machine that output:// stands for. Created when missing\n"
      "  --time-step SECONDS       Advance the game by this much time in every frame, for example 0.016667, "
      "so that a run gives the same frames every time\n"
      "  --headless-renderer       Render without a window, for screenshots and checks on a machine with no display\n"
      "  --render-scale NUMBER     Pixels that are drawn for each point, for example 2 for what a display of high "
      "density shows. Needs --headless-renderer, a window takes the density of its display\n"
      "  --input SCRIPT            Input in place of devices, for example \"1: pointer 640 360; 2: click\". "
      "Needs --headless-renderer\n"
      "  --input-script PATH       The same from a file, for example assets://input/menu.input. "
      "Needs --headless-renderer\n"
      "  --spawn PATH              Spawn this prefab at the top of the world once the scene is read, as a script "
      "would, for example assets://prefabs/target.prefab.yml\n"
      "  --tonemapper NAME         Curve for light brighter than white: none, aces, or agx, over "
      "rendering.tonemapper of the settings\n"
      "  --exposure NUMBER         How bright the scene is taken to be, for example 2 for twice the light, over "
      "rendering.exposure of the settings\n"
      "  --jit on|off              Compile the scripts as they run, or run them in LuaJIT's interpreter. Over "
      "scripting.jit of the settings, for comparing the two\n"
      "  --headless                Run as a dedicated server. Not available yet, see --headless-renderer\n");
  }

  // --tonemapper and --exposure

  TEST_F(EditorOptionsTest, LeavesTheTonemapperAndTheExposureToTheSettingsWhenNotGiven)
  {
    _settings.tonemapper = neon::Tonemapper::Agx;
    _settings.exposure = 0.5;

    ASSERT_TRUE(Apply({"--scene", "assets://scenes/demo.scene.yml"}));

    EXPECT_EQ(_settings.tonemapper, neon::Tonemapper::Agx);
    EXPECT_DOUBLE_EQ(_settings.exposure, 0.5);
  }

  TEST_F(EditorOptionsTest, TonemapperTakesEveryCurveByItsName)
  {
    ASSERT_TRUE(Apply({"--tonemapper", "aces"}));
    EXPECT_EQ(_settings.tonemapper, neon::Tonemapper::Aces);

    ASSERT_TRUE(Apply({"--tonemapper=agx"}));
    EXPECT_EQ(_settings.tonemapper, neon::Tonemapper::Agx);

    ASSERT_TRUE(Apply({"--tonemapper", "none"}));
    EXPECT_EQ(_settings.tonemapper, neon::Tonemapper::None);
  }

  TEST_F(EditorOptionsTest, TonemapperRefusesACurveItDoesNotHave)
  {
    EXPECT_FALSE(Apply({"--tonemapper", "reinhard"}));
    EXPECT_EQ(_error, "Option '--tonemapper' needs none, aces, or agx");
  }

  TEST_F(EditorOptionsTest, ExposureIsANumberAboveZero)
  {
    ASSERT_TRUE(Apply({"--exposure", "2"}));
    EXPECT_DOUBLE_EQ(_settings.exposure, 2.0);

    ASSERT_TRUE(Apply({"--exposure=0.25"}));
    EXPECT_DOUBLE_EQ(_settings.exposure, 0.25);
  }

  TEST_F(EditorOptionsTest, ExposureRefusesZeroAndWhatIsNoNumber)
  {
    EXPECT_FALSE(Apply({"--exposure", "0"}));
    EXPECT_EQ(_error, "Option '--exposure' needs a number above zero, such as 2");

    EXPECT_FALSE(Apply({"--exposure", "bright"}));
    EXPECT_EQ(_error, "Option '--exposure' needs a number above zero, such as 2");
  }

  // --jit

  TEST_F(EditorOptionsTest, JitOffRunsTheScriptsInTheInterpreter)
  {
    ASSERT_TRUE(Apply({"--jit", "off"}));
    EXPECT_FALSE(_settings.script_jit);
  }

  TEST_F(EditorOptionsTest, JitOnCompilesOverASettingThatSaidOff)
  {
    _settings.script_jit = false;
    ASSERT_TRUE(Apply({"--jit=on"}));
    EXPECT_TRUE(_settings.script_jit);
  }

  TEST_F(EditorOptionsTest, JitLeftOutLeavesTheSetting)
  {
    _settings.script_jit = false;
    ASSERT_TRUE(Apply({}));
    EXPECT_FALSE(_settings.script_jit);
  }

  TEST_F(EditorOptionsTest, JitRefusesAnythingButOnAndOff)
  {
    EXPECT_FALSE(Parse({"--jit", "maybe"}));
    EXPECT_EQ(_command_line.GetError(), "'maybe' is not a value of '--jit'. It accepts on, off");
  }

  TEST_F(EditorOptionsTest, SceneNamesTheSceneToStartWith)
  {
    ASSERT_TRUE(Apply({"--scene", "assets://scenes/physics.scene.yml"}));

    EXPECT_EQ(_settings.scene_path, "assets://scenes/physics.scene.yml");
  }

  TEST_F(EditorOptionsTest, UiNamesTheUserInterfaceToShow)
  {
    ASSERT_TRUE(Apply({"--ui", "assets://ui/hud.ui.yml"}));

    EXPECT_EQ(_settings.ui_path, "assets://ui/hud.ui.yml");
    EXPECT_EQ(_error, "");
  }

  TEST_F(EditorOptionsTest, UiCanBeWrittenWithAnEqualsSign)
  {
    ASSERT_TRUE(Apply({"--ui=assets://ui/hud.ui.yml"}));

    EXPECT_EQ(_settings.ui_path, "assets://ui/hud.ui.yml");
  }

  TEST_F(EditorOptionsTest, UiLeavesTheSceneAsItIs)
  {
    const std::string scene = _settings.scene_path;

    ASSERT_TRUE(Apply({"--ui", "assets://ui/hud.ui.yml"}));

    EXPECT_EQ(_settings.scene_path, scene);
  }

  TEST_F(EditorOptionsTest, HeadlessRendererTakesNoValue)
  {
    EXPECT_FALSE(Parse({"--headless-renderer=yes"}));
    EXPECT_EQ(_command_line.GetError(), "Option '--headless-renderer' takes no value");
  }

  TEST_F(EditorOptionsTest, EveryOptionButTheHeadlessOnesNeedsAValue)
  {
    for (const char *option : {"--scene", "--ui", "--frames", "--screenshot", "--screenshot-at", "--output-dir",
                               "--time-step", "--spawn", "--render-scale", "--input", "--input-script", "--jit",
                               "--tonemapper", "--exposure"})
    {
      EXPECT_FALSE(Parse({option})) << option;
      EXPECT_EQ(_command_line.GetError(), "Option '" + std::string(option) + "' needs a value");
    }
  }

  TEST_F(EditorOptionsTest, CanBeRegisteredTwice)
  {
    const std::string help = _command_line.GetHelp();

    _options.Register(_command_line);

    EXPECT_EQ(_command_line.GetHelp(), help);
  }

  // Apply, without arguments

  TEST_F(EditorOptionsTest, LeavesTheSettingsAsTheyAreWithoutArguments)
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
    EXPECT_EQ(_settings.scene_path, "");
    EXPECT_EQ(_settings.ui_path, "");
    EXPECT_EQ(_settings.render_scale, 1.0);
    EXPECT_TRUE(_settings.input_script.empty());
    EXPECT_TRUE(_settings.input_script_path.empty());
    EXPECT_THAT(_settings.screenshot_frames, IsEmpty());
    EXPECT_EQ(_settings.width, 1920);
    EXPECT_EQ(_settings.height, 1080);
  }

  TEST_F(EditorOptionsTest, KeepsWhatTheApplicationSetWhenAnOptionIsNotGiven)
  {
    _settings.max_frames = 5;
    _settings.time_step = 0.5;
    _settings.output_directory = "somewhere";

    ASSERT_TRUE(Apply({}));

    EXPECT_EQ(_settings.max_frames, 5u);
    EXPECT_EQ(_settings.time_step, 0.5);
    EXPECT_EQ(_settings.output_directory, "somewhere");
  }

  // --headless-renderer

  TEST_F(EditorOptionsTest, HeadlessRendererIsCarriedOver)
  {
    ASSERT_TRUE(Apply({"--headless-renderer"}));

    EXPECT_TRUE(_settings.headless_renderer);
  }

  TEST_F(EditorOptionsTest, HeadlessRendererIsTurnedOffWhenItIsNotGiven)
  {
    _settings.headless_renderer = true;

    ASSERT_TRUE(Apply({}));

    EXPECT_FALSE(_settings.headless_renderer);
  }

  // --headless

  TEST_F(EditorOptionsTest, HeadlessIsRefusedUntilThereIsADedicatedServer)
  {
    EXPECT_FALSE(Apply({"--headless"}));

    EXPECT_EQ(
      _error,
      "'--headless' is for a dedicated server, which does not exist yet (#144). "
      "To render without a window, use '--headless-renderer'");
  }

  TEST_F(EditorOptionsTest, HeadlessIsNotTheHeadlessRenderer)
  {
    EXPECT_FALSE(Apply({"--headless"}));

    EXPECT_FALSE(_settings.headless_renderer);
  }

  // --output-dir

  TEST_F(EditorOptionsTest, OutputDirectoryIsCarriedOverAsItWasGiven)
  {
    ASSERT_TRUE(Apply({"--output-dir", "../some where/shots"}));

    EXPECT_EQ(_settings.output_directory, "../some where/shots");
  }

  // --spawn

  TEST_F(EditorOptionsTest, SpawnIsCarriedOverAsItWasGiven)
  {
    ASSERT_TRUE(Apply({"--spawn", "assets://prefabs/target.prefab.yml"}));

    EXPECT_EQ(_settings.spawn_path, "assets://prefabs/target.prefab.yml");
  }

  TEST_F(EditorOptionsTest, SpawnsNothingUnlessAsked)
  {
    ASSERT_TRUE(Apply({}));

    EXPECT_TRUE(_settings.spawn_path.empty());
  }

  // --frames

  TEST_F(EditorOptionsTest, FramesAreCarriedOver)
  {
    ASSERT_TRUE(Apply({"--frames", "60"}));

    EXPECT_EQ(_settings.max_frames, 60u);
  }

  TEST_F(EditorOptionsTest, FramesAcceptOne)
  {
    ASSERT_TRUE(Apply({"--frames=1"}));

    EXPECT_EQ(_settings.max_frames, 1u);
  }

  class EditorOptionsBadFrames : public EditorOptionsTest,
                                      public ::testing::WithParamInterface<const char *> {};

  TEST_P(EditorOptionsBadFrames, AreRefused)
  {
    EXPECT_FALSE(Apply({"--frames", GetParam()}));

    EXPECT_EQ(_error, "Option '--frames' needs a whole number above zero");
    EXPECT_EQ(_settings.max_frames, 0u);
  }

  INSTANTIATE_TEST_SUITE_P(
    EditorOptions,
    EditorOptionsBadFrames,
    ::testing::Values("0", "-1", "abc", "1.5", "10abc", "1,2", "99999999999999999999999999"));

  // --time-step

  TEST_F(EditorOptionsTest, TimeStepIsCarriedOver)
  {
    ASSERT_TRUE(Apply({"--time-step", "0.016667"}));

    EXPECT_DOUBLE_EQ(_settings.time_step, 0.016667);
  }

  TEST_F(EditorOptionsTest, TimeStepAcceptsAWholeNumber)
  {
    ASSERT_TRUE(Apply({"--time-step=2"}));

    EXPECT_DOUBLE_EQ(_settings.time_step, 2.0);
  }

  class EditorOptionsBadTimeStep : public EditorOptionsTest,
                                        public ::testing::WithParamInterface<const char *> {};

  TEST_P(EditorOptionsBadTimeStep, IsRefused)
  {
    EXPECT_FALSE(Apply({"--time-step", GetParam()}));

    EXPECT_EQ(_error, "Option '--time-step' needs a number of seconds above zero, such as 0.016667");
    EXPECT_EQ(_settings.time_step, 0.0);
  }

  INSTANTIATE_TEST_SUITE_P(
    EditorOptions,
    EditorOptionsBadTimeStep,
    ::testing::Values("0", "0.0", "-0.5", "abc", "0,5", "1e-3", "."));

  // --render-scale

  TEST_F(EditorOptionsTest, TakesTheRenderScaleWithoutAWindow)
  {
    ASSERT_TRUE(Apply({"--headless-renderer", "--render-scale", "1.25"}));
    EXPECT_EQ(_settings.render_scale, 1.25);
  }

  TEST_F(EditorOptionsTest, RefusesARenderScaleWithAWindow)
  {
    EXPECT_FALSE(Apply({"--render-scale", "2"}));
    EXPECT_EQ(_error, "Option '--render-scale' needs '--headless-renderer'. A window takes the density of its display");
    EXPECT_EQ(_settings.render_scale, 1.0);
  }

  TEST_F(EditorOptionsTest, RefusesARenderScaleThatIsOutOfRange)
  {
    for (const char *scale : {"0", "-1", "0.1", "9", "twice"})
    {
      _settings.render_scale = 1.0;
      EXPECT_FALSE(Apply({"--headless-renderer", "--render-scale", scale})) << scale;
      EXPECT_EQ(_error, "Option '--render-scale' needs a number from 0.25 to 8, such as 2");
      EXPECT_EQ(_settings.render_scale, 1.0);
    }
  }

  // --input and --input-script

  TEST_F(EditorOptionsTest, TakesAScriptOfInputWithoutAWindow)
  {
    ASSERT_TRUE(Apply({"--headless-renderer", "--input", "1: pointer 10 10; 2: click"}));
    EXPECT_EQ(_settings.input_script, "1: pointer 10 10; 2: click");

    ASSERT_TRUE(Apply({"--headless-renderer", "--input-script", "assets://input/menu.input"}));
    EXPECT_EQ(_settings.input_script_path, "assets://input/menu.input");
  }

  TEST_F(EditorOptionsTest, RefusesAScriptOfInputWithAWindow)
  {
    EXPECT_FALSE(Apply({"--input", "1: click"}));
    EXPECT_EQ(_error, "Option '--input' needs '--headless-renderer'. A window takes its input from devices");

    EXPECT_FALSE(Apply({"--input-script", "assets://a.input"}));
    EXPECT_EQ(_error, "Option '--input-script' needs '--headless-renderer'. A window takes its input from devices");
  }

  TEST_F(EditorOptionsTest, RefusesTwoScriptsOfInput)
  {
    EXPECT_FALSE(Apply({"--headless-renderer", "--input", "1: click", "--input-script", "assets://a.input"}));
    EXPECT_EQ(_error, "Option '--input' and '--input-script' cannot be given together");
  }

  // --screenshot

  TEST_F(EditorOptionsTest, ScreenshotIsCarriedOverWithFrames)
  {
    ASSERT_TRUE(Apply({"--frames", "3", "--screenshot", "user://frame.png"}));

    EXPECT_EQ(_settings.screenshot_path, "user://frame.png");
    EXPECT_EQ(_settings.max_frames, 3u);
    EXPECT_THAT(_settings.screenshot_frames, IsEmpty());
  }

  TEST_F(EditorOptionsTest, ScreenshotDoesNotDependOnTheOrderOfTheOptions)
  {
    ASSERT_TRUE(Apply({"--screenshot", "user://frame.png", "--frames", "3"}));

    EXPECT_EQ(_settings.screenshot_path, "user://frame.png");
    EXPECT_EQ(_settings.max_frames, 3u);
  }

  TEST_F(EditorOptionsTest, ScreenshotIsRefusedWithoutFramesOrScreenshotAt)
  {
    EXPECT_FALSE(Apply({"--screenshot", "user://frame.png"}));

    EXPECT_EQ(_error, "Option '--screenshot' needs '--frames' or '--screenshot-at'");
    EXPECT_EQ(_settings.screenshot_path, "");
  }

  TEST_F(EditorOptionsTest, ScreenshotAcceptsTheFramesTheApplicationSet)
  {
    _settings.max_frames = 10;

    ASSERT_TRUE(Apply({"--screenshot", "user://frame.png"}));

    EXPECT_EQ(_settings.screenshot_path, "user://frame.png");
  }

  TEST_F(EditorOptionsTest, ScreenshotToOutputIsCarriedOverWithAnOutputDirectory)
  {
    ASSERT_TRUE(Apply({"--frames", "3", "--output-dir", "shots", "--screenshot", "output://frame.png"}));

    EXPECT_EQ(_settings.screenshot_path, "output://frame.png");
    EXPECT_EQ(_settings.output_directory, "shots");
  }

  TEST_F(EditorOptionsTest, ScreenshotToOutputIsRefusedWithoutAnOutputDirectory)
  {
    EXPECT_FALSE(Apply({"--frames", "3", "--screenshot", "output://frame.png"}));

    EXPECT_EQ(_error, "'output://frame.png' needs '--output-dir', which says where output:// is");
  }

  TEST_F(EditorOptionsTest, ScreenshotToOutputAcceptsTheOutputDirectoryTheApplicationSet)
  {
    _settings.output_directory = "shots";

    EXPECT_TRUE(Apply({"--frames", "3", "--screenshot", "output://frame.png"}));
  }

  TEST_F(EditorOptionsTest, ScreenshotPathIsNotCheckedAgainstThePathRules)
  {
    // the file system refuses it when the frame is written
    ASSERT_TRUE(Apply({"--frames", "3", "--screenshot", "frame.png"}));

    EXPECT_EQ(_settings.screenshot_path, "frame.png");
  }

  // --screenshot-at

  TEST_F(EditorOptionsTest, ScreenshotAtIsCarriedOver)
  {
    ASSERT_TRUE(Apply({"--screenshot", "user://frame.png", "--screenshot-at", "1,30,60"}));

    EXPECT_THAT(_settings.screenshot_frames, ElementsAre(1u, 30u, 60u));
    EXPECT_EQ(_settings.screenshot_path, "user://frame.png");
  }

  TEST_F(EditorOptionsTest, ScreenshotAtGivesTheSameFramesWhenAppliedTwice)
  {
    // the application applies the command line again after its settings
    // files, so that the command line wins
    ASSERT_TRUE(Apply({"--screenshot", "user://frame.png", "--screenshot-at", "1,30,60"}));
    ASSERT_TRUE(_options.Apply(_command_line, _settings, _error));

    EXPECT_THAT(_settings.screenshot_frames, ElementsAre(1u, 30u, 60u));
    EXPECT_EQ(_settings.max_frames, 60u);
  }

  TEST_F(EditorOptionsTest, ScreenshotAtAcceptsOneFrame)
  {
    ASSERT_TRUE(Apply({"--screenshot", "user://frame.png", "--screenshot-at=7"}));

    EXPECT_THAT(_settings.screenshot_frames, ElementsAre(7u));
    EXPECT_EQ(_settings.max_frames, 7u);
  }

  TEST_F(EditorOptionsTest, ScreenshotAtPutsTheFramesInRisingOrder)
  {
    ASSERT_TRUE(Apply({"--screenshot", "user://frame.png", "--screenshot-at", "60,1,30"}));

    EXPECT_THAT(_settings.screenshot_frames, ElementsAre(1u, 30u, 60u));
  }

  TEST_F(EditorOptionsTest, ScreenshotAtKeepsAFrameThatIsListedTwiceOnce)
  {
    ASSERT_TRUE(Apply({"--screenshot", "user://frame.png", "--screenshot-at", "30,1,30,1,30"}));

    EXPECT_THAT(_settings.screenshot_frames, ElementsAre(1u, 30u));
  }

  TEST_F(EditorOptionsTest, ScreenshotAtMakesTheRunAsLongAsItsHighestFrame)
  {
    ASSERT_TRUE(Apply({"--screenshot", "user://frame.png", "--screenshot-at", "10,30"}));

    EXPECT_EQ(_settings.max_frames, 30u);
  }

  TEST_F(EditorOptionsTest, ScreenshotAtLeavesALongerRunAsLongAsItIs)
  {
    ASSERT_TRUE(Apply({"--frames", "60", "--screenshot", "user://frame.png", "--screenshot-at", "10,30"}));

    EXPECT_EQ(_settings.max_frames, 60u);
    EXPECT_THAT(_settings.screenshot_frames, ElementsAre(10u, 30u));
  }

  TEST_F(EditorOptionsTest, ScreenshotAtAcceptsTheLastFrameOfTheRun)
  {
    ASSERT_TRUE(Apply({"--frames", "30", "--screenshot", "user://frame.png", "--screenshot-at", "30"}));

    EXPECT_EQ(_settings.max_frames, 30u);
  }

  TEST_F(EditorOptionsTest, ScreenshotAtIsRefusedWhenAFrameIsNeverReached)
  {
    EXPECT_FALSE(Apply({"--frames", "10", "--screenshot", "user://frame.png", "--screenshot-at", "5,11"}));

    EXPECT_EQ(_error, "Frame 11 of '--screenshot-at' is never reached, '--frames' stops after 10");
  }

  TEST_F(EditorOptionsTest, ScreenshotAtIsRefusedWithoutScreenshot)
  {
    EXPECT_FALSE(Apply({"--screenshot-at", "1,2"}));

    EXPECT_EQ(_error, "Option '--screenshot-at' needs '--screenshot'");
    EXPECT_THAT(_settings.screenshot_frames, IsEmpty());
  }

  TEST_F(EditorOptionsTest, ScreenshotAtIsRefusedWithoutScreenshotEvenWithFrames)
  {
    EXPECT_FALSE(Apply({"--frames", "10", "--screenshot-at", "1,2"}));

    EXPECT_EQ(_error, "Option '--screenshot-at' needs '--screenshot'");
  }

  TEST_F(EditorOptionsTest, ScreenshotAtToOutputIsRefusedWithoutAnOutputDirectory)
  {
    EXPECT_FALSE(Apply({"--screenshot", "output://frame.png", "--screenshot-at", "1"}));

    EXPECT_EQ(_error, "'output://frame.png' needs '--output-dir', which says where output:// is");
  }

  class EditorOptionsBadScreenshotAt : public EditorOptionsTest,
                                            public ::testing::WithParamInterface<const char *> {};

  TEST_P(EditorOptionsBadScreenshotAt, IsRefused)
  {
    EXPECT_FALSE(Apply({"--screenshot", "user://frame.png", "--screenshot-at", GetParam()}));

    EXPECT_EQ(_error, "Option '--screenshot-at' needs whole numbers above zero, separated by commas");
  }

  INSTANTIATE_TEST_SUITE_P(
    EditorOptions,
    EditorOptionsBadScreenshotAt,
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

  TEST_F(EditorOptionsTest, CarriesOverEveryOptionAtOnce)
  {
    ASSERT_TRUE(
      Apply({
        "--renderer", "vulkan",
        "--headless-renderer",
        "--frames", "60",
        "--output-dir", "shots",
        "--screenshot", "output://frame.png",
        "--screenshot-at", "10,30",
        "--time-step", "0.05",
        "--render-scale", "2",
        "--window-size", "640x480"
        }));

    EXPECT_EQ(_error, "");
    EXPECT_EQ(_settings.selected_api, RenderingApi::Vulkan);
    EXPECT_TRUE(_settings.headless_renderer);
    EXPECT_EQ(_settings.max_frames, 60u);
    EXPECT_EQ(_settings.output_directory, "shots");
    EXPECT_EQ(_settings.screenshot_path, "output://frame.png");
    EXPECT_THAT(_settings.screenshot_frames, ElementsAre(10u, 30u));
    EXPECT_DOUBLE_EQ(_settings.time_step, 0.05);
    EXPECT_EQ(_settings.render_scale, 2.0);
    EXPECT_EQ(_settings.width, 640);
    EXPECT_EQ(_settings.height, 480);
  }
}
