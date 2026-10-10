#include "game-menu.hpp"

#include <algorithm>
#include <cstdint>
#include <format>
#include <map>
#include <vector>
#include <memory>
#include <string>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <neon/testing/memory-file-system.hpp>
#include <neon/testing/mock-ui-system.hpp>
#include <neon/testing/recording-logger.hpp>

namespace
{
  using neon::DataValue;
  using neon::DocumentFormat;
  using neon::GameMenu;
  using neon::PlayerSettings;
  using neon::SettingControl;
  using neon::SettingDeclaration;
  using neon::SettingKind;
  using neon::UiHandle;
  using neon::SettingsStore;
  using neon::SettingsSubscription;
  using neon::testing::LogLevel;
  using neon::testing::MemoryFileSystem;
  using neon::testing::MockUiContext;
  using neon::testing::RecordingLogger;
  using ::testing::_;
  using ::testing::ElementsAre;
  using ::testing::NiceMock;

  // A format that reads nothing and writes a line, for a test that looks at
  // what would be written rather than at the text.
  class WrittenFormat final : public DocumentFormat
  {
  public:
    bool Read(const std::string &, const std::string &, DataValue &, std::string &error) override
    {
      error = "not read in this test";
      return false;
    }

    std::string Write(const DataValue &) override { return "written\n"; }
  };

  /// A value as one line, `{type: panel, class: row, children: [...]}`,
  /// for looking at what the menu makes.
  std::string Flat(const DataValue &value)
  {
    bool flag = false;
    double number = 0.0;
    std::string text;
    if (value.GetBool(flag)) { return flag ? "true" : "false"; }
    if (value.GetNumber(number)) { return std::format("{}", number); }
    if (value.GetText(text)) { return text; }
    std::string line;
    if (value.IsList())
    {
      for (const auto &item : value.GetItems()) { line += (line.empty() ? "" : ", ") + Flat(item); }
      return "[" + line + "]";
    }
    for (const auto &[name, entry] : value.GetEntries()) { line += (line.empty() ? "" : ", ") + name + ": " + Flat(entry); }
    return "{" + line + "}";
  }

  /// A user interface whose document 7 has an element named `list`, and
  /// the elements of `existing`, and which records what is made inside it.
  class BuildingUi final : public NiceMock<MockUiContext>
  {
  public:
    std::vector<std::string> existing;
    std::vector<std::string> made;

    [[nodiscard]] UiHandle GetRoot(const int document) const override
    {
      return document == 7 ? UiHandle{.id = 1} : UiHandle{};
    }

    [[nodiscard]] UiHandle FindByName(const std::string &name, const UiHandle from) const override
    {
      if (from.id != 1) { return {}; }
      if (name == "list") { return UiHandle{.id = 2}; }
      if (std::ranges::find(existing, name) != existing.end()) { return UiHandle{.id = 3}; }
      return {};
    }

    UiHandle CreateFrom(const DataValue &description, const UiHandle parent, int) override
    {
      if (parent.id != 2) { return {}; }
      made.push_back(Flat(description));
      return UiHandle{.id = 10 + static_cast<std::uint64_t>(made.size())};
    }
  };

  class GameMenuTest : public ::testing::Test
  {
  protected:
    std::shared_ptr<RecordingLogger> _logger = std::make_shared<RecordingLogger>();
    BuildingUi _ui;
    MemoryFileSystem _files{SettingsConfig{}, _logger};
    WrittenFormat _format;
    PlayerSettings _player{&_files, &_format, _logger};
    SettingsStore _store{_logger};

    // what the user interface holds, as text, as the menu's elements would
    std::map<std::string, std::string> _values;

    void SetUp() override
    {
      _files.Initialize();

      ON_CALL(_ui, SetText(_, _)).WillByDefault([this](const std::string &name, const std::string &text)
      {
        _values[name] = text;
      });
      ON_CALL(_ui, SetFlag(_, _)).WillByDefault([this](const std::string &name, const bool flag)
      {
        _values[name] = flag ? "true" : "false";
      });
      ON_CALL(_ui, SetNumber(_, _)).WillByDefault([this](const std::string &name, const double number)
      {
        _values[name] = std::format("{}", number);
      });
      ON_CALL(_ui, GetValue(_, _)).WillByDefault([this](const std::string &name, bool *is_set)
      {
        const auto found = _values.find(name);
        if (is_set != nullptr) { *is_set = found != _values.end(); }
        return found != _values.end() ? found->second : std::string();
      });

      ASSERT_TRUE(_store.Declare({.name = "subtitles", .kind = SettingKind::Flag, .default_value = DataValue::Bool(true)}));
      ASSERT_TRUE(_store.Declare({
        .name = "field_of_view", .kind = SettingKind::Number, .default_value = DataValue::Number(90.0), .least = 60.0,
        .most = 120.0
      }));
      ASSERT_TRUE(_store.Declare({
        .name = "difficulty", .kind = SettingKind::Choice, .default_value = DataValue::Text("normal"),
        .choices = {"easy", "normal", "hard"}
      }));
      ASSERT_TRUE(_store.Declare({.name = "player_name", .kind = SettingKind::Text, .default_value = DataValue::Text("Ada")}));
    }

    GameMenu Menu(PlayerSettings *player = nullptr)
    {
      return GameMenu(&_ui, &_store, player, _logger);
    }

    /// What the file of the player would hold under `game`, or null.
    const DataValue *Kept(const std::string &name)
    {
      const DataValue *found = _player.Document().Find("game");
      return found != nullptr ? found->Find(name) : nullptr;
    }

    std::string TextOf(const std::string &name)
    {
      std::string text;
      (void) _store.GetText(name, text);
      return text;
    }

    double NumberOf(const std::string &name)
    {
      double number = 0.0;
      (void) _store.GetNumber(name, number);
      return number;
    }

    bool FlagOf(const std::string &name)
    {
      bool flag = false;
      (void) _store.GetFlag(name, flag);
      return flag;
    }
  };

  TEST_F(GameMenuTest, ShowsTheSettingsAsTheyAreWhenItIsOpened)
  {
    ASSERT_TRUE(_store.Set("difficulty", DataValue::Text("hard")));

    GameMenu menu = Menu();
    menu.Open();

    EXPECT_EQ(_values["subtitles"], "true");
    EXPECT_EQ(_values["field_of_view"], "90");
    EXPECT_EQ(_values["difficulty"], "hard");
    EXPECT_EQ(_values["player_name"], "Ada");
    EXPECT_TRUE(menu.IsOpen());
  }

  TEST_F(GameMenuTest, HasAValueForEverySettingOfTheGame)
  {
    EXPECT_THAT(Menu().GetValueNames(), ElementsAre("subtitles", "field_of_view", "difficulty", "player_name"));
  }

  TEST_F(GameMenuTest, SetsWhatThePlayerChangesOnTheStoreAtOnceWhichTellsTheSubscribers)
  {
    std::vector<std::string> heard;
    const SettingsSubscription subscription = _store.OnChange("difficulty", [&](const std::string &, const DataValue &value)
    {
      std::string text;
      (void) value.GetText(text);
      heard.push_back(text);
    });

    GameMenu menu = Menu();
    menu.Open();

    _values["difficulty"] = "hard";
    _values["subtitles"] = "false";
    _values["field_of_view"] = "100";
    _values["player_name"] = "Grace";
    menu.Update();

    EXPECT_EQ(TextOf("difficulty"), "hard");
    EXPECT_FALSE(FlagOf("subtitles"));
    EXPECT_DOUBLE_EQ(NumberOf("field_of_view"), 100.0);
    EXPECT_EQ(TextOf("player_name"), "Grace");
    EXPECT_THAT(heard, ElementsAre("hard"));
  }

  TEST_F(GameMenuTest, ChangesNothingWhenNothingChanged)
  {
    int heard = 0;
    const SettingsSubscription subscription = _store.OnChange("field_of_view", [&](const std::string &, const DataValue &) { heard++; });

    GameMenu menu = Menu();
    menu.Open();
    menu.Update();

    // a slider may say 90 as 90.0, which is no change
    _values["field_of_view"] = "90.0";
    menu.Update();

    EXPECT_EQ(heard, 0);
    EXPECT_EQ(_logger->Count(LogLevel::Info), 0u) << _logger->Messages(LogLevel::Info);
  }

  TEST_F(GameMenuTest, KeepsWhatChangedWhenTheMenuIsClosedWithApply)
  {
    GameMenu menu = Menu(&_player);
    menu.Open();
    _values["difficulty"] = "easy";
    _values["field_of_view"] = "70";
    menu.Update();

    menu.Close(true);

    ASSERT_NE(Kept("difficulty"), nullptr);
    std::string difficulty;
    EXPECT_TRUE(Kept("difficulty")->GetText(difficulty));
    EXPECT_EQ(difficulty, "easy");
    ASSERT_NE(Kept("field_of_view"), nullptr);
    double field_of_view = 0.0;
    EXPECT_TRUE(Kept("field_of_view")->GetNumber(field_of_view));
    EXPECT_DOUBLE_EQ(field_of_view, 70.0);
    EXPECT_EQ(Kept("subtitles"), nullptr) << "what the player never changed stays the project's";
    EXPECT_TRUE(_files.Exists("user://settings.yml"));
    EXPECT_FALSE(menu.IsOpen());
  }

  TEST_F(GameMenuTest, TakesWhatChangedInTheFrameTheMenuIsClosed)
  {
    GameMenu menu = Menu(&_player);
    menu.Open();
    _values["subtitles"] = "false";

    menu.Close(true);

    EXPECT_FALSE(FlagOf("subtitles"));
    EXPECT_NE(Kept("subtitles"), nullptr);
  }

  TEST_F(GameMenuTest, PutsBackWhatWasNotKept)
  {
    std::vector<std::string> heard;
    const SettingsSubscription subscription = _store.OnChange("difficulty", [&](const std::string &, const DataValue &value)
    {
      std::string text;
      (void) value.GetText(text);
      heard.push_back(text);
    });

    GameMenu menu = Menu(&_player);
    menu.Open();
    _values["difficulty"] = "hard";
    _values["field_of_view"] = "110";
    menu.Update();

    menu.Close(false);

    EXPECT_EQ(TextOf("difficulty"), "normal");
    EXPECT_DOUBLE_EQ(NumberOf("field_of_view"), 90.0);
    EXPECT_THAT(heard, ElementsAre("hard", "normal")) << "the subscribers hear of the way back as well";
    EXPECT_FALSE(_files.Exists("user://settings.yml"));
  }

  TEST_F(GameMenuTest, SaysOnceWhatCannotBeTakenAndKeepsNothingOfIt)
  {
    GameMenu menu = Menu(&_player);
    menu.Open();
    _values["field_of_view"] = "200";
    _values["difficulty"] = "nightmare";
    _values["subtitles"] = "maybe";
    menu.Update();
    menu.Update();
    menu.Update();

    EXPECT_DOUBLE_EQ(NumberOf("field_of_view"), 90.0);
    EXPECT_EQ(TextOf("difficulty"), "normal");
    EXPECT_TRUE(FlagOf("subtitles"));
    EXPECT_EQ(_logger->Count(LogLevel::Warn), 3u) << _logger->Messages(LogLevel::Warn);

    menu.Close(true);
    EXPECT_EQ(Kept("field_of_view"), nullptr);
    EXPECT_EQ(Kept("difficulty"), nullptr);
    EXPECT_FALSE(_files.Exists("user://settings.yml"));
  }

  TEST_F(GameMenuTest, TakesAValueThatFitsAfterOneThatDidNot)
  {
    GameMenu menu = Menu();
    menu.Open();
    _values["field_of_view"] = "200";
    menu.Update();
    _values["field_of_view"] = "110";
    menu.Update();

    EXPECT_DOUBLE_EQ(NumberOf("field_of_view"), 110.0);
  }

  TEST_F(GameMenuTest, LeavesASettingNoElementFollowsAlone)
  {
    GameMenu menu = Menu();
    menu.Open();
    _values.erase("player_name");
    _values["difficulty"] = "easy";
    menu.Update();

    EXPECT_EQ(TextOf("player_name"), "Ada");
    EXPECT_EQ(TextOf("difficulty"), "easy");
  }

  // building the rows

  TEST_F(GameMenuTest, BuildsASectionPerCategoryWithItsGroupsAndRowsInOrder)
  {
    // declared out of order, in two categories, one with a group
    ASSERT_TRUE(_store.Declare({
      .name = "invert_look", .kind = SettingKind::Flag, .default_value = DataValue::Bool(false), .category = "Controls",
      .group = "Camera", .order = 2
    }));
    ASSERT_TRUE(_store.Declare({.name = "reset_progress", .kind = SettingKind::Action, .category = "Gameplay", .order = 1}));
    ASSERT_TRUE(_store.Declare({
      .name = "sensitivity", .kind = SettingKind::Number, .default_value = DataValue::Number(1.0), .least = 0.1, .most = 5.0,
      .category = "Controls", .group = "Camera", .order = 1, .step = 0.1
    }));
    ASSERT_TRUE(_store.Declare({
      .name = "vibration", .kind = SettingKind::Flag, .default_value = DataValue::Bool(true), .category = "Controls",
      .order = 9, .control = SettingControl::Checkbox, .label = "Rumble"
    }));

    GameMenu menu = Menu();
    menu.Open(7);

    ASSERT_EQ(_ui.made.size(), 12u) << ::testing::PrintToString(_ui.made);
    // the four settings of the fixture have no category: the section Game, first, in their order
    EXPECT_EQ(_ui.made[0], "{type: label, class: section, text: Game}");
    EXPECT_EQ(_ui.made[1], "{type: panel, class: row, name: subtitles-row, children: [{type: label, class: row-label, text: Subtitles}, {type: toggle, name: subtitles, checked: {subtitles}}]}");
    EXPECT_EQ(_ui.made[2], "{type: panel, class: row, name: field_of_view-row, children: [{type: label, class: row-label, text: Field of view}, {type: panel, class: with-value, children: [{type: slider, name: field_of_view, min: 60, max: 120, value: {field_of_view}}, {type: label, class: value, text: {field_of_view}}]}]}");
    EXPECT_EQ(_ui.made[3], "{type: panel, class: row, name: difficulty-row, children: [{type: label, class: row-label, text: Difficulty}, {type: select, name: difficulty, value: {difficulty}, options: [{value: easy, text: easy}, {value: normal, text: normal}, {value: hard, text: hard}]}]}");
    EXPECT_EQ(_ui.made[4], "{type: panel, class: row, name: player_name-row, children: [{type: label, class: row-label, text: Player name}, {type: input, name: player_name, value: {player_name}}]}");
    // then Gameplay, whose first setting has order 1, before Controls, whose first has order 2
    EXPECT_EQ(_ui.made[5], "{type: label, class: section, text: Gameplay}");
    EXPECT_EQ(_ui.made[6], "{type: panel, class: row, name: reset_progress-row, children: [{type: button, name: reset_progress, text: Reset progress}]}");
    EXPECT_EQ(_ui.made[7], "{type: label, class: section, text: Controls}");
    // the row under no heading first, then the group, its rows in order
    EXPECT_EQ(_ui.made[8], "{type: panel, class: row, name: vibration-row, children: [{type: label, class: row-label, text: Rumble}, {type: checkbox, name: vibration, checked: {vibration}}]}");
    EXPECT_EQ(_ui.made[9], "{type: label, class: group, text: Camera}");
    EXPECT_THAT(_ui.made[10], ::testing::HasSubstr("name: sensitivity-row"));
    EXPECT_THAT(_ui.made[11], ::testing::HasSubstr("name: invert_look-row"));
  }

  TEST_F(GameMenuTest, PutsTheRowsOfAGroupInOrderUnderItsHeading)
  {
    ASSERT_TRUE(_store.Declare({
      .name = "invert_look", .kind = SettingKind::Flag, .default_value = DataValue::Bool(false), .category = "Controls",
      .group = "Camera", .order = 2
    }));
    ASSERT_TRUE(_store.Declare({
      .name = "sensitivity", .kind = SettingKind::Number, .default_value = DataValue::Number(1.0), .least = 0.1, .most = 5.0,
      .category = "Controls", .group = "Camera", .order = 1, .step = 0.1
    }));
    _ui.existing = {"subtitles", "field_of_view", "difficulty", "player_name"};

    GameMenu menu = Menu();
    menu.Open(7);

    ASSERT_EQ(_ui.made.size(), 4u) << ::testing::PrintToString(_ui.made);
    EXPECT_EQ(_ui.made[0], "{type: label, class: section, text: Controls}");
    EXPECT_EQ(_ui.made[1], "{type: label, class: group, text: Camera}");
    EXPECT_EQ(_ui.made[2], "{type: panel, class: row, name: sensitivity-row, children: [{type: label, class: row-label, text: Sensitivity}, {type: panel, class: with-value, children: [{type: slider, name: sensitivity, min: 0.1, max: 5, step: 0.1, value: {sensitivity}}, {type: label, class: value, text: {sensitivity}}]}]}");
    EXPECT_EQ(_ui.made[3], "{type: panel, class: row, name: invert_look-row, children: [{type: label, class: row-label, text: Invert look}, {type: toggle, name: invert_look, checked: {invert_look}}]}");
  }

  TEST_F(GameMenuTest, LeavesARowTheRecipeWritesToTheRecipeAndStillBindsIt)
  {
    _ui.existing = {"difficulty", "player_name"};

    GameMenu menu = Menu();
    menu.Open(7);

    ASSERT_EQ(_ui.made.size(), 3u) << ::testing::PrintToString(_ui.made);
    EXPECT_THAT(_ui.made[1], ::testing::HasSubstr("name: subtitles-row"));
    EXPECT_THAT(_ui.made[2], ::testing::HasSubstr("name: field_of_view-row"));

    _values["difficulty"] = "hard";
    menu.Update();
    EXPECT_EQ(TextOf("difficulty"), "hard");
  }

  TEST_F(GameMenuTest, MakesNothingWithoutADocumentOrAList)
  {
    GameMenu menu = Menu();
    menu.Open();
    menu.Close(false);
    menu.Open(3);

    EXPECT_TRUE(_ui.made.empty());
    EXPECT_EQ(_values["difficulty"], "normal") << "the values are set all the same";
  }

  TEST_F(GameMenuTest, ANumberWithoutARangeIsTypedIntoAField)
  {
    ASSERT_TRUE(_store.Declare({.name = "seed", .kind = SettingKind::Number, .default_value = DataValue::Number(7.0)}));
    _ui.existing = {"subtitles", "field_of_view", "difficulty", "player_name"};

    GameMenu menu = Menu();
    menu.Open(7);

    ASSERT_EQ(_ui.made.size(), 2u) << ::testing::PrintToString(_ui.made);
    EXPECT_EQ(_ui.made[1], "{type: panel, class: row, name: seed-row, children: [{type: label, class: row-label, text: Seed}, {type: input, name: seed, value: {seed}, kind: number}]}");
  }

  TEST_F(GameMenuTest, AButtonTriggersItsActionWhichTellsTheSubscribers)
  {
    ASSERT_TRUE(_store.Declare({.name = "reset_progress", .kind = SettingKind::Action}));
    int heard = 0;
    const SettingsSubscription subscription = _store.OnChange("reset_progress", [&](const std::string &, const DataValue &) { heard++; });

    GameMenu menu = Menu(&_player);
    menu.Open(7);
    menu.Update();
    EXPECT_EQ(heard, 0);

    ON_CALL(_ui, WasClicked("reset_progress")).WillByDefault(::testing::Return(true));
    menu.Update();
    EXPECT_EQ(heard, 1);

    menu.Close(true);
    EXPECT_EQ(Kept("reset_progress"), nullptr) << "nothing is kept of an action";
    EXPECT_FALSE(_files.Exists("user://settings.yml"));
  }

  TEST_F(GameMenuTest, DoesNothingUntilItIsOpened)
  {
    GameMenu menu = Menu();
    _values["difficulty"] = "hard";

    menu.Update();

    EXPECT_EQ(TextOf("difficulty"), "normal");
    EXPECT_FALSE(menu.IsOpen());
  }
} // namespace
