// The file of a project, as an application reads it: ProjectFile with the
// document format for YAML. Files are kept in memory, except for the project
// of the runtime, which is read as it is in the repository.

#include <filesystem>
#include <fstream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <neon/data/ryml-document-format.hpp>
#include <neon/project/project-file.hpp>
#include <neon/testing/memory-file-system.hpp>
#include <neon/testing/recording-logger.hpp>

namespace
{
  using neon::Project;
  using neon::ProjectFile;
  using neon::RYML_DocumentFormat;
  using neon::testing::MemoryFileSystem;
  using neon::testing::RecordingLogger;
  using ::testing::ElementsAre;
  using ::testing::HasSubstr;
  using ::testing::IsEmpty;

  const std::string complete =
    "version: 1\n"
    "name: neon-runtime\n"
    "organization: neon-engine\n"
    "\n"
    "scenes:\n"
    "  - assets://scenes/demo.scene.yml\n"
    "  - assets://scenes/physics.scene.yml\n"
    "\n"
    "entry_scene: assets://scenes/physics.scene.yml\n";

  class ProjectFilesTest : public ::testing::Test
  {
  protected:
    std::shared_ptr<RecordingLogger> _logger = std::make_shared<RecordingLogger>();
    MemoryFileSystem _files{SettingsConfig{}, _logger};
    RYML_DocumentFormat _yaml;
    ProjectFile _file{&_files, &_yaml, _logger};
    Project _project;
    std::vector<std::string> _errors;

    void SetUp() override
    {
      _files.Initialize();
    }

    /// Puts the text where the project file is.
    void Write(const std::string &text)
    {
      _files.AddNativeFile("/assets/project.yml", text);
    }

    bool Read()
    {
      return _file.Read(_project, _errors);
    }

    /// Reads, and expects it to fail with one error that holds the text.
    void ExpectRefused(const std::string &text)
    {
      EXPECT_FALSE(Read());
      ASSERT_EQ(_errors.size(), 1u) << ::testing::PrintToString(_errors);
      EXPECT_THAT(_errors.front(), HasSubstr(text));
    }

    static std::string FileOfTheRuntime(const std::string &name)
    {
      const std::filesystem::path path = std::filesystem::path(NEON_RUNTIME_ASSETS) / name;
      const std::ifstream file(path);
      EXPECT_TRUE(file.good()) << path;

      std::stringstream text;
      text << file.rdbuf();
      return text.str();
    }
  };

  // reading

  TEST_F(ProjectFilesTest, ReadsEverything)
  {
    Write(complete);

    ASSERT_TRUE(Read()) << ::testing::PrintToString(_errors);
    EXPECT_EQ(_project.name, "neon-runtime");
    EXPECT_EQ(_project.organization, "neon-engine");
    EXPECT_THAT(_project.scenes, ElementsAre("assets://scenes/demo.scene.yml", "assets://scenes/physics.scene.yml"));
    EXPECT_EQ(_project.entry_scene, "assets://scenes/physics.scene.yml");
    EXPECT_THAT(_errors, IsEmpty());
  }

  TEST_F(ProjectFilesTest, PlaysWithTheEnginesInputMapWhenNoneIsNamed)
  {
    Write(complete);

    ASSERT_TRUE(Read()) << ::testing::PrintToString(_errors);
    EXPECT_EQ(_project.input, "");
  }

  TEST_F(ProjectFilesTest, ReadsTheInputMapTheProjectPlaysWith)
  {
    Write(complete + "input: assets://input/game.input.yml\n");

    ASSERT_TRUE(Read()) << ::testing::PrintToString(_errors);
    EXPECT_EQ(_project.input, "assets://input/game.input.yml");
  }

  TEST_F(ProjectFilesTest, RefusesAnEmptyInputMap)
  {
    Write(complete + "input: \"\"\n");

    ExpectRefused("assets://project.yml:10: 'input' is empty. It is the path of the input map, or left out");
  }

  TEST_F(ProjectFilesTest, StartsWithTheFirstSceneWhenNoneIsNamed)
  {
    Write(
      "version: 1\n"
      "name: game\n"
      "organization: maker\n"
      "scenes: [assets://scenes/one.scene.yml, assets://scenes/two.scene.yml]\n");

    ASSERT_TRUE(Read()) << ::testing::PrintToString(_errors);
    EXPECT_EQ(_project.entry_scene, "assets://scenes/one.scene.yml");
  }

  TEST_F(ProjectFilesTest, LeavesTheProjectAloneWhenTheFileIsWrong)
  {
    _project.name = "before";
    Write("version: 1\nname: Game\norganization: maker\nscenes: [a]\n");

    EXPECT_FALSE(Read());
    EXPECT_EQ(_project.name, "before");
  }

  TEST_F(ProjectFilesTest, SaysWhereTheFileIs)
  {
    EXPECT_EQ(ProjectFile::path, "assets://project.yml");
  }

  // what is refused

  TEST_F(ProjectFilesTest, RefusesAMissingFile)
  {
    ExpectRefused("assets://project.yml: there is no project here, the file cannot be read");
  }

  TEST_F(ProjectFilesTest, RefusesTextThatIsNoDocument)
  {
    Write("name: [\n");

    EXPECT_FALSE(Read());
    EXPECT_EQ(_errors.size(), 1u) << ::testing::PrintToString(_errors);
  }

  TEST_F(ProjectFilesTest, RefusesAMissingVersion)
  {
    Write("name: game\norganization: maker\nscenes: [a]\n");

    ExpectRefused("'version' is missing. It holds the version of the layout, which is 1");
  }

  TEST_F(ProjectFilesTest, RefusesAVersionAboveItsOwn)
  {
    Write("version: 2\nname: game\norganization: maker\nscenes: [a]\n");

    ExpectRefused("assets://project.yml:1: the project has version 2, and this engine reads up to version 1");
  }

  TEST_F(ProjectFilesTest, RefusesAVersionThatIsNoNumber)
  {
    Write("version: one\nname: game\norganization: maker\nscenes: [a]\n");

    ExpectRefused("'version' of the project is text, where a whole number was expected");
  }

  TEST_F(ProjectFilesTest, RefusesAMissingName)
  {
    Write("version: 1\norganization: maker\nscenes: [a]\n");

    ExpectRefused("'name' is missing. It is what the project is called");
  }

  TEST_F(ProjectFilesTest, RefusesAnEmptyName)
  {
    Write("version: 1\nname: \"\"\norganization: maker\nscenes: [a]\n");

    ExpectRefused("assets://project.yml:2: 'name' is empty. It is what the project is called");
  }

  TEST_F(ProjectFilesTest, AProjectWithoutAnOrganizationIsOneOfTheEngines)
  {
    Write("version: 1\nname: game\nscenes: [a]\n");

    ASSERT_TRUE(Read()) << ::testing::PrintToString(_errors);
    EXPECT_EQ(_project.organization, "neon-engine");
  }

  TEST_F(ProjectFilesTest, AnEmptyOrganizationIsTheEnginesToo)
  {
    Write("version: 1\nname: game\norganization: \"\"\nscenes: [a]\n");

    ASSERT_TRUE(Read()) << ::testing::PrintToString(_errors);
    EXPECT_EQ(_project.organization, "neon-engine");
  }

  TEST_F(ProjectFilesTest, RefusesAnOrganizationThatIsNotPlain)
  {
    Write("version: 1\nname: game\norganization: My Studio\nscenes: [a]\n");

    ExpectRefused("'organization' is 'My Studio', where a plain name was expected");
  }

  TEST_F(ProjectFilesTest, RefusesANameThatIsNotPlain)
  {
    Write("version: 1\nname: My Game\norganization: maker\nscenes: [a]\n");

    ExpectRefused(
      "assets://project.yml:2: 'name' is 'My Game', where a plain name was expected: lowercase letters, digits, "
      "and dashes, starting with a letter. It becomes a folder name on every platform");
  }

  TEST_F(ProjectFilesTest, RefusesMissingScenes)
  {
    Write("version: 1\nname: game\norganization: maker\n");

    ExpectRefused("'scenes' is missing. It lists the scenes of the project");
  }

  TEST_F(ProjectFilesTest, RefusesAnEmptyListOfScenes)
  {
    Write("version: 1\nname: game\norganization: maker\nscenes: []\n");

    ExpectRefused("assets://project.yml:4: 'scenes' is empty, a project has at least one scene");
  }

  TEST_F(ProjectFilesTest, RefusesAnEntrySceneThatIsNotListed)
  {
    Write("version: 1\nname: game\norganization: maker\nscenes: [a]\nentry_scene: b\n");

    ExpectRefused("assets://project.yml:5: 'entry_scene' is 'b', which is not one of the scenes");
  }

  TEST_F(ProjectFilesTest, RefusesANameItDoesNotKnow)
  {
    Write("version: 1\nname: game\norganization: maker\nscenes: [a]\nentry: a\n");

    ExpectRefused("assets://project.yml:5: 'entry' is not known to the project. Known are: ");
  }

  TEST_F(ProjectFilesTest, ReportsEveryProblemNotOnlyTheFirst)
  {
    Write("version: 1\nname: My Game\nscenes: []\nentry: a\n");

    EXPECT_FALSE(Read());
    EXPECT_EQ(_errors.size(), 3u) << ::testing::PrintToString(_errors);
  }

  // plain names

  TEST(ProjectFile, AcceptsPlainNames)
  {
    EXPECT_TRUE(ProjectFile::IsPlainName("neon-runtime"));
    EXPECT_TRUE(ProjectFile::IsPlainName("a"));
    EXPECT_TRUE(ProjectFile::IsPlainName("game2"));
  }

  TEST(ProjectFile, RefusesNamesThatAreNotPlain)
  {
    EXPECT_FALSE(ProjectFile::IsPlainName(""));
    EXPECT_FALSE(ProjectFile::IsPlainName("Game"));
    EXPECT_FALSE(ProjectFile::IsPlainName("my game"));
    EXPECT_FALSE(ProjectFile::IsPlainName("2games"));
    EXPECT_FALSE(ProjectFile::IsPlainName("-game"));
    EXPECT_FALSE(ProjectFile::IsPlainName("game_two"));
    EXPECT_FALSE(ProjectFile::IsPlainName("maker/game"));
  }

  // the project of the runtime

  TEST_F(ProjectFilesTest, ReadsTheProjectOfTheRuntime)
  {
    Write(FileOfTheRuntime("project.yml"));

    ASSERT_TRUE(Read()) << ::testing::PrintToString(_errors);
    EXPECT_EQ(_project.name, "neon-runtime");
    EXPECT_EQ(_project.organization, "neon-engine");
    EXPECT_EQ(_project.entry_scene, "assets://scenes/start.scene.yml");
  }

  TEST_F(ProjectFilesTest, TheProjectOfTheRuntimeListsEveryScene)
  {
    Write(FileOfTheRuntime("project.yml"));
    ASSERT_TRUE(Read()) << ::testing::PrintToString(_errors);

    const std::filesystem::path scenes = std::filesystem::path(NEON_RUNTIME_ASSETS) / "scenes";
    std::vector<std::string> on_disk;
    for (const auto &entry : std::filesystem::directory_iterator(scenes))
    {
      if (entry.path().extension() == ".yml") { on_disk.push_back("assets://scenes/" + entry.path().filename().string()); }
    }

    EXPECT_THAT(_project.scenes, ::testing::UnorderedElementsAreArray(on_disk));
  }
} // namespace
