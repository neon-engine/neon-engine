// The file of a project, as an application reads it: ProjectFile with the
// document format for YAML. Files are kept in memory, except for the project
// of the sandbox, which is read as it is in the repository.

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
  using neon::SettingControl;
  using neon::SettingKind;
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

  // the quality presets

  TEST_F(ProjectFilesTest, BringsTheEnginesPresetsWithoutAWord)
  {
    Write(complete);

    ASSERT_TRUE(Read()) << ::testing::PrintToString(_errors);
    EXPECT_THAT(_project.graphics_presets.Names(), ElementsAre("low", "medium", "high", "ultra", "custom"));
    EXPECT_EQ(_project.graphics_presets.Named("low")->anisotropy, 2);
  }

  TEST_F(ProjectFilesTest, ABuiltInPresetIsChangedValueByValue)
  {
    Write(complete +
          "graphics_presets:\n"
          "  low:\n"
          "    texture_scale: 1\n"
          "    shadow_filter: pcf\n");

    ASSERT_TRUE(Read()) << ::testing::PrintToString(_errors);
    const neon::GraphicsPreset *low = _project.graphics_presets.Named("low");
    ASSERT_NE(low, nullptr);
    EXPECT_DOUBLE_EQ(low->texture_scale, 1.0);
    EXPECT_EQ(low->shadow_filter, neon::ShadowFilter::Pcf);
    EXPECT_EQ(low->anisotropy, 2) << "what is not written stays the engine's";
    EXPECT_EQ(low->shadow_cascades, 1);
    EXPECT_THAT(_project.graphics_presets.Names(), ElementsAre("low", "medium", "high", "ultra", "custom"))
      << "and it keeps its place";
  }

  TEST_F(ProjectFilesTest, ANewPresetComesAfterTheBuiltInOnesWithTheEnginesDefaults)
  {
    Write(complete +
          "graphics_presets:\n"
          "  potato:\n"
          "    anisotropy: 1\n"
          "    texture_scale: 0.25\n"
          "    target_scale: 0.5\n"
          "    target_mipmaps: 1\n"
          "    shadow_map_size: 512\n"
          "    shadow_filter: none\n"
          "    shadow_cascades: 1\n"
          "    shadow_distance: 20\n"
          "  cinematic:\n"
          "    shadow_distance: 300\n");

    ASSERT_TRUE(Read()) << ::testing::PrintToString(_errors);
    EXPECT_THAT(_project.graphics_presets.Names(),
                ElementsAre("low", "medium", "high", "ultra", "potato", "cinematic", "custom"));

    const neon::GraphicsPreset *potato = _project.graphics_presets.Named("potato");
    ASSERT_NE(potato, nullptr);
    EXPECT_EQ(potato->anisotropy, 1);
    EXPECT_DOUBLE_EQ(potato->texture_scale, 0.25);
    EXPECT_DOUBLE_EQ(potato->target_scale, 0.5);
    EXPECT_EQ(potato->target_mipmaps, 1);
    EXPECT_EQ(potato->shadow_map_size, 512);
    EXPECT_EQ(potato->shadow_filter, neon::ShadowFilter::None);
    EXPECT_EQ(potato->shadow_cascades, 1);
    EXPECT_DOUBLE_EQ(potato->shadow_distance, 20.0);

    const neon::GraphicsPreset *cinematic = _project.graphics_presets.Named("cinematic");
    ASSERT_NE(cinematic, nullptr);
    EXPECT_DOUBLE_EQ(cinematic->shadow_distance, 300.0);
    EXPECT_EQ(cinematic->anisotropy, 8) << "what it leaves out is the engine's default, which is high";
    EXPECT_EQ(cinematic->shadow_map_size, 2048);
  }

  TEST_F(ProjectFilesTest, APresetTurnedOffIsDropped)
  {
    Write(complete + "graphics_presets:\n  ultra: off\n  low: off\n");

    ASSERT_TRUE(Read()) << ::testing::PrintToString(_errors);
    EXPECT_THAT(_project.graphics_presets.Names(), ElementsAre("medium", "high", "custom"));
    EXPECT_FALSE(_project.graphics_presets.IsName("ultra"));
  }

  TEST_F(ProjectFilesTest, RefusesTurningOffAPresetThatIsNotThere)
  {
    Write(complete + "graphics_presets:\n  best: off\n");

    ExpectRefused("assets://project.yml:11: 'best' of 'graphics_presets' is off, and there is no preset best to drop. "
                  "There are: low, medium, high, ultra");
  }

  TEST_F(ProjectFilesTest, RefusesDroppingEveryPreset)
  {
    Write(complete + "graphics_presets:\n  low: off\n  medium: off\n  high: off\n  ultra: off\n");

    ExpectRefused("'graphics_presets' drops every preset, and the settings menu offers at least one");
  }

  TEST_F(ProjectFilesTest, RefusesDefiningCustom)
  {
    Write(complete + "graphics_presets:\n  custom:\n    anisotropy: 1\n");

    ExpectRefused("assets://project.yml:11: 'custom' of 'graphics_presets' is the name of no preset: it is what the "
                  "settings menu shows when the values match none, and cannot be defined or dropped");
  }

  TEST_F(ProjectFilesTest, RefusesAPresetWhoseNameIsNotPlain)
  {
    Write(complete + "graphics_presets:\n  Very Low:\n    anisotropy: 1\n");

    ExpectRefused("'Very Low' of 'graphics_presets' is not a plain name: lowercase letters, digits, and dashes, "
                  "starting with a letter, as a preset is named in the settings and on the command line");
  }

  TEST_F(ProjectFilesTest, RefusesAPresetThatIsNeitherAMapNorOff)
  {
    Write(complete + "graphics_presets:\n  low: on\n  medium: [1, 2]\n");

    EXPECT_FALSE(Read());
    ASSERT_EQ(_errors.size(), 2u) << ::testing::PrintToString(_errors);
    EXPECT_THAT(_errors[0], HasSubstr("'low' of 'graphics_presets' is text, where a map of the values of the "
                                      "preset was expected, or off to drop it"));
    EXPECT_THAT(_errors[1], HasSubstr("'medium' of 'graphics_presets' is a list, where a map"));
  }

  TEST_F(ProjectFilesTest, RefusesPresetsThatAreNoMap)
  {
    Write(complete + "graphics_presets: [low, high]\n");

    ExpectRefused("assets://project.yml:10: 'graphics_presets' is a list, where a map of presets by name was expected");
  }

  TEST_F(ProjectFilesTest, RefusesAValueOfAPresetAsTheSettingsRefuseIt)
  {
    Write(complete +
          "graphics_presets:\n"
          "  potato:\n"
          "    anisotropy: 3\n"
          "    texture_scale: 0.3\n"
          "    target_scale: 2\n"
          "    target_mipmaps: 17\n"
          "    shadow_map_size: 3000\n"
          "    shadow_filter: soft\n"
          "    shadow_cascades: 5\n"
          "    shadow_distance: 0\n");

    EXPECT_FALSE(Read());
    ASSERT_EQ(_errors.size(), 8u) << ::testing::PrintToString(_errors);
    EXPECT_EQ(_errors[0], "assets://project.yml:12: 'anisotropy' of preset 'potato' of the project is 3, where 1, 2, 4, 8, "
                          "or 16 was expected");
    EXPECT_THAT(_errors[1], HasSubstr("'texture_scale' of preset 'potato' of the project is 0.3, where 1, 0.5, 0.25, or 0.125 was expected"));
    EXPECT_THAT(_errors[2], HasSubstr("'target_scale' of preset 'potato' of the project is 2, where 1, 0.5, or 0.25 was expected"));
    EXPECT_THAT(_errors[3], HasSubstr("'target_mipmaps' of preset 'potato' of the project is 17, where 0 for as many as the size allows, or 1 to 16 was expected"));
    EXPECT_THAT(_errors[4], HasSubstr("'shadow_map_size' of preset 'potato' of the project is 3000, where 512, 1024, 2048, or 4096 was expected"));
    EXPECT_THAT(_errors[5], HasSubstr("'shadow_filter'"));
    EXPECT_THAT(_errors[6], HasSubstr("'shadow_cascades' of preset 'potato' of the project is 5, where 1 to 4 was expected"));
    EXPECT_THAT(_errors[7], HasSubstr("'shadow_distance' of preset 'potato' of the project is 0, where a number above zero was expected"));
  }

  TEST_F(ProjectFilesTest, RefusesAValueOfAPresetItDoesNotKnow)
  {
    Write(complete + "graphics_presets:\n  low:\n    vsync: false\n");

    ExpectRefused("assets://project.yml:12: 'vsync' is not known to preset 'low' of the project. Known are: ");
  }

  TEST_F(ProjectFilesTest, LeavesTheProjectAloneWhenAPresetIsWrong)
  {
    Write(complete + "graphics_presets:\n  potato:\n    anisotropy: 3\n");

    EXPECT_FALSE(Read());
    EXPECT_THAT(_project.graphics_presets.Names(), ElementsAre("low", "medium", "high", "ultra", "custom"));
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

  // the project of the sandbox

  // the settings of the game

  const std::string with_settings =
    "version: 1\n"
    "name: a-game\n"
    "scenes:\n"
    "  - assets://scenes/demo.scene.yml\n"
    "settings:\n"
    "  difficulty:\n"
    "    kind: choice\n"
    "    default: normal\n"
    "    choices:\n"
    "      - easy\n"
    "      - normal\n"
    "      - hard\n"
    "  subtitles:\n"
    "    kind: flag\n"
    "    default: true\n"
    "  field_of_view:\n"
    "    kind: number\n"
    "    default: 90\n"
    "    least: 60\n"
    "    most: 120\n"
    "  player_name:\n"
    "    kind: text\n"
    "    default: Ada\n";

  /// A project with these declarations under `settings`.
  std::string Declaring(const std::string &settings)
  {
    return "version: 1\nname: a-game\nscenes:\n  - assets://scenes/demo.scene.yml\nsettings:\n" + settings;
  }

  TEST_F(ProjectFilesTest, ReadsTheSettingsTheGameDeclares)
  {
    Write(with_settings);

    ASSERT_TRUE(Read()) << ::testing::PrintToString(_errors);
    ASSERT_EQ(_project.settings.size(), 4u);

    const auto &difficulty = _project.settings[0];
    EXPECT_EQ(difficulty.name, "difficulty");
    EXPECT_EQ(difficulty.kind, SettingKind::Choice);
    EXPECT_THAT(difficulty.choices, ElementsAre("easy", "normal", "hard"));
    std::string text;
    EXPECT_TRUE(difficulty.default_value.GetText(text));
    EXPECT_EQ(text, "normal");
    EXPECT_TRUE(difficulty.value.IsEmpty()) << "nothing set it: the settings files do";

    const auto &subtitles = _project.settings[1];
    EXPECT_EQ(subtitles.kind, SettingKind::Flag);
    bool flag = false;
    EXPECT_TRUE(subtitles.default_value.GetBool(flag));
    EXPECT_TRUE(flag);

    const auto &field_of_view = _project.settings[2];
    EXPECT_EQ(field_of_view.kind, SettingKind::Number);
    ASSERT_TRUE(field_of_view.least.has_value());
    ASSERT_TRUE(field_of_view.most.has_value());
    EXPECT_DOUBLE_EQ(*field_of_view.least, 60.0);
    EXPECT_DOUBLE_EQ(*field_of_view.most, 120.0);

    EXPECT_EQ(_project.settings[3].kind, SettingKind::Text);
    EXPECT_FALSE(_project.settings[3].least.has_value());
  }

  TEST_F(ProjectFilesTest, TakesTheNaturalControlForAKind)
  {
    Write(with_settings);
    ASSERT_TRUE(Read()) << ::testing::PrintToString(_errors);
    EXPECT_EQ(_project.settings[0].GetControl(), SettingControl::Dropdown) << "a choice";
    EXPECT_EQ(_project.settings[1].GetControl(), SettingControl::Toggle) << "a flag";
    EXPECT_EQ(_project.settings[2].GetControl(), SettingControl::Slider) << "a number with a least and a most";
    EXPECT_EQ(_project.settings[3].GetControl(), SettingControl::Field) << "a text";
  }

  TEST_F(ProjectFilesTest, ReadsWhereAndHowTheMenuShowsASetting)
  {
    Write(Declaring(
      "  field_of_view:\n"
      "    kind: number\n"
      "    default: 90\n"
      "    least: 60\n"
      "    most: 120\n"
      "    category: Controls\n"
      "    group: Camera\n"
      "    order: 2\n"
      "    control: slider\n"
      "    step: 5\n"
      "    label: Field of view\n"
      "  invert_look:\n"
      "    kind: flag\n"
      "    default: false\n"
      "    control: checkbox\n"
      "  reset_progress:\n"
      "    kind: action\n"
      "    category: Gameplay\n"
      "  sensitivity:\n"
      "    kind: number\n"
      "    default: 1\n"
      "    control: field\n"));

    ASSERT_TRUE(Read()) << ::testing::PrintToString(_errors);
    ASSERT_EQ(_project.settings.size(), 4u);

    const auto &field_of_view = _project.settings[0];
    EXPECT_EQ(field_of_view.category, "Controls");
    EXPECT_EQ(field_of_view.group, "Camera");
    EXPECT_DOUBLE_EQ(field_of_view.order, 2.0);
    EXPECT_EQ(field_of_view.GetControl(), SettingControl::Slider);
    ASSERT_TRUE(field_of_view.step.has_value());
    EXPECT_DOUBLE_EQ(*field_of_view.step, 5.0);
    EXPECT_EQ(field_of_view.GetLabel(), "Field of view");

    const auto &invert_look = _project.settings[1];
    EXPECT_EQ(invert_look.GetControl(), SettingControl::Checkbox);
    EXPECT_EQ(invert_look.GetLabel(), "Invert look") << "the name, with spaces and a capital";
    EXPECT_TRUE(invert_look.category.empty());
    EXPECT_DOUBLE_EQ(invert_look.order, 0.0);

    const auto &reset_progress = _project.settings[2];
    EXPECT_EQ(reset_progress.kind, SettingKind::Action);
    EXPECT_EQ(reset_progress.GetControl(), SettingControl::Button);
    EXPECT_TRUE(reset_progress.default_value.IsEmpty());

    EXPECT_EQ(_project.settings[3].GetControl(), SettingControl::Field);
  }

  TEST_F(ProjectFilesTest, RefusesADeclarationWithoutAKindOrADefault)
  {
    Write(Declaring(
      "  difficulty:\n"
      "    default: normal\n"
      "  subtitles:\n"
      "    kind: flag\n"));
    EXPECT_FALSE(Read());
    ASSERT_EQ(_errors.size(), 2u) << ::testing::PrintToString(_errors);
    EXPECT_THAT(_errors[0], HasSubstr("'kind' is missing"));
    EXPECT_THAT(_errors[1], HasSubstr("'default' is missing"));
  }

  TEST_F(ProjectFilesTest, RefusesADeclarationWhoseDefaultDoesNotFit)
  {
    Write(Declaring(
      "  difficulty:\n"
      "    kind: choice\n"
      "    default: nightmare\n"
      "    choices: [easy, hard]\n"
      "  field_of_view:\n"
      "    kind: number\n"
      "    default: 200\n"
      "    least: 60\n"
      "    most: 120\n"));
    EXPECT_FALSE(Read());
    ASSERT_EQ(_errors.size(), 2u) << ::testing::PrintToString(_errors);
    EXPECT_THAT(_errors[0], HasSubstr("'default' of 'difficulty' of 'settings' of the project does not fit: 'nightmare' is none of the choices, which are: easy, hard"));
    EXPECT_THAT(_errors[1], HasSubstr("200 is above the most, which is 120"));
    EXPECT_TRUE(_project.settings.empty());
  }

  TEST_F(ProjectFilesTest, RefusesAKindItDoesNotKnowAndAChoiceWithoutChoices)
  {
    Write(Declaring(
      "  difficulty:\n"
      "    kind: choice\n"
      "    default: normal\n"
      "  volume:\n"
      "    kind: slider\n"
      "    default: 1\n"));
    EXPECT_FALSE(Read());
    ASSERT_EQ(_errors.size(), 2u) << ::testing::PrintToString(_errors);
    EXPECT_THAT(_errors[0], HasSubstr("'choices' is missing"));
    EXPECT_THAT(_errors[1], HasSubstr("'kind' of 'volume' of 'settings' of the project is 'slider', where flag, number, text, choice, or action was expected"));
  }

  TEST_F(ProjectFilesTest, RefusesANameThatIsNoneAndWhatAKindHasNoUseFor)
  {
    Write(Declaring(
      "  player-name:\n"
      "    kind: text\n"
      "    default: Ada\n"
      "  subtitles:\n"
      "    kind: flag\n"
      "    default: true\n"
      "    least: 0\n"));
    EXPECT_FALSE(Read());
    ASSERT_EQ(_errors.size(), 2u) << ::testing::PrintToString(_errors);
    EXPECT_THAT(_errors[0], HasSubstr("'player-name' of 'settings' of the project is not the name of a setting"));
    EXPECT_THAT(_errors[1], HasSubstr("'least' is not known to 'subtitles' of 'settings' of the project"));
  }

  TEST_F(ProjectFilesTest, RefusesAControlThatDoesNotShowTheKind)
  {
    Write(Declaring(
      "  subtitles:\n"
      "    kind: flag\n"
      "    default: true\n"
      "    control: slider\n"
      "  sensitivity:\n"
      "    kind: number\n"
      "    default: 1\n"
      "    control: slider\n"
      "  difficulty:\n"
      "    kind: choice\n"
      "    default: easy\n"
      "    choices: [easy, hard]\n"
      "    control: knob\n"
      "  reset_progress:\n"
      "    kind: action\n"
      "    default: true\n"));
    EXPECT_FALSE(Read());
    ASSERT_EQ(_errors.size(), 4u) << ::testing::PrintToString(_errors);
    EXPECT_THAT(_errors[0], HasSubstr("'control' of 'subtitles' of 'settings' of the project is slider, which does not show flag"));
    EXPECT_THAT(_errors[1], HasSubstr("is a slider, which needs 'least' and 'most'"));
    EXPECT_THAT(_errors[2], HasSubstr("'control' of 'difficulty' of 'settings' of the project is 'knob'"));
    EXPECT_THAT(_errors[3], HasSubstr("'default' of 'reset_progress' of 'settings' of the project is not taken: an action holds nothing"));
  }

  TEST_F(ProjectFilesTest, RefusesASettingThatIsNotDeclaredAsAMap)
  {
    Write(Declaring("  difficulty: hard\n"));
    EXPECT_FALSE(Read());
    ASSERT_EQ(_errors.size(), 1u) << ::testing::PrintToString(_errors);
    EXPECT_THAT(_errors[0], HasSubstr("'difficulty' of 'settings' of the project is text, where a map with the kind and the default of the setting was expected. What a setting is set to is in settings.yml under 'game'"));
  }

  TEST_F(ProjectFilesTest, RefusesSettingsThatAreNotAMap)
  {
    Write(Declaring("  - difficulty\n"));
    EXPECT_FALSE(Read());
    ASSERT_EQ(_errors.size(), 1u) << ::testing::PrintToString(_errors);
    EXPECT_THAT(_errors[0], HasSubstr("'settings' of the project is a list, where a map of settings was expected"));
  }

  TEST_F(ProjectFilesTest, ReadsTheProjectOfTheSandbox)
  {
    Write(FileOfTheSandbox("project.yml"));

    ASSERT_TRUE(Read()) << ::testing::PrintToString(_errors);
    EXPECT_EQ(_project.name, "neon-sandbox");
    EXPECT_EQ(_project.organization, "neon-engine");
    EXPECT_EQ(_project.entry_scene, "assets://scenes/start.scene.yml");
  }

  TEST_F(ProjectFilesTest, TheProjectOfTheSandboxListsEveryScene)
  {
    Write(FileOfTheSandbox("project.yml"));
    ASSERT_TRUE(Read()) << ::testing::PrintToString(_errors);

    const std::filesystem::path scenes = std::filesystem::path(NEON_SANDBOX_ASSETS) / "scenes";
    std::vector<std::string> on_disk;
    for (const auto &entry : std::filesystem::directory_iterator(scenes))
    {
      if (entry.path().extension() == ".yml") { on_disk.push_back("assets://scenes/" + entry.path().filename().string()); }
    }

    EXPECT_THAT(_project.scenes, ::testing::UnorderedElementsAreArray(on_disk));
  }
} // namespace
