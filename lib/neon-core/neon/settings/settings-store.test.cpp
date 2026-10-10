#include "settings-store.hpp"

#include <memory>
#include <string>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <neon/testing/recording-logger.hpp>

namespace
{
  using neon::DataValue;
  using neon::SettingDeclaration;
  using neon::SettingKind;
  using neon::SettingsStore;
  using neon::SettingsSubscription;
  using neon::testing::LogLevel;
  using neon::testing::RecordingLogger;
  using ::testing::ElementsAre;
  using ::testing::HasSubstr;

  SettingDeclaration Flag(const std::string &name, const bool fallback)
  {
    return {.name = name, .kind = SettingKind::Flag, .default_value = DataValue::Bool(fallback)};
  }

  SettingDeclaration Number(const std::string &name, const double fallback, const double least, const double most)
  {
    return {
      .name = name, .kind = SettingKind::Number, .default_value = DataValue::Number(fallback), .least = least,
      .most = most
    };
  }

  SettingDeclaration Choice(const std::string &name, const std::string &fallback, const std::vector<std::string> &choices)
  {
    return {.name = name, .kind = SettingKind::Choice, .default_value = DataValue::Text(fallback), .choices = choices};
  }

  class SettingsStoreTest : public ::testing::Test
  {
  protected:
    std::shared_ptr<RecordingLogger> _logger = std::make_shared<RecordingLogger>();
    SettingsStore _store{_logger};

    void SetUp() override
    {
      ASSERT_TRUE(_store.Declare(Flag("subtitles", true)));
      ASSERT_TRUE(_store.Declare(Number("field_of_view", 90.0, 60.0, 120.0)));
      ASSERT_TRUE(_store.Declare(Choice("difficulty", "normal", {"easy", "normal", "hard"})));
      ASSERT_TRUE(_store.Declare({.name = "player_name", .kind = SettingKind::Text, .default_value = DataValue::Text("Ada")}));
    }

    std::string Warnings() const
    {
      return _logger->Messages(LogLevel::Warn);
    }
  };

  TEST_F(SettingsStoreTest, HoldsTheDefaultsOnceDeclared)
  {
    EXPECT_THAT(_store.Names(), ElementsAre("subtitles", "field_of_view", "difficulty", "player_name"));
    EXPECT_TRUE(_store.Has("subtitles"));
    EXPECT_FALSE(_store.Has("volume"));

    bool subtitles = false;
    double field_of_view = 0.0;
    std::string difficulty;
    std::string player_name;
    EXPECT_TRUE(_store.GetFlag("subtitles", subtitles));
    EXPECT_TRUE(_store.GetNumber("field_of_view", field_of_view));
    EXPECT_TRUE(_store.GetText("difficulty", difficulty));
    EXPECT_TRUE(_store.GetText("player_name", player_name));
    EXPECT_TRUE(subtitles);
    EXPECT_DOUBLE_EQ(field_of_view, 90.0);
    EXPECT_EQ(difficulty, "normal");
    EXPECT_EQ(player_name, "Ada");

    EXPECT_FALSE(_store.GetNumber("subtitles", field_of_view)) << "a flag is not a number";
    EXPECT_EQ(_store.Get("volume"), nullptr);
    ASSERT_NE(_store.Find("difficulty"), nullptr);
    EXPECT_EQ(_store.Find("difficulty")->kind, SettingKind::Choice);
  }

  TEST_F(SettingsStoreTest, StartsWithTheValueADeclarationBringsOverItsDefault)
  {
    SettingDeclaration declaration = Number("volume", 1.0, 0.0, 1.0);
    declaration.value = DataValue::Number(0.5);
    ASSERT_TRUE(_store.Declare(declaration));

    double volume = 0.0;
    EXPECT_TRUE(_store.GetNumber("volume", volume));
    EXPECT_DOUBLE_EQ(volume, 0.5);
    EXPECT_DOUBLE_EQ(_store.Find("volume")->default_value.GetNumber(volume) ? volume : 0.0, 1.0) << "the default stays";
  }

  TEST_F(SettingsStoreTest, SetsAndTellsTheSubscribersAfterTheValueIsStored)
  {
    std::vector<std::string> heard;
    const SettingsSubscription subscription = _store.OnChange("difficulty", [&](const std::string &name, const DataValue &value)
    {
      std::string now;
      (void) _store.GetText(name, now);
      std::string given;
      (void) value.GetText(given);
      heard.push_back(name + " " + given + " stored as " + now);
    });

    EXPECT_TRUE(_store.Set("difficulty", DataValue::Text("hard")));
    EXPECT_TRUE(_store.Set("subtitles", DataValue::Bool(false))) << "another name, which this subscriber does not hear";

    EXPECT_THAT(heard, ElementsAre("difficulty hard stored as hard"));
    EXPECT_TRUE(subscription.IsActive());
  }

  TEST_F(SettingsStoreTest, TellsNobodyOfAValueThatIsSetToWhatItIs)
  {
    int heard = 0;
    const SettingsSubscription subscription = _store.OnChange("field_of_view", [&](const std::string &, const DataValue &) { heard++; });

    EXPECT_TRUE(_store.Set("field_of_view", DataValue::Number(90.0)));
    EXPECT_TRUE(_store.Set("field_of_view", DataValue::Number(90)));
    EXPECT_EQ(heard, 0);

    EXPECT_TRUE(_store.Set("field_of_view", DataValue::Number(100)));
    EXPECT_EQ(heard, 1);
  }

  TEST_F(SettingsStoreTest, RefusesWhatDoesNotFitAndSaysSoOncePerName)
  {
    int heard = 0;
    const SettingsSubscription subscription = _store.OnChange("field_of_view", [&](const std::string &, const DataValue &) { heard++; });

    EXPECT_FALSE(_store.Set("field_of_view", DataValue::Number(200.0))) << "above the most";
    EXPECT_FALSE(_store.Set("field_of_view", DataValue::Number(10.0))) << "below the least";
    EXPECT_FALSE(_store.Set("field_of_view", DataValue::Text("wide"))) << "not a number";
    EXPECT_FALSE(_store.Set("subtitles", DataValue::Number(1.0))) << "not a flag";
    EXPECT_FALSE(_store.Set("difficulty", DataValue::Text("nightmare"))) << "none of the choices";
    EXPECT_FALSE(_store.Set("volume", DataValue::Number(0.5))) << "not declared";

    double field_of_view = 0.0;
    EXPECT_TRUE(_store.GetNumber("field_of_view", field_of_view));
    EXPECT_DOUBLE_EQ(field_of_view, 90.0);
    EXPECT_EQ(heard, 0);

    // one warning for each name, not one for each refusal
    EXPECT_EQ(_logger->Count(LogLevel::Warn), 4u) << Warnings();
    EXPECT_THAT(Warnings(), HasSubstr("above the most"));
    EXPECT_THAT(Warnings(), HasSubstr("easy, normal, hard"));
    EXPECT_THAT(Warnings(), HasSubstr("not declared"));
  }

  TEST_F(SettingsStoreTest, TakesAWholeNumberAsANumber)
  {
    EXPECT_TRUE(_store.Set("field_of_view", DataValue::Number(100)));
    double field_of_view = 0.0;
    EXPECT_TRUE(_store.GetNumber("field_of_view", field_of_view));
    EXPECT_DOUBLE_EQ(field_of_view, 100.0);
  }

  TEST_F(SettingsStoreTest, HearsNothingOnceTheSubscriptionIsGone)
  {
    int heard = 0;
    {
      const SettingsSubscription subscription = _store.OnChange("subtitles", [&](const std::string &, const DataValue &) { heard++; });
      EXPECT_TRUE(_store.Set("subtitles", DataValue::Bool(false)));
    }
    EXPECT_TRUE(_store.Set("subtitles", DataValue::Bool(true)));
    EXPECT_EQ(heard, 1);

    SettingsSubscription ended = _store.OnChange("subtitles", [&](const std::string &, const DataValue &) { heard++; });
    ended.End();
    EXPECT_FALSE(ended.IsActive());
    EXPECT_TRUE(_store.Set("subtitles", DataValue::Bool(false)));
    EXPECT_EQ(heard, 1);
  }

  TEST_F(SettingsStoreTest, ASubscriptionMayEndItselfWhileItIsTold)
  {
    int heard = 0;
    SettingsSubscription subscription;
    subscription = _store.OnChange("subtitles", [&](const std::string &, const DataValue &)
    {
      heard++;
      subscription.End();
    });
    int others = 0;
    const SettingsSubscription other = _store.OnChange("subtitles", [&](const std::string &, const DataValue &) { others++; });

    EXPECT_TRUE(_store.Set("subtitles", DataValue::Bool(false)));
    EXPECT_TRUE(_store.Set("subtitles", DataValue::Bool(true)));
    EXPECT_EQ(heard, 1);
    EXPECT_EQ(others, 2);
  }

  TEST_F(SettingsStoreTest, ASubscriptionOutlivingTheStoreEndsQuietly)
  {
    SettingsSubscription subscription;
    {
      SettingsStore store{_logger};
      ASSERT_TRUE(store.Declare(Flag("subtitles", true)));
      subscription = store.OnChange("subtitles", [](const std::string &, const DataValue &) {});
      EXPECT_TRUE(subscription.IsActive());
    }
    EXPECT_FALSE(subscription.IsActive());
    subscription.End();
  }

  TEST_F(SettingsStoreTest, RefusesADeclarationThatIsWrong)
  {
    EXPECT_FALSE(_store.Declare(Flag("subtitles", false))) << "declared already";
    EXPECT_FALSE(_store.Declare(Flag("sub-titles", false))) << "not a name";
    EXPECT_FALSE(_store.Declare(Flag("", false))) << "not a name";
    EXPECT_FALSE(_store.Declare(Choice("mode", "a", {}))) << "no choices";
    EXPECT_FALSE(_store.Declare(Choice("mode", "d", {"a", "b"}))) << "a default that is none of the choices";
    EXPECT_FALSE(_store.Declare(Number("fov", 200.0, 0.0, 100.0))) << "a default outside the range";
    EXPECT_EQ(_logger->Count(LogLevel::Error), 6u) << _logger->Messages(LogLevel::Error);
    EXPECT_EQ(_store.Names().size(), 4u);
  }

  TEST_F(SettingsStoreTest, AnActionIsTriggeredNotSetAndItsSubscribersAreToldWithNoValue)
  {
    ASSERT_TRUE(_store.Declare({.name = "reset_progress", .kind = SettingKind::Action}));
    EXPECT_FALSE(_store.Declare({.name = "quit", .kind = SettingKind::Action, .default_value = DataValue::Bool(true)}))
      << "an action has no default";

    int heard = 0;
    bool empty = false;
    const SettingsSubscription subscription = _store.OnChange("reset_progress", [&](const std::string &, const DataValue &value)
    {
      heard++;
      empty = value.IsEmpty();
    });

    EXPECT_TRUE(_store.Trigger("reset_progress"));
    EXPECT_TRUE(_store.Trigger("reset_progress")) << "every press is told, there is no same value";
    EXPECT_EQ(heard, 2);
    EXPECT_TRUE(empty);

    EXPECT_FALSE(_store.Set("reset_progress", DataValue::Bool(true)));
    EXPECT_FALSE(_store.Trigger("subtitles")) << "a flag is set, not triggered";
    EXPECT_FALSE(_store.Trigger("volume")) << "not declared";
    EXPECT_EQ(heard, 2);
    ASSERT_NE(_store.Get("reset_progress"), nullptr);
    EXPECT_TRUE(_store.Get("reset_progress")->IsEmpty());
    EXPECT_THAT(Warnings(), HasSubstr("triggered rather than set"));
  }

  TEST_F(SettingsStoreTest, SubscribingToANameThatIsNotDeclaredHearsNothing)
  {
    const SettingsSubscription subscription = _store.OnChange("volume", [](const std::string &, const DataValue &) {});
    EXPECT_FALSE(subscription.IsActive());
    EXPECT_THAT(Warnings(), HasSubstr("subtitles, field_of_view, difficulty, player_name"));
  }
} // namespace
