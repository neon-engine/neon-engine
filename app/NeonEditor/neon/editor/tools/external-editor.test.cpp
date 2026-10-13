#include "external-editor.hpp"

#include <memory>
#include <string>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <neon/testing/fake-native-file-context.hpp>
#include <neon/testing/fake-process-context.hpp>
#include <neon/testing/recording-logger.hpp>

namespace
{
  using neon::ExternalEditor;
  using neon::ProcessCommand;
  using neon::Toolchains;
  using neon::testing::FakeNativeFileContext;
  using neon::testing::FakeProcessContext;
  using neon::testing::LogLevel;
  using neon::testing::RecordingLogger;
  using ::testing::ElementsAre;
  using ::testing::IsEmpty;
  using ::testing::SizeIs;

  const std::string root = "/work/game";
  const std::string in_bundle = "Visual Studio Code.app/Contents/Resources/app/bin/code";

  class ExternalEditorTest : public ::testing::Test
  {
  protected:
    std::shared_ptr<RecordingLogger> _logger = std::make_shared<RecordingLogger>();
    Toolchains _toolchains;
    FakeProcessContext _processes;
    FakeNativeFileContext _native;

    /// The editor on a platform, by what the library loader calls it.
    ExternalEditor On(const std::string &platform)
    {
      return {&_toolchains, &_processes, &_native, platform, _logger};
    }

    /// Marks a program at a native path as installed.
    void InstallAt(const std::string &path)
    {
      _processes.installed[path] = path;
    }
  };

  // where it is

  TEST_F(ExternalEditorTest, UsesWhereTheToolchainFilesSayItIs)
  {
    _toolchains.SetEditor("vscode", "/opt/code/bin/code");
    InstallAt("/opt/code/bin/code");
    _processes.Install("code");

    EXPECT_TRUE(On("macos-arm64").Open(root));

    ASSERT_THAT(_processes.started, SizeIs(1));
    EXPECT_EQ(_processes.started[0].program, "/opt/code/bin/code");
    EXPECT_EQ(_processes.started[0].directory, root);
    EXPECT_THAT(_processes.started[0].arguments, ElementsAre("."));
    EXPECT_TRUE(_logger->Contains(LogLevel::Info, "Opening /work/game in VS Code"))
      << _logger->Messages(LogLevel::Info);
  }

  TEST_F(ExternalEditorTest, ANameInTheFilesIsLookedForOnPath)
  {
    _toolchains.SetEditor("vscode", "code");
    _processes.Install("code");

    std::string program;
    EXPECT_TRUE(On("linux-x86_64").Locate(program));
    EXPECT_EQ(program, "/fake/bin/code");
  }

  TEST_F(ExternalEditorTest, LooksOnPathWhenTheFilesSayNothing)
  {
    _processes.Install("code");

    EXPECT_TRUE(On("macos-arm64").Open(root));

    ASSERT_THAT(_processes.started, SizeIs(1));
    EXPECT_EQ(_processes.started[0].program, "/fake/bin/code");
  }

  TEST_F(ExternalEditorTest, LooksOnWhenTheFilesNameAPlaceThatIsNotThere)
  {
    _toolchains.SetEditor("vscode", "/nowhere/code");
    InstallAt("/Applications/" + in_bundle);

    EXPECT_TRUE(On("macos-arm64").Open(root));

    ASSERT_THAT(_processes.started, SizeIs(1));
    EXPECT_EQ(_processes.started[0].program, "/Applications/" + in_bundle);
    EXPECT_TRUE(_logger->Contains(LogLevel::Warn, "/nowhere/code")) << _logger->Messages(LogLevel::Warn);
  }

  TEST_F(ExternalEditorTest, TheUsualPlacesOfTheMacComeAfterPath)
  {
    EXPECT_THAT(
      On("macos-arm64").DefaultPlaces(),
      ElementsAre("code", "/Applications/" + in_bundle, "/home/someone/Applications/" + in_bundle));
  }

  TEST_F(ExternalEditorTest, TheUsualPlacesOfWindowsComeAfterPath)
  {
    _native.home = "C:\\Users\\someone";
    _native.separator = '\\';

    EXPECT_THAT(
      On("windows-x86_64").DefaultPlaces(),
      ElementsAre(
        "code",
        "code.cmd",
        "C:\\Users\\someone\\AppData\\Local\\Programs\\Microsoft VS Code\\bin\\code.cmd",
        "C:\\Program Files\\Microsoft VS Code\\bin\\code.cmd"));
  }

  TEST_F(ExternalEditorTest, TheUsualPlacesOfLinuxComeAfterPath)
  {
    EXPECT_THAT(
      On("linux-x86_64").DefaultPlaces(),
      ElementsAre("code", "/usr/bin/code", "/snap/bin/code", "/usr/share/code/bin/code"));
  }

  TEST_F(ExternalEditorTest, WithoutAHomeFolderThePlacesUnderItAreLeftOut)
  {
    _native.home = "";

    EXPECT_THAT(On("macos-arm64").DefaultPlaces(), ElementsAre("code", "/Applications/" + in_bundle));
  }

  TEST_F(ExternalEditorTest, TheUsualPlacesAreTriedInOrder)
  {
    InstallAt("/snap/bin/code");
    InstallAt("/usr/share/code/bin/code");

    std::string program;
    EXPECT_TRUE(On("linux-x86_64").Locate(program));
    EXPECT_EQ(program, "/snap/bin/code");
  }

  TEST_F(ExternalEditorTest, SaysWhereToSetItWhenItIsNowhere)
  {
    EXPECT_FALSE(On("macos-arm64").Open(root));

    EXPECT_THAT(_processes.started, IsEmpty());
    EXPECT_TRUE(_logger->Contains(LogLevel::Error, "VS Code was not found")) << _logger->Messages(LogLevel::Error);
    EXPECT_TRUE(_logger->Contains(LogLevel::Error, "'editors: vscode:'"));
    EXPECT_TRUE(_logger->Contains(LogLevel::Error, "editor.yml or user-editor.yml"));
    EXPECT_TRUE(_logger->Contains(LogLevel::Error, "docs/editor-toolchains.md"));
  }

  // the command

  TEST_F(ExternalEditorTest, OpensTheRootOfTheProjectInItsOwnFolder)
  {
    const ProcessCommand command = ExternalEditor::Command("code", "projects/museum", "", 0);

    EXPECT_EQ(command.program, "code");
    EXPECT_EQ(command.directory, "projects/museum");
    EXPECT_THAT(command.arguments, ElementsAre("."));
  }

  TEST_F(ExternalEditorTest, PutsAFileAtALineInFront)
  {
    const ProcessCommand command = ExternalEditor::Command("code", root, "assets/scripts/door.lua", 12);

    EXPECT_THAT(command.arguments, ElementsAre(".", "--goto", "assets/scripts/door.lua:12"));
  }

  TEST_F(ExternalEditorTest, AFileWithoutALineOpensWhereItWasLeft)
  {
    const ProcessCommand command = ExternalEditor::Command("code", root, "assets/scripts/door.lua", 0);

    EXPECT_THAT(command.arguments, ElementsAre(".", "--goto", "assets/scripts/door.lua"));
  }

  TEST_F(ExternalEditorTest, OpensWithAFileInFront)
  {
    _processes.Install("code");

    EXPECT_TRUE(On("macos-arm64").Open(root, "assets/scripts/door.lua", 12));

    ASSERT_THAT(_processes.started, SizeIs(1));
    EXPECT_THAT(_processes.started[0].arguments, ElementsAre(".", "--goto", "assets/scripts/door.lua:12"));
    EXPECT_TRUE(_logger->Contains(LogLevel::Info, "with assets/scripts/door.lua in front"))
      << _logger->Messages(LogLevel::Info);
  }

  TEST_F(ExternalEditorTest, SaysWhenItCouldNotBeStarted)
  {
    _processes.Install("code");
    _processes.refuses_start = true;

    EXPECT_FALSE(On("macos-arm64").Open(root));

    EXPECT_TRUE(_logger->Contains(LogLevel::Error, "VS Code could not be started")) << _logger->Messages(LogLevel::Error);
  }
} // namespace
