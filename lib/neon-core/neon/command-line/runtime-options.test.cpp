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

  /// Takes arguments with the set the runtime owns alone: the arguments are
  /// parsed, and the result carried over into the settings. Nothing of the
  /// editor's set is declared.
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

  TEST_F(RuntimeOptionsTest, HelpTextListsWhatTheRuntimeOwns)
  {
    EXPECT_EQ(
      _command_line.GetHelp(),
      "Runs a Neon Engine project.\n"
      "\n"
      "Usage: NeonRuntime [options]\n"
      "\n"
      "  --help                Show this text\n"
      "  --renderer vulkan     Renderer to draw with. Default: vulkan\n"
      "  --vulkan-version 1.N  Highest version of Vulkan to render with, for example 1.2. Default: 1.3\n");
  }

  TEST_F(RuntimeOptionsTest, TheEditorsOptionsAreNotOptionsOfThisSet)
  {
    for (const char *option : {"--scene", "--ui", "--frames", "--screenshot", "--screenshot-at", "--output-dir",
                               "--time-step", "--headless-renderer", "--headless", "--spawn", "--input",
                               "--input-script", "--render-scale"})
    {
      EXPECT_FALSE(Parse({option, "value"})) << option;
      EXPECT_EQ(_command_line.GetError(), "Unknown option '" + std::string(option) + "'");
    }
  }

  TEST_F(RuntimeOptionsTest, RendererAndVulkanVersionNeedAValue)
  {
    for (const char *option : {"--renderer", "--vulkan-version"})
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
    EXPECT_EQ(_settings.vulkan_version, (ApiVersion{1, 3}));
    EXPECT_EQ(_settings.width, 1920);
    EXPECT_EQ(_settings.height, 1080);
  }

  // --renderer

  TEST_F(RuntimeOptionsTest, RendererAcceptsVulkan)
  {
    EXPECT_TRUE(Parse({"--renderer", "vulkan"}));
  }

  TEST_F(RuntimeOptionsTest, RendererRefusesWhatIsNotARenderer)
  {
    EXPECT_FALSE(Parse({"--renderer", "opengl"}));
    EXPECT_EQ(_command_line.GetError(), "'opengl' is not a value of '--renderer'. It accepts vulkan");
  }

  TEST_F(RuntimeOptionsTest, RendererSelectsVulkan)
  {
    ASSERT_TRUE(Apply({"--renderer=vulkan"}));

    EXPECT_EQ(_settings.selected_api, RenderingApi::Vulkan);
  }

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
}
