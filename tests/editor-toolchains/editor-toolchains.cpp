// The toolchains of the editor, as the editor reads and runs them:
// ToolchainFile with the document format for YAML, the merge of the team's
// file and the user's, the placeholders, the check of what is installed
// against fake processes, and one step run through the processes of the
// operating system. The files are kept in memory, except for the toolchains
// of the sandbox, which are read as they are in the repository.

#include <filesystem>
#include <fstream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <neon/data/ryml-document-format.hpp>
#include <neon/editor/toolchains/toolchain-check.hpp>
#include <neon/editor/toolchains/toolchain-file.hpp>
#include <neon/editor/toolchains/toolchain-placeholders.hpp>
#include <neon/editor/toolchains/toolchain-run.hpp>
#include <neon/editor/toolchains/toolchains.hpp>
#include <neon/process/os-process-runner.hpp>
#include <neon/testing/fake-process-context.hpp>
#include <neon/testing/memory-file-system.hpp>
#include <neon/testing/recording-logger.hpp>

namespace
{
  using neon::BuildTarget;
  using neon::OS_ProcessRunner;
  using neon::ProcessCommand;
  using neon::RYML_DocumentFormat;
  using neon::Toolchain;
  using neon::ToolchainCheck;
  using neon::ToolchainFile;
  using neon::ToolchainKind;
  using neon::ToolchainPlaceholders;
  using neon::ToolchainRun;
  using neon::Toolchains;
  using neon::ToolchainStep;
  using neon::testing::FakeProcessContext;
  using neon::testing::MemoryFileSystem;
  using neon::testing::RecordingLogger;
  using ::testing::ElementsAre;
  using ::testing::HasSubstr;
  using ::testing::IsEmpty;
  using ::testing::SizeIs;

  const std::string complete =
    "version: 1\n"
    "toolchains:\n"
    "  macos-arm64:\n"
    "    kind: native\n"
    "    platform: darwin-arm64\n"
    "    requires: [cmake, ninja, /opt/homebrew/opt/llvm@20/bin/clang++]\n"
    "    configure: cmake -S {project} -B {build} --preset macos-arm64-release\n"
    "    build: cmake --build {build}\n"
    "    run: \"{build}/{name}/{name}\"\n"
    "  linux-x64:\n"
    "    kind: docker\n"
    "    platform: linux-x86_64\n"
    "    image: neon-engine/linux-x64\n"
    "    requires: [docker]\n"
    "    configure: cmake -S {project} -B {build} --preset linux-x64-release\n"
    "    build: cmake --build {build}\n"
    "    run: off\n"
    "editors:\n"
    "  vscode: /Applications/Visual Studio Code.app/Contents/Resources/app/bin/code\n";

  const std::string one_native =
    "version: 1\n"
    "toolchains:\n"
    "  one:\n"
    "    kind: native\n"
    "    platform: darwin-arm64\n"
    "    configure: cmake -S {project} -B {build}\n"
    "    build: cmake --build {build}\n";

  class EditorToolchainsTest : public ::testing::Test
  {
  protected:
    std::shared_ptr<RecordingLogger> _logger = std::make_shared<RecordingLogger>();
    MemoryFileSystem _files{SettingsConfig{}, _logger};
    RYML_DocumentFormat _yaml;
    ToolchainFile _file{&_files, &_yaml, _logger};
    Toolchains _toolchains;
    std::vector<std::string> _errors;

    void SetUp() override
    {
      _files.Initialize();
    }

    /// Reads a text as the file `editor.yml`.
    bool Read(const std::string &text)
    {
      return _file.ReadText("editor.yml", text, _toolchains, _errors);
    }

    /// Reads, and expects it to fail with one error that holds the text.
    void ExpectRefused(const std::string &text, const std::string &expected)
    {
      EXPECT_FALSE(Read(text));
      ASSERT_EQ(_errors.size(), 1u) << ::testing::PrintToString(_errors);
      EXPECT_THAT(_errors.front(), HasSubstr(expected));
    }

    /// The text of a file of the sandbox, as it is in the repository.
    static std::string FileOfTheSandbox(const std::string &name)
    {
      const std::filesystem::path path = std::filesystem::path(NEON_SANDBOX_DIRECTORY) / name;
      const std::ifstream file(path);
      EXPECT_TRUE(file.good()) << path;

      std::stringstream text;
      text << file.rdbuf();
      return text.str();
    }
  };

  // reading

  TEST_F(EditorToolchainsTest, ReadsEverything)
  {
    ASSERT_TRUE(Read(complete)) << ::testing::PrintToString(_errors);
    ASSERT_THAT(_toolchains.All(), SizeIs(2));

    const Toolchain &native = _toolchains.All()[0];
    EXPECT_EQ(native.name, "macos-arm64");
    EXPECT_EQ(native.kind, ToolchainKind::Native);
    EXPECT_EQ(native.platform, "darwin-arm64");
    EXPECT_EQ(native.image, "");
    EXPECT_THAT(native.required, ElementsAre("cmake", "ninja", "/opt/homebrew/opt/llvm@20/bin/clang++"));
    EXPECT_EQ(native.configure, "cmake -S {project} -B {build} --preset macos-arm64-release");
    EXPECT_EQ(native.build, "cmake --build {build}");
    EXPECT_EQ(native.run, "{build}/{name}/{name}");
    EXPECT_TRUE(native.CanRun());

    const Toolchain &docker = _toolchains.All()[1];
    EXPECT_EQ(docker.name, "linux-x64");
    EXPECT_EQ(docker.kind, ToolchainKind::Docker);
    EXPECT_EQ(docker.platform, "linux-x86_64");
    EXPECT_EQ(docker.image, "neon-engine/linux-x64");
    EXPECT_THAT(docker.required, ElementsAre("docker"));
    EXPECT_FALSE(docker.CanRun());

    EXPECT_EQ(_toolchains.Editor("vscode"), "/Applications/Visual Studio Code.app/Contents/Resources/app/bin/code");
    EXPECT_EQ(_toolchains.Editor("other"), "");
    EXPECT_THAT(_errors, IsEmpty());
  }

  TEST_F(EditorToolchainsTest, ReadsAFileThroughTheFileSystem)
  {
    _files.AddNativeFile("/editor/user-editor.yml", one_native);

    ASSERT_TRUE(_file.Read(std::string(ToolchainFile::user_path), _toolchains, _errors))
      << ::testing::PrintToString(_errors);
    EXPECT_NE(_toolchains.Find("one"), nullptr);
  }

  TEST_F(EditorToolchainsTest, ReadsAFileWithEditorsAlone)
  {
    ASSERT_TRUE(Read("version: 1\neditors:\n  vscode: code\n")) << ::testing::PrintToString(_errors);
    EXPECT_THAT(_toolchains.All(), IsEmpty());
    EXPECT_EQ(_toolchains.Editor("vscode"), "code");
  }

  TEST_F(EditorToolchainsTest, RunIsOffWhenItIsLeftOut)
  {
    ASSERT_TRUE(Read(one_native)) << ::testing::PrintToString(_errors);
    EXPECT_FALSE(_toolchains.Find("one")->CanRun());
  }

  TEST_F(EditorToolchainsTest, ReadsTheToolchainsOfTheSandbox)
  {
    ASSERT_TRUE(Read(FileOfTheSandbox(std::string(ToolchainFile::shared_name)))) << ::testing::PrintToString(_errors);
    ASSERT_THAT(_toolchains.All(), SizeIs(3));
    EXPECT_EQ(_toolchains.All()[0].name, "macos-arm64");
    EXPECT_EQ(_toolchains.All()[1].name, "linux-x64");
    EXPECT_EQ(_toolchains.All()[2].name, "windows-x64");
    EXPECT_TRUE(_toolchains.Find("macos-arm64")->CanRun());
    EXPECT_FALSE(_toolchains.Find("windows-x64")->CanRun());
  }

  TEST_F(EditorToolchainsTest, SaysWhereTheFilesAre)
  {
    EXPECT_EQ(ToolchainFile::shared_name, "editor.yml");
    EXPECT_EQ(ToolchainFile::user_name, "user-editor.yml");
    EXPECT_EQ(ToolchainFile::user_path, "editor://user-editor.yml");
  }

  // what is refused

  TEST_F(EditorToolchainsTest, RefusesAMissingFile)
  {
    EXPECT_FALSE(_file.Read("editor://user-editor.yml", _toolchains, _errors));
    ASSERT_THAT(_errors, SizeIs(1));
    EXPECT_THAT(_errors.front(), HasSubstr("editor://user-editor.yml: the file cannot be read"));
  }

  TEST_F(EditorToolchainsTest, RefusesAMissingVersion)
  {
    ExpectRefused("toolchains: {}\n", "'version' is missing. It holds the version of the layout, which is 1");
  }

  TEST_F(EditorToolchainsTest, RefusesAVersionAboveItsOwn)
  {
    ExpectRefused("version: 2\n", "editor.yml:1: the file has version 2, and this editor reads up to version 1");
  }

  TEST_F(EditorToolchainsTest, RefusesANameThatIsNotKnown)
  {
    ExpectRefused("version: 1\ntoolchain: {}\n", "'toolchain' is not known to the toolchains. Known are: version, toolchains, editors");
  }

  TEST_F(EditorToolchainsTest, RefusesAMissingKind)
  {
    ExpectRefused(
      "version: 1\ntoolchains:\n  one:\n    platform: darwin-arm64\n    configure: a\n    build: b\n",
      "editor.yml:3: 'kind' is missing. It is native or docker");
  }

  TEST_F(EditorToolchainsTest, RefusesAKindThatIsNotKnown)
  {
    ExpectRefused(
      "version: 1\ntoolchains:\n  one:\n    kind: remote\n    platform: darwin-arm64\n    configure: a\n    build: b\n",
      "editor.yml:4: 'kind' of toolchain 'one' is 'remote', where native or docker was expected");
  }

  TEST_F(EditorToolchainsTest, RefusesAMissingPlatform)
  {
    ExpectRefused(
      "version: 1\ntoolchains:\n  one:\n    kind: native\n    configure: a\n    build: b\n",
      "'platform' is missing. It is what the toolchain builds for");
  }

  TEST_F(EditorToolchainsTest, RefusesADockerToolchainWithoutAnImage)
  {
    ExpectRefused(
      "version: 1\ntoolchains:\n  one:\n    kind: docker\n    platform: linux-x86_64\n    configure: a\n    build: b\n",
      "'image' is missing. It is the Docker image the toolchain runs in");
  }

  TEST_F(EditorToolchainsTest, RefusesAnImageOnANativeToolchain)
  {
    ExpectRefused(
      "version: 1\ntoolchains:\n  one:\n    kind: native\n    platform: darwin-arm64\n    image: x\n    configure: a\n    build: b\n",
      "editor.yml:6: 'image' of toolchain 'one' is not taken: a native toolchain runs in no Docker image");
  }

  TEST_F(EditorToolchainsTest, RefusesAMissingCommand)
  {
    ExpectRefused(
      "version: 1\ntoolchains:\n  one:\n    kind: native\n    platform: darwin-arm64\n    configure: a\n",
      "'build' is missing. It is the command, as one would type it");
  }

  TEST_F(EditorToolchainsTest, RefusesAPlaceholderThatIsNotKnown)
  {
    ExpectRefused(
      "version: 1\ntoolchains:\n  one:\n    kind: native\n    platform: darwin-arm64\n    configure: cmake -S {source}\n    build: b\n",
      "editor.yml:6: 'configure' of toolchain 'one': '{source}' is not a placeholder. There are: {project}, {build}, {name}, {engine}, {platform}");
  }

  TEST_F(EditorToolchainsTest, RefusesABraceThatIsNotClosed)
  {
    ExpectRefused(
      "version: 1\ntoolchains:\n  one:\n    kind: native\n    platform: darwin-arm64\n    configure: a\n    build: cmake --build {build\n",
      "editor.yml:7: 'build' of toolchain 'one': the brace at '{build' is not closed");
  }

  TEST_F(EditorToolchainsTest, RefusesANameOfAToolchainThatIsNotKnown)
  {
    ExpectRefused(
      "version: 1\ntoolchains:\n  one:\n    kind: native\n    platform: darwin-arm64\n    configure: a\n    build: b\n    runs: c\n",
      "editor.yml:8: 'runs' is not known to toolchain 'one'. Known are: kind, platform, image, requires, configure, build, run");
  }

  TEST_F(EditorToolchainsTest, RefusesAToolchainThatIsNoMap)
  {
    ExpectRefused(
      "version: 1\ntoolchains:\n  one: native\n",
      "editor.yml:3: 'one' of 'toolchains' is text, where a map with the kind, the platform, and the commands of the toolchain was expected, or off to drop it");
  }

  TEST_F(EditorToolchainsTest, RefusesANameThatIsNotPlain)
  {
    ExpectRefused("version: 1\ntoolchains:\n  macOS: {}\n", "'macOS' of 'toolchains' is not a plain name");
  }

  TEST_F(EditorToolchainsTest, RefusesAnEditorWithoutAPath)
  {
    ExpectRefused("version: 1\neditors:\n  vscode: [a]\n", "editor.yml:3: 'vscode' of 'editors' is a list, where the path of the editor was expected");
  }

  TEST_F(EditorToolchainsTest, ReportsEveryProblemAndAddsNothing)
  {
    EXPECT_FALSE(Read(
      "version: 1\n"
      "toolchains:\n"
      "  one:\n"
      "    kind: native\n"
      "    platform: darwin-arm64\n"
      "    configure: a\n"
      "    build: b\n"
      "  two:\n"
      "    kind: nowhere\n"
      "    platform: linux-x86_64\n"
      "    configure: a\n"
      "editors:\n"
      "  vscode: \"\"\n"));

    EXPECT_THAT(_errors, SizeIs(3)) << ::testing::PrintToString(_errors);
    EXPECT_THAT(_toolchains.All(), IsEmpty());
    EXPECT_EQ(_toolchains.Editor("vscode"), "");
  }

  // merging

  TEST_F(EditorToolchainsTest, TheUsersFileReplacesAToolchainOfTheSameNameWhole)
  {
    ASSERT_TRUE(Read(complete)) << ::testing::PrintToString(_errors);
    ASSERT_TRUE(_file.ReadText(
      "user-editor.yml",
      "version: 1\n"
      "toolchains:\n"
      "  macos-arm64:\n"
      "    kind: native\n"
      "    platform: darwin-arm64\n"
      "    requires: [/usr/local/bin/cmake]\n"
      "    configure: /usr/local/bin/cmake -S {project} -B {build}\n"
      "    build: /usr/local/bin/cmake --build {build}\n",
      _toolchains, _errors)) << ::testing::PrintToString(_errors);

    ASSERT_THAT(_toolchains.All(), SizeIs(2));
    const Toolchain &native = _toolchains.All()[0];
    EXPECT_EQ(native.name, "macos-arm64");
    EXPECT_THAT(native.required, ElementsAre("/usr/local/bin/cmake"));
    EXPECT_EQ(native.configure, "/usr/local/bin/cmake -S {project} -B {build}");
    EXPECT_FALSE(native.CanRun());
    EXPECT_EQ(_toolchains.All()[1].name, "linux-x64");
  }

  TEST_F(EditorToolchainsTest, TheUsersFileAddsAToolchainAfterTheTeams)
  {
    ASSERT_TRUE(Read(complete)) << ::testing::PrintToString(_errors);
    ASSERT_TRUE(_file.ReadText("user-editor.yml", one_native, _toolchains, _errors)) << ::testing::PrintToString(_errors);

    ASSERT_THAT(_toolchains.All(), SizeIs(3));
    EXPECT_EQ(_toolchains.All()[2].name, "one");
  }

  TEST_F(EditorToolchainsTest, TheUsersFileDropsAToolchainWithOff)
  {
    ASSERT_TRUE(Read(complete)) << ::testing::PrintToString(_errors);
    ASSERT_TRUE(_file.ReadText("user-editor.yml", "version: 1\ntoolchains:\n  linux-x64: off\n", _toolchains, _errors))
      << ::testing::PrintToString(_errors);

    ASSERT_THAT(_toolchains.All(), SizeIs(1));
    EXPECT_EQ(_toolchains.All()[0].name, "macos-arm64");
    EXPECT_THAT(_toolchains.Dropped(), ElementsAre("linux-x64"));
  }

  TEST_F(EditorToolchainsTest, TheUsersFileReplacesAnEditorAndKeepsTheOthers)
  {
    ASSERT_TRUE(Read("version: 1\neditors:\n  vscode: code\n  clion: clion\n")) << ::testing::PrintToString(_errors);
    ASSERT_TRUE(_file.ReadText("user-editor.yml", "version: 1\neditors:\n  vscode: /opt/code\n", _toolchains, _errors))
      << ::testing::PrintToString(_errors);

    EXPECT_EQ(_toolchains.Editor("vscode"), "/opt/code");
    EXPECT_EQ(_toolchains.Editor("clion"), "clion");
  }

  TEST_F(EditorToolchainsTest, AFileWithAMistakeChangesNothingOfWhatWasReadBefore)
  {
    ASSERT_TRUE(Read(complete)) << ::testing::PrintToString(_errors);
    EXPECT_FALSE(_file.ReadText("user-editor.yml", "version: 1\ntoolchains:\n  linux-x64: off\n  bad: {}\n", _toolchains, _errors));

    EXPECT_THAT(_toolchains.All(), SizeIs(2));
    EXPECT_THAT(_toolchains.Dropped(), IsEmpty());
  }

  TEST_F(EditorToolchainsTest, DroppingOnAnEmptySetIsRememberedForWhatComesUnder)
  {
    Toolchains user;
    user.Drop("linux-x64");

    ASSERT_TRUE(Read(complete)) << ::testing::PrintToString(_errors);
    _toolchains.Apply(user);

    ASSERT_THAT(_toolchains.All(), SizeIs(1));
    EXPECT_EQ(_toolchains.All()[0].name, "macos-arm64");
  }

  // placeholders

  class ToolchainPlaceholdersTest : public ::testing::Test
  {
  protected:
    BuildTarget _target{.project = "/Users/me/Dev/my-game", .name = "my-game", .engine = "/Users/me/Dev/neon-engine"};

    static Toolchain Native()
    {
      return {.name = "macos-arm64", .kind = ToolchainKind::Native, .platform = "darwin-arm64"};
    }

    static Toolchain Docker()
    {
      return {.name = "linux-x64", .kind = ToolchainKind::Docker, .platform = "linux-x86_64", .image = "neon-engine/linux-x64"};
    }
  };

  TEST_F(ToolchainPlaceholdersTest, FillsThePathsOfThisMachineForANativeToolchain)
  {
    const ToolchainPlaceholders placeholders = ToolchainPlaceholders::For(Native(), _target);

    std::string filled;
    std::string problem;
    ASSERT_TRUE(placeholders.Fill("cmake -S {project} -B {build} -DENGINE={engine} && {build}/{name}/{name} --{platform}", filled, problem)) << problem;
    EXPECT_EQ(filled,
              "cmake -S /Users/me/Dev/my-game -B /Users/me/Dev/my-game/build/macos-arm64 "
              "-DENGINE=/Users/me/Dev/neon-engine && "
              "/Users/me/Dev/my-game/build/macos-arm64/my-game/my-game --darwin-arm64");
  }

  TEST_F(ToolchainPlaceholdersTest, FillsThePathsInsideTheContainerForADockerToolchain)
  {
    const ToolchainPlaceholders placeholders = ToolchainPlaceholders::For(Docker(), _target);

    std::string filled;
    std::string problem;
    ASSERT_TRUE(placeholders.Fill("cmake -S {project} -B {build} -DENGINE={engine} {name} {platform}", filled, problem)) << problem;
    EXPECT_EQ(filled, "cmake -S /src -B /src/build/linux-x64 -DENGINE=/engine my-game linux-x86_64");
  }

  TEST_F(ToolchainPlaceholdersTest, LeavesATextWithoutPlaceholdersAsItIs)
  {
    std::string filled;
    std::string problem;
    ASSERT_TRUE(ToolchainPlaceholders::For(Native(), _target).Fill("ninja -C build", filled, problem)) << problem;
    EXPECT_EQ(filled, "ninja -C build");
  }

  TEST_F(ToolchainPlaceholdersTest, RefusesAPlaceholderThatIsNotKnown)
  {
    std::string filled = "unchanged";
    std::string problem;
    EXPECT_FALSE(ToolchainPlaceholders::For(Native(), _target).Fill("{build}/{exe}", filled, problem));
    EXPECT_EQ(problem, "'{exe}' is not a placeholder. There are: {project}, {build}, {name}, {engine}, {platform}");
    EXPECT_EQ(filled, "unchanged");

    EXPECT_FALSE(ToolchainPlaceholders::Check("a {b", problem));
    EXPECT_EQ(problem, "the brace at '{b' is not closed");
    EXPECT_TRUE(ToolchainPlaceholders::Check("a {build} b", problem));
  }

  // detection

  class ToolchainCheckTest : public ::testing::Test
  {
  protected:
    std::shared_ptr<RecordingLogger> _logger = std::make_shared<RecordingLogger>();
    FakeProcessContext _processes;
    ToolchainCheck _check{&_processes, _logger};
    std::vector<std::string> _missing;

    static Toolchain Native()
    {
      return {
        .name = "macos-arm64", .kind = ToolchainKind::Native, .platform = "darwin-arm64",
        .required = {"cmake", "ninja", "/opt/homebrew/opt/llvm@20/bin/clang++"}
      };
    }

    static Toolchain Docker()
    {
      return {
        .name = "linux-x64", .kind = ToolchainKind::Docker, .platform = "linux-x86_64", .image = "neon-engine/linux-x64",
        .required = {"docker"}
      };
    }
  };

  TEST_F(ToolchainCheckTest, ANativeToolchainIsUsableWhenEveryProgramIsThere)
  {
    _processes.Install("cmake");
    _processes.Install("ninja");
    _processes.Install("/opt/homebrew/opt/llvm@20/bin/clang++");

    EXPECT_TRUE(_check.Check(Native(), _missing));
    EXPECT_THAT(_missing, IsEmpty());
    EXPECT_THAT(_processes.commands, IsEmpty());
  }

  TEST_F(ToolchainCheckTest, SaysEveryProgramThatIsMissing)
  {
    _processes.Install("cmake");

    EXPECT_FALSE(_check.Check(Native(), _missing));
    EXPECT_THAT(_missing, ElementsAre("ninja is not on PATH", "/opt/homebrew/opt/llvm@20/bin/clang++ is not there"));
  }

  TEST_F(ToolchainCheckTest, AToolchainThatRequiresNothingIsUsable)
  {
    Toolchain toolchain = Native();
    toolchain.required.clear();

    EXPECT_TRUE(_check.Check(toolchain, _missing));
  }

  TEST_F(ToolchainCheckTest, ADockerToolchainAsksDockerForItsImage)
  {
    _processes.Install("docker");

    EXPECT_TRUE(_check.Check(Docker(), _missing));
    ASSERT_THAT(_processes.commands, SizeIs(1));
    EXPECT_EQ(_processes.commands[0].program, "docker");
    EXPECT_THAT(_processes.commands[0].arguments, ElementsAre("image", "inspect", "neon-engine/linux-x64"));
  }

  TEST_F(ToolchainCheckTest, ADockerToolchainIsNotUsableWithoutItsImage)
  {
    _processes.Install("docker");
    _processes.results["docker"] = {.lines = {"[]", "Error: No such image: neon-engine/linux-x64"}, .exit_code = 1};

    EXPECT_FALSE(_check.Check(Docker(), _missing));
    EXPECT_THAT(_missing, ElementsAre("the Docker image neon-engine/linux-x64 is not built"));
  }

  TEST_F(ToolchainCheckTest, ADockerToolchainNeedsDockerWhetherOrNotItSaysSo)
  {
    Toolchain toolchain = Docker();
    toolchain.required.clear();

    EXPECT_FALSE(_check.Check(toolchain, _missing));
    EXPECT_THAT(_missing, ElementsAre("docker is not on PATH"));
    EXPECT_THAT(_processes.commands, IsEmpty());
  }

  // running

  class ToolchainRunTest : public ::testing::Test
  {
  protected:
    std::shared_ptr<RecordingLogger> _logger = std::make_shared<RecordingLogger>();
    FakeProcessContext _processes;
    ToolchainRun _run{&_processes, _logger};
    BuildTarget _target{.project = "/Users/me/Dev/my-game", .name = "my-game", .engine = "/Users/me/Dev/neon-engine"};
    ProcessCommand _command;
    std::string _error;

    static Toolchain Native()
    {
      return {
        .name = "macos-arm64", .kind = ToolchainKind::Native, .platform = "darwin-arm64",
        .configure = "cmake -S {project} -B {build} --preset macos-arm64-release",
        .build = "cmake --build {build}",
        .run = "{build}/{name}/{name}"
      };
    }

    static Toolchain Docker()
    {
      return {
        .name = "linux-x64", .kind = ToolchainKind::Docker, .platform = "linux-x86_64", .image = "neon-engine/linux-x64",
        .configure = "cmake -S {project} -B {build} --preset linux-x64-release",
        .build = "cmake --build {build}"
      };
    }
  };

  TEST_F(ToolchainRunTest, ANativeToolchainStartsTheFirstWordInTheProject)
  {
    ASSERT_TRUE(_run.Command(Native(), ToolchainStep::Configure, _target, _command, _error)) << _error;
    EXPECT_EQ(_command.program, "cmake");
    EXPECT_THAT(_command.arguments, ElementsAre("-S", "/Users/me/Dev/my-game", "-B", "/Users/me/Dev/my-game/build/macos-arm64", "--preset", "macos-arm64-release"));
    EXPECT_EQ(_command.directory, "/Users/me/Dev/my-game");
  }

  TEST_F(ToolchainRunTest, TheRunStepStartsTheResultNamedAfterTheProject)
  {
    ASSERT_TRUE(_run.Command(Native(), ToolchainStep::Run, _target, _command, _error)) << _error;
    EXPECT_EQ(_command.program, "/Users/me/Dev/my-game/build/macos-arm64/my-game/my-game");
    EXPECT_THAT(_command.arguments, IsEmpty());
  }

  TEST_F(ToolchainRunTest, QuotesHoldAWordTogether)
  {
    Toolchain toolchain = Native();
    toolchain.build = "cmake --build \"{build}\" -- -j 4 'one word'";

    ASSERT_TRUE(_run.Command(toolchain, ToolchainStep::Build, _target, _command, _error)) << _error;
    EXPECT_THAT(_command.arguments, ElementsAre("--build", "/Users/me/Dev/my-game/build/macos-arm64", "--", "-j", "4", "one word"));
  }

  TEST_F(ToolchainRunTest, RefusesAQuoteThatIsNotClosed)
  {
    Toolchain toolchain = Native();
    toolchain.build = "cmake --build '{build}";

    EXPECT_FALSE(_run.Command(toolchain, ToolchainStep::Build, _target, _command, _error));
    EXPECT_EQ(_error, "'build' of the toolchain macos-arm64: a quote is not closed in 'cmake --build '/Users/me/Dev/my-game/build/macos-arm64'");
  }

  TEST_F(ToolchainRunTest, ADockerToolchainStartsTheLineInAShellOfItsContainer)
  {
    ASSERT_TRUE(_run.Command(Docker(), ToolchainStep::Build, _target, _command, _error)) << _error;
    EXPECT_EQ(_command.program, "docker");
    EXPECT_THAT(_command.arguments, ElementsAre(
                  "run", "--rm",
                  "-v", "/Users/me/Dev/my-game:/src",
                  "-v", "/Users/me/Dev/neon-engine:/engine",
                  "-w", "/src",
                  "neon-engine/linux-x64",
                  "sh", "-c", "cmake --build /src/build/linux-x64"));
    EXPECT_EQ(_command.directory, "/Users/me/Dev/my-game");
  }

  TEST_F(ToolchainRunTest, ADockerToolchainMountsNoEngineWhenThereIsNone)
  {
    _target.engine.clear();

    ASSERT_TRUE(_run.Command(Docker(), ToolchainStep::Configure, _target, _command, _error)) << _error;
    EXPECT_THAT(_command.arguments, ElementsAre(
                  "run", "--rm", "-v", "/Users/me/Dev/my-game:/src", "-w", "/src", "neon-engine/linux-x64",
                  "sh", "-c", "cmake -S /src -B /src/build/linux-x64 --preset linux-x64-release"));
  }

  TEST_F(ToolchainRunTest, RefusesToRunWhatCannotRunHere)
  {
    EXPECT_FALSE(_run.Command(Docker(), ToolchainStep::Run, _target, _command, _error));
    EXPECT_EQ(_error, "the toolchain linux-x64 cannot run here: its 'run' is off");

    int exit_code = 0;
    EXPECT_FALSE(_run.Run(Docker(), ToolchainStep::Run, _target, [](const std::string &) {}, exit_code, _error));
    EXPECT_THAT(_processes.commands, IsEmpty());
  }

  TEST_F(ToolchainRunTest, RunsAStepThroughTheProcessesAndHandsTheLinesOn)
  {
    _processes.Install("cmake");
    _processes.results["cmake"] = {.lines = {"[1/2] Building", "[2/2] Linking"}, .exit_code = 0};

    std::vector<std::string> lines;
    int exit_code = -1;
    ASSERT_TRUE(_run.Run(Native(), ToolchainStep::Build, _target, [&lines](const std::string &line) { lines.push_back(line); }, exit_code, _error)) << _error;
    EXPECT_EQ(exit_code, 0);
    EXPECT_THAT(lines, ElementsAre("[1/2] Building", "[2/2] Linking"));
    ASSERT_THAT(_processes.commands, SizeIs(1));
    EXPECT_EQ(_processes.commands[0].program, "cmake");
  }

  TEST_F(ToolchainRunTest, ReportsAStepThatFailed)
  {
    _processes.Install("cmake");
    _processes.results["cmake"] = {.lines = {"error: no"}, .exit_code = 2};

    int exit_code = -1;
    ASSERT_TRUE(_run.Run(Native(), ToolchainStep::Build, _target, [](const std::string &) {}, exit_code, _error)) << _error;
    EXPECT_EQ(exit_code, 2);
    EXPECT_TRUE(_logger->Contains(neon::testing::LogLevel::Error, "failed the build of my-game with exit code 2"))
      << _logger->Messages(neon::testing::LogLevel::Error);
  }

  TEST_F(ToolchainRunTest, ReportsAProgramThatCouldNotBeStarted)
  {
    int exit_code = -1;
    EXPECT_FALSE(_run.Run(Native(), ToolchainStep::Build, _target, [](const std::string &) {}, exit_code, _error));
    EXPECT_EQ(_error, "cmake is not installed");
  }

  // one step through the processes of the operating system, as the editor
  // runs one: the shell of the platform, which every machine has

  TEST(ToolchainRunOnThisMachineTest, RunsACommandAndReadsWhatItPrints)
  {
    const auto logger = std::make_shared<RecordingLogger>();
    OS_ProcessRunner processes(logger);
    const ToolchainRun run(&processes, logger);
    const BuildTarget target{.project = ".", .name = "game"};

    Toolchain toolchain{.name = "here", .kind = ToolchainKind::Native, .platform = "any"};
#if defined(_WIN32)
    toolchain.configure = "cmd /c \"echo configured {name}\"";
    toolchain.build = "cmd /c \"exit 4\"";
#else
    toolchain.configure = "sh -c 'echo configured {name}'";
    toolchain.build = "sh -c 'exit 4'";
#endif

    std::vector<std::string> lines;
    int exit_code = -1;
    std::string error;
    ASSERT_TRUE(run.Run(toolchain, ToolchainStep::Configure, target, [&lines](const std::string &line) { lines.push_back(line); }, exit_code, error)) << error;
    EXPECT_EQ(exit_code, 0);
    EXPECT_THAT(lines, ElementsAre("configured game"));

    ASSERT_TRUE(run.Run(toolchain, ToolchainStep::Build, target, [](const std::string &) {}, exit_code, error)) << error;
    EXPECT_EQ(exit_code, 4);
  }
} // namespace
