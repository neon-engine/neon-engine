// The file of a built game, as the runtime reads it and as the export
// writes it: GameFile with the document format for YAML, next to
// ProjectFile. Files are kept in memory, except for the project of the
// sandbox, which is read as it is in the repository, written as a game, and
// read back.

#include <filesystem>
#include <fstream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <neon/data/ryml-document-format.hpp>
#include <neon/project/game-file.hpp>
#include <neon/project/project-file.hpp>
#include <neon/testing/memory-file-system.hpp>
#include <neon/testing/recording-logger.hpp>

namespace
{
  using neon::DataValue;
  using neon::GameFile;
  using neon::GraphicsPreset;
  using neon::Project;
  using neon::ProjectFile;
  using neon::RYML_DocumentFormat;
  using neon::SettingControl;
  using neon::SettingDeclaration;
  using neon::SettingKind;
  using neon::testing::MemoryFileSystem;
  using neon::testing::RecordingLogger;
  using ::testing::ElementsAre;
  using ::testing::HasSubstr;
  using ::testing::IsEmpty;

  const std::string complete =
    "version: 1\n"
    "name: a-game\n"
    "organization: maker\n"
    "\n"
    "scenes:\n"
    "  - assets://scenes/title.scene.yml\n"
    "  - assets://scenes/level.scene.yml\n"
    "\n"
    "entry_scene: assets://scenes/level.scene.yml\n"
    "input: assets://input/game.input.yml\n"
    "\n"
    "graphics_presets:\n"
    "  ultra: off\n"
    "  low:\n"
    "    texture_scale: 1\n"
    "  potato:\n"
    "    anisotropy: 1\n"
    "    shadow_filter: none\n"
    "\n"
    "settings:\n"
    "  difficulty:\n"
    "    kind: choice\n"
    "    default: normal\n"
    "    choices:\n"
    "      - easy\n"
    "      - normal\n"
    "      - hard\n"
    "    category: Gameplay\n";

  /// Whether two values of a document hold the same.
  void ExpectSameValue(const DataValue &expected, const DataValue &read)
  {
    ASSERT_EQ(expected.GetKind(), read.GetKind());
    switch (expected.GetKind())
    {
      case DataValue::Kind::Bool:
      {
        bool a = false;
        bool b = false;
        EXPECT_TRUE(expected.GetBool(a));
        EXPECT_TRUE(read.GetBool(b));
        EXPECT_EQ(a, b);
        break;
      }
      case DataValue::Kind::Number:
      {
        double a = 0.0;
        double b = 0.0;
        EXPECT_TRUE(expected.GetNumber(a));
        EXPECT_TRUE(read.GetNumber(b));
        EXPECT_DOUBLE_EQ(a, b);
        break;
      }
      case DataValue::Kind::Text:
      {
        std::string a;
        std::string b;
        EXPECT_TRUE(expected.GetText(a));
        EXPECT_TRUE(read.GetText(b));
        EXPECT_EQ(a, b);
        break;
      }
      default:
        break;
    }
  }

  /// Whether a game read back says what the project said, value by value.
  void ExpectSameProject(const Project &expected, const Project &read)
  {
    EXPECT_EQ(read.name, expected.name);
    EXPECT_EQ(read.organization, expected.organization);
    EXPECT_EQ(read.scenes, expected.scenes);
    EXPECT_EQ(read.entry_scene, expected.entry_scene);
    EXPECT_EQ(read.input, expected.input);

    EXPECT_EQ(read.graphics_presets.Names(), expected.graphics_presets.Names());
    for (const GraphicsPreset &preset : expected.graphics_presets.All())
    {
      const GraphicsPreset *found = read.graphics_presets.Named(preset.name);
      ASSERT_NE(found, nullptr) << preset.name;
      EXPECT_TRUE(found->HasTheValuesOf(preset)) << preset.name;
    }

    ASSERT_EQ(read.settings.size(), expected.settings.size());
    for (std::size_t i = 0; i < expected.settings.size(); i++)
    {
      const SettingDeclaration &a = expected.settings[i];
      const SettingDeclaration &b = read.settings[i];
      EXPECT_EQ(b.name, a.name);
      EXPECT_EQ(b.kind, a.kind) << a.name;
      ExpectSameValue(a.default_value, b.default_value);
      EXPECT_EQ(b.least, a.least) << a.name;
      EXPECT_EQ(b.most, a.most) << a.name;
      EXPECT_EQ(b.choices, a.choices) << a.name;
      EXPECT_EQ(b.category, a.category) << a.name;
      EXPECT_EQ(b.group, a.group) << a.name;
      EXPECT_DOUBLE_EQ(b.order, a.order) << a.name;
      EXPECT_EQ(b.control, a.control) << a.name;
      EXPECT_EQ(b.step, a.step) << a.name;
      EXPECT_EQ(b.label, a.label) << a.name;
    }
  }

  class GameFilesTest : public ::testing::Test
  {
  protected:
    std::shared_ptr<RecordingLogger> _logger = std::make_shared<RecordingLogger>();
    MemoryFileSystem _files{SettingsConfig{}, _logger};
    RYML_DocumentFormat _yaml;
    GameFile _file{&_files, &_yaml, _logger};
    ProjectFile _project_file{&_files, &_yaml, _logger};
    Project _game;
    std::vector<std::string> _errors;

    void SetUp() override
    {
      _files.Initialize();
    }

    /// Puts the text where the file of the game is.
    void WriteGame(const std::string &text)
    {
      _files.AddNativeFile("/assets/game.yml", text);
    }

    bool Read()
    {
      return _file.Read(_game, _errors);
    }

    /// Reads, and expects it to fail with one error that holds the text.
    void ExpectRefused(const std::string &text)
    {
      EXPECT_FALSE(Read());
      ASSERT_EQ(_errors.size(), 1u) << ::testing::PrintToString(_errors);
      EXPECT_THAT(_errors.front(), HasSubstr(text));
    }

    /// Reads a project from its text, as the export does before it writes
    /// the game.
    Project ProjectOf(const std::string &text)
    {
      _files.AddNativeFile("/assets/project.yml", text);
      Project project;
      std::vector<std::string> errors;
      EXPECT_TRUE(_project_file.Read(project, errors)) << ::testing::PrintToString(errors);
      return project;
    }

    /// Writes the game of a project, reads it back, and expects the same.
    void ExpectRoundTrip(const Project &project)
    {
      WriteGame(_file.WriteFrom(project));
      ASSERT_TRUE(Read()) << ::testing::PrintToString(_errors);
      ExpectSameProject(project, _game);
    }

    static std::string FileOfTheSandbox(const std::string &name)
    {
      const std::filesystem::path path = std::filesystem::path(NEON_SANDBOX_ASSETS) / name;
      const std::ifstream file(path);
      EXPECT_TRUE(file.good()) << path;

      std::stringstream text;
      text << file.rdbuf();
      return text.str();
    }
  };

  // reading

  TEST_F(GameFilesTest, ReadsEverything)
  {
    WriteGame(complete);

    ASSERT_TRUE(Read()) << ::testing::PrintToString(_errors);
    EXPECT_EQ(_game.name, "a-game");
    EXPECT_EQ(_game.organization, "maker");
    EXPECT_THAT(_game.scenes, ElementsAre("assets://scenes/title.scene.yml", "assets://scenes/level.scene.yml"));
    EXPECT_EQ(_game.entry_scene, "assets://scenes/level.scene.yml");
    EXPECT_EQ(_game.input, "assets://input/game.input.yml");
    EXPECT_THAT(_game.graphics_presets.Names(), ElementsAre("low", "medium", "high", "potato", "custom"));
    EXPECT_DOUBLE_EQ(_game.graphics_presets.Named("low")->texture_scale, 1.0);
    EXPECT_EQ(_game.graphics_presets.Named("potato")->anisotropy, 1);
    ASSERT_EQ(_game.settings.size(), 1u);
    EXPECT_EQ(_game.settings[0].name, "difficulty");
    EXPECT_EQ(_game.settings[0].kind, SettingKind::Choice);
    EXPECT_THAT(_errors, IsEmpty());
  }

  TEST_F(GameFilesTest, SaysWhereTheFileIsAndThatItIsReadLikeTheProject)
  {
    EXPECT_EQ(GameFile::path, "assets://game.yml");
    EXPECT_EQ(GameFile::version, ProjectFile::version);
  }

  TEST_F(GameFilesTest, SaysThatItReadsTheGame)
  {
    WriteGame(complete);
    ASSERT_TRUE(Read()) << ::testing::PrintToString(_errors);
    EXPECT_TRUE(_logger->Contains(neon::testing::LogLevel::Info, "Reading the game from assets://game.yml"));
  }

  TEST_F(GameFilesTest, RefusesAMissingFile)
  {
    ExpectRefused("assets://game.yml: there is no game here, the file cannot be read");
  }

  TEST_F(GameFilesTest, RefusesANameItDoesNotKnowWithItsLine)
  {
    WriteGame("version: 1\nname: game\nscenes: [a]\nentry: a\ntoolchains: [clang]\n");

    EXPECT_FALSE(Read());
    ASSERT_EQ(_errors.size(), 2u) << ::testing::PrintToString(_errors);
    EXPECT_THAT(_errors[0], HasSubstr("assets://game.yml:4: 'entry' is not known to the game. Known are: "));
    EXPECT_THAT(_errors[1], HasSubstr("assets://game.yml:5: 'toolchains' is not known to the game"));
  }

  TEST_F(GameFilesTest, RefusesAVersionAboveItsOwn)
  {
    WriteGame("version: 2\nname: game\nscenes: [a]\n");

    ExpectRefused("assets://game.yml:1: the game has version 2, and this engine reads up to version 1");
  }

  TEST_F(GameFilesTest, RefusesWhatTheProjectRefuses)
  {
    WriteGame("version: 1\nname: My Game\nscenes: []\nsettings:\n  volume:\n    kind: slider\n    default: 1\n");

    EXPECT_FALSE(Read());
    ASSERT_EQ(_errors.size(), 3u) << ::testing::PrintToString(_errors);
    EXPECT_THAT(_errors[0], HasSubstr("'name' is 'My Game', where a plain name was expected"));
    EXPECT_THAT(_errors[1], HasSubstr("'scenes' is empty, a game has at least one scene"));
    EXPECT_THAT(_errors[2], HasSubstr("'kind' of 'volume' of 'settings' of the game is 'slider'"));
  }

  // writing

  TEST_F(GameFilesTest, WritesWhatAProjectSaysAndLeavesTheDefaultsOut)
  {
    Project project;
    project.name = "a-game";
    project.scenes = {"assets://scenes/one.scene.yml", "assets://scenes/two.scene.yml"};
    project.entry_scene = project.scenes.front();

    EXPECT_EQ(_file.WriteFrom(project),
              "version: 1\n"
              "name: a-game\n"
              "\n"
              "scenes:\n"
              "  - assets://scenes/one.scene.yml\n"
              "  - assets://scenes/two.scene.yml\n");
  }

  TEST_F(GameFilesTest, WritesEverythingAProjectSaysInBlockStyle)
  {
    const Project project = ProjectOf(
      "version: 1\n"
      "name: a-game\n"
      "organization: maker\n"
      "scenes: [assets://scenes/title.scene.yml, assets://scenes/level.scene.yml]\n"
      "entry_scene: assets://scenes/level.scene.yml\n"
      "input: assets://input/game.input.yml\n"
      "graphics_presets:\n"
      "  low: {texture_scale: 1, shadow_filter: pcf}\n"
      "  ultra: off\n"
      "  potato: {anisotropy: 1, shadow_cascades: 1, shadow_distance: 20}\n"
      "settings:\n"
      "  difficulty:\n"
      "    kind: choice\n"
      "    default: normal\n"
      "    choices: [easy, normal, hard]\n"
      "    category: Gameplay\n"
      "  look_sensitivity:\n"
      "    kind: number\n"
      "    default: 1\n"
      "    least: 0.1\n"
      "    most: 5\n"
      "    step: 0.1\n"
      "    group: Camera\n"
      "    order: 2\n"
      "    label: Look sensitivity\n"
      "  invert_look:\n"
      "    kind: flag\n"
      "    default: false\n"
      "    control: checkbox\n"
      "  reset_progress:\n"
      "    kind: action\n");

    EXPECT_EQ(_file.WriteFrom(project),
              "version: 1\n"
              "name: a-game\n"
              "organization: maker\n"
              "\n"
              "scenes:\n"
              "  - assets://scenes/title.scene.yml\n"
              "  - assets://scenes/level.scene.yml\n"
              "entry_scene: assets://scenes/level.scene.yml\n"
              "input: assets://input/game.input.yml\n"
              "\n"
              "graphics_presets:\n"
              "  ultra: off\n"
              "  low:\n"
              "    texture_scale: 1\n"
              "    shadow_filter: pcf\n"
              "  potato:\n"
              "    anisotropy: 1\n"
              "    shadow_cascades: 1\n"
              "    shadow_distance: 20\n"
              "\n"
              "settings:\n"
              "  difficulty:\n"
              "    kind: choice\n"
              "    default: normal\n"
              "    choices:\n"
              "      - easy\n"
              "      - normal\n"
              "      - hard\n"
              "    category: Gameplay\n"
              "  look_sensitivity:\n"
              "    kind: number\n"
              "    default: 1\n"
              "    least: 0.1\n"
              "    most: 5\n"
              "    group: Camera\n"
              "    order: 2\n"
              "    step: 0.1\n"
              "    label: Look sensitivity\n"
              "  invert_look:\n"
              "    kind: flag\n"
              "    default: false\n"
              "    control: checkbox\n"
              "  reset_progress:\n"
              "    kind: action\n");

    ExpectRoundTrip(project);
  }

  TEST_F(GameFilesTest, WritesAPresetOfTheProjectThatIsAllDefaultsAsAnEmptyMap)
  {
    const Project project = ProjectOf(
      "version: 1\nname: a-game\nscenes: [a]\ngraphics_presets:\n  plain: {}\n");

    EXPECT_THAT(_file.WriteFrom(project), HasSubstr("graphics_presets:\n  plain: {}\n"));
    ExpectRoundTrip(project);
  }

  TEST_F(GameFilesTest, LeavesTheTableOfTheEngineAlone)
  {
    const Project project = ProjectOf("version: 1\nname: a-game\nscenes: [a]\n");

    const std::string written = _file.WriteFrom(project);
    EXPECT_THAT(written, ::testing::Not(HasSubstr("graphics_presets")));
    EXPECT_THAT(written, ::testing::Not(HasSubstr("settings")));
    EXPECT_THAT(written, ::testing::Not(HasSubstr("organization")));
    EXPECT_THAT(written, ::testing::Not(HasSubstr("entry_scene")));
    ExpectRoundTrip(project);
  }

  TEST_F(GameFilesTest, WritesTheProjectOfTheSandboxAndReadsItBack)
  {
    const Project project = ProjectOf(FileOfTheSandbox("project.yml"));

    const std::string written = _file.WriteFrom(project);
    EXPECT_THAT(written, HasSubstr("name: neon-sandbox\n"));
    EXPECT_THAT(written, HasSubstr("  potato:\n    anisotropy: 1\n"));
    EXPECT_THAT(written, HasSubstr("  text_size:\n    kind: choice\n"));
    EXPECT_THAT(written, ::testing::Not(HasSubstr("organization")))
      << "the sandbox is one of the engine's own, which is the default";

    ExpectRoundTrip(project);
    EXPECT_EQ(_game.name, "neon-sandbox");
    EXPECT_EQ(_game.organization, "neon-engine");
    EXPECT_THAT(_game.graphics_presets.Names(), ElementsAre("low", "medium", "high", "ultra", "potato", "custom"));
    EXPECT_EQ(_game.settings.size(), 5u);
  }
} // namespace
