#include "ui-values.hpp"

#include <limits>
#include <string>
#include <vector>

#include <gtest/gtest.h>

namespace
{
  using neon::DataValue;
  using neon::UiFlag;
  using neon::UiNumber;
  using neon::UiTemplate;
  using neon::UiValue;
  using neon::UiValues;

  std::string Format(const std::string &text, const UiValues &values)
  {
    UiTemplate parsed;
    std::string error;
    EXPECT_TRUE(UiTemplate::Parse(text, parsed, error)) << error;
    return parsed.Format(values);
  }

  std::string ErrorOf(const std::string &text)
  {
    UiTemplate parsed;
    std::string error;
    EXPECT_FALSE(UiTemplate::Parse(text, parsed, error)) << text;
    return error;
  }

  // a value

  TEST(UiValue, ShowsAWholeNumberWithoutAPoint)
  {
    EXPECT_EQ(UiValue::Number(75).AsText(), "75");
    EXPECT_EQ(UiValue::Number(0).AsText(), "0");
    EXPECT_EQ(UiValue::Number(-3).AsText(), "-3");
    EXPECT_EQ(UiValue::Number(1000000).AsText(), "1000000");
  }

  TEST(UiValue, ShowsAnyOtherNumberWithTwoDigitsBehindThePoint)
  {
    EXPECT_EQ(UiValue::Number(0.5).AsText(), "0.50");
    EXPECT_EQ(UiValue::Number(0.1 + 0.2).AsText(), "0.30");
    EXPECT_EQ(UiValue::Number(-12.345).AsText(), "-12.35");
  }

  TEST(UiValue, ShowsANumberWithTheDigitsThatWereAskedFor)
  {
    EXPECT_EQ(UiValue::Number(75).AsText(1), "75.0");
    EXPECT_EQ(UiValue::Number(74.96).AsText(1), "75.0");
    EXPECT_EQ(UiValue::Number(74.96).AsText(0), "75");
    EXPECT_EQ(UiValue::Number(3.14159).AsText(3), "3.142");
  }

  TEST(UiValue, ShowsWhatIsNoNumber)
  {
    EXPECT_EQ(UiValue::Number(std::numeric_limits<double>::quiet_NaN()).AsText(), "nan");
    EXPECT_EQ(UiValue::Number(std::numeric_limits<double>::infinity()).AsText(), "inf");
    EXPECT_EQ(UiValue::Number(-std::numeric_limits<double>::infinity()).AsText(1), "-inf");
  }

  TEST(UiValue, ShowsTextAndFlags)
  {
    EXPECT_EQ(UiValue::Text("Ada").AsText(), "Ada");
    EXPECT_EQ(UiValue::Text("Ada").AsText(2), "Ada");
    EXPECT_EQ(UiValue::Flag(true).AsText(), "true");
    EXPECT_EQ(UiValue::Flag(false).AsText(), "false");
  }

  TEST(UiValue, CountsAsANumber)
  {
    EXPECT_DOUBLE_EQ(UiValue::Number(75).AsNumber(-1), 75);
    EXPECT_DOUBLE_EQ(UiValue::Text("12.5").AsNumber(-1), 12.5);
    EXPECT_DOUBLE_EQ(UiValue::Text("many").AsNumber(-1), -1);
    EXPECT_DOUBLE_EQ(UiValue::Flag(true).AsNumber(-1), 1);
    EXPECT_DOUBLE_EQ(UiValue::Flag(false).AsNumber(-1), 0);
  }

  TEST(UiValue, CountsAsAFlag)
  {
    EXPECT_TRUE(UiValue::Flag(true).AsFlag());
    EXPECT_FALSE(UiValue::Flag(false).AsFlag());
    EXPECT_TRUE(UiValue::Number(0.1).AsFlag());
    EXPECT_FALSE(UiValue::Number(0).AsFlag());
    EXPECT_TRUE(UiValue::Text("no").AsFlag());
    EXPECT_FALSE(UiValue::Text("").AsFlag());
  }

  // the values

  TEST(UiValues, KeepsWhatIsSet)
  {
    UiValues values;
    values.Set("health", UiValue::Number(75));
    values.Set("name", UiValue::Text("Ada"));

    ASSERT_NE(values.Find("health"), nullptr);
    EXPECT_EQ(*values.Find("health"), UiValue::Number(75));
    EXPECT_EQ(*values.Find("name"), UiValue::Text("Ada"));
    EXPECT_EQ(values.Find("armor"), nullptr);
  }

  TEST(UiValues, AValueCanChangeItsKind)
  {
    UiValues values;
    values.Set("health", UiValue::Number(75));
    values.Set("health", UiValue::Text("full"));

    EXPECT_EQ(*values.Find("health"), UiValue::Text("full"));
  }

  TEST(UiValues, TheRevisionGoesUpWhenAValueChanges)
  {
    UiValues values;
    const auto at_the_start = values.GetRevision();

    values.Set("health", UiValue::Number(75));
    const auto after_the_first = values.GetRevision();
    EXPECT_GT(after_the_first, at_the_start);

    values.Set("health", UiValue::Number(74));
    EXPECT_GT(values.GetRevision(), after_the_first);
  }

  TEST(UiValues, TheRevisionStaysWhenAValueIsSetToWhatItHolds)
  {
    UiValues values;
    values.Set("health", UiValue::Number(75));
    const auto revision = values.GetRevision();

    values.Set("health", UiValue::Number(75));

    EXPECT_EQ(values.GetRevision(), revision);
  }

  TEST(UiValues, ADefaultDoesNotReplaceWhatIsSet)
  {
    UiValues values;
    values.Set("health", UiValue::Number(20));

    values.SetDefault("health", UiValue::Number(100));
    values.SetDefault("armor", UiValue::Number(50));

    EXPECT_EQ(*values.Find("health"), UiValue::Number(20));
    EXPECT_EQ(*values.Find("armor"), UiValue::Number(50));
  }

  TEST(UiValues, HandsOutANameThatWasAskedForInVainOnce)
  {
    UiValues values;
    values.Set("health", UiValue::Number(75));

    (void) values.Find("helth");
    (void) values.Find("helth");
    (void) values.Find("health");
    (void) values.Find("armor");

    EXPECT_EQ(values.TakeMissed(), (std::vector<std::string>{"helth", "armor"}));

    (void) values.Find("helth");
    EXPECT_TRUE(values.TakeMissed().empty());
  }

  // a text with values

  TEST(UiTemplate, LeavesPlainTextAsItIs)
  {
    const UiValues values;

    EXPECT_EQ(Format("Start", values), "Start");
    EXPECT_EQ(Format("", values), "");
    EXPECT_EQ(Format("50% of 100", values), "50% of 100");
  }

  TEST(UiTemplate, PutsValuesInTheirPlaces)
  {
    UiValues values;
    values.Set("health", UiValue::Number(75));
    values.Set("name", UiValue::Text("Ada"));
    values.Set("armed", UiValue::Flag(true));

    EXPECT_EQ(Format("Health: {health}", values), "Health: 75");
    EXPECT_EQ(Format("{name} has {health} of 100", values), "Ada has 75 of 100");
    EXPECT_EQ(Format("{health}{health}", values), "7575");
    EXPECT_EQ(Format("armed: {armed}", values), "armed: true");
  }

  TEST(UiTemplate, ShowsWhatTheValueHoldsNow)
  {
    UiValues values;
    values.Set("health", UiValue::Number(75));

    UiTemplate parsed;
    std::string error;
    ASSERT_TRUE(UiTemplate::Parse("Health: {health}", parsed, error));

    EXPECT_EQ(parsed.Format(values), "Health: 75");

    values.Set("health", UiValue::Number(40));
    EXPECT_EQ(parsed.Format(values), "Health: 40");
  }

  TEST(UiTemplate, WritesANumberWithTheDigitsThatWereAskedFor)
  {
    UiValues values;
    values.Set("speed", UiValue::Number(12.3456));

    EXPECT_EQ(Format("{speed}", values), "12.35");
    EXPECT_EQ(Format("{speed:0}", values), "12");
    EXPECT_EQ(Format("{speed:1} m/s", values), "12.3 m/s");
    EXPECT_EQ(Format("{speed:3}", values), "12.346");
  }

  TEST(UiTemplate, ShowsANameWithoutAValueAsItIsWritten)
  {
    UiValues values;
    values.Set("health", UiValue::Number(75));

    EXPECT_EQ(Format("Health: {helth}", values), "Health: {helth}");
    EXPECT_EQ(values.TakeMissed(), (std::vector<std::string>{"helth"}));
  }

  TEST(UiTemplate, DoubledBracketsStandForTheBracketsThemselves)
  {
    UiValues values;
    values.Set("health", UiValue::Number(75));

    EXPECT_EQ(Format("{{health}}", values), "{health}");
    EXPECT_EQ(Format("{{{health}}}", values), "{75}");
    EXPECT_EQ(Format("a }} b {{ c", values), "a } b { c");
  }

  TEST(UiTemplate, AcceptsTheNamesAGameIsLikelyToUse)
  {
    UiValues values;
    values.Set("player.health", UiValue::Number(1));
    values.Set("player-2_score", UiValue::Number(2));
    values.Set("_x", UiValue::Number(3));

    EXPECT_EQ(Format("{player.health} {player-2_score} {_x}", values), "1 2 3");
  }

  TEST(UiTemplate, KnowsWhetherItChangesWithTheValues)
  {
    UiTemplate plain;
    UiTemplate bound;
    UiTemplate escaped;
    std::string error;

    ASSERT_TRUE(UiTemplate::Parse("Start", plain, error));
    ASSERT_TRUE(UiTemplate::Parse("Health: {health}", bound, error));
    ASSERT_TRUE(UiTemplate::Parse("{{health}}", escaped, error));

    EXPECT_FALSE(plain.HasValues());
    EXPECT_TRUE(bound.HasValues());
    EXPECT_FALSE(escaped.HasValues());
  }

  TEST(UiTemplate, RefusesABracketThatIsLeftOpen)
  {
    EXPECT_EQ(ErrorOf("Health: {health"), "a '{' is never closed. Write '{{' for the bracket itself");
    EXPECT_EQ(ErrorOf("Health: health}"), "a '}' has no '{' in front of it. Write '}}' for the bracket itself");
  }

  TEST(UiTemplate, RefusesWhatIsNoName)
  {
    const std::string expected_end = "is not the name of a value. A name is made of letters, digits, '_', '-', and '.'";

    EXPECT_EQ(ErrorOf("{}"), "'{}' " + expected_end);
    EXPECT_EQ(ErrorOf("{two words}"), "'{two words}' " + expected_end);
    EXPECT_EQ(ErrorOf("{1up}"), "'{1up}' " + expected_end);
    EXPECT_EQ(ErrorOf("{a{b}"), "'{a{b}' " + expected_end);
  }

  TEST(UiTemplate, RefusesDigitsThatAreNoNumber)
  {
    EXPECT_EQ(
      ErrorOf("{speed:two}"),
      "'{speed:two}' asks for 'two' digits behind the point, where a number from 0 to 9 was expected");
    EXPECT_EQ(
      ErrorOf("{speed:12}"),
      "'{speed:12}' asks for '12' digits behind the point, where a number from 0 to 9 was expected");
    EXPECT_EQ(
      ErrorOf("{speed:}"),
      "'{speed:}' asks for '' digits behind the point, where a number from 0 to 9 was expected");
  }

  // a number and a flag that may follow a value

  TEST(UiNumber, IsTheNumberThatIsWritten)
  {
    UiNumber number;
    ASSERT_TRUE(UiNumber::Read(DataValue::Number(75), number));

    EXPECT_DOUBLE_EQ(number.Get(UiValues{}, -1), 75);
  }

  TEST(UiNumber, FollowsAValue)
  {
    UiValues values;
    values.Set("health", UiValue::Number(75));

    UiNumber number;
    ASSERT_TRUE(UiNumber::Read(DataValue::Text("{health}"), number));
    EXPECT_DOUBLE_EQ(number.Get(values, -1), 75);

    values.Set("health", UiValue::Number(40));
    EXPECT_DOUBLE_EQ(number.Get(values, -1), 40);
  }

  TEST(UiNumber, CountsAsWhatItIsToldWithoutAValue)
  {
    UiValues values;
    values.Set("name", UiValue::Text("Ada"));

    UiNumber missing;
    UiNumber no_number;
    ASSERT_TRUE(UiNumber::Read(DataValue::Text("{health}"), missing));
    ASSERT_TRUE(UiNumber::Read(DataValue::Text("{name}"), no_number));

    EXPECT_DOUBLE_EQ(missing.Get(values, -1), -1);
    EXPECT_DOUBLE_EQ(no_number.Get(values, -1), -1);
  }

  TEST(UiNumber, RefusesWhatIsNeither)
  {
    UiNumber number(7);

    EXPECT_FALSE(UiNumber::Read(DataValue::Text("health"), number));
    EXPECT_FALSE(UiNumber::Read(DataValue::Text("{health} of 100"), number));
    EXPECT_FALSE(UiNumber::Read(DataValue::Text("{!health}"), number));
    EXPECT_FALSE(UiNumber::Read(DataValue::Text("{}"), number));
    EXPECT_FALSE(UiNumber::Read(DataValue::Bool(true), number));
    EXPECT_FALSE(UiNumber::Read(DataValue::List(), number));

    EXPECT_DOUBLE_EQ(number.Get(UiValues{}, -1), 7);
  }

  TEST(UiFlag, IsTheFlagThatIsWritten)
  {
    UiFlag yes;
    UiFlag no;
    ASSERT_TRUE(UiFlag::Read(DataValue::Bool(true), yes));
    ASSERT_TRUE(UiFlag::Read(DataValue::Bool(false), no));

    EXPECT_TRUE(yes.Get(UiValues{}, false));
    EXPECT_FALSE(no.Get(UiValues{}, true));
  }

  TEST(UiFlag, FollowsAValueOrItsOpposite)
  {
    UiValues values;
    values.Set("paused", UiValue::Flag(true));

    UiFlag paused;
    UiFlag running;
    ASSERT_TRUE(UiFlag::Read(DataValue::Text("{paused}"), paused));
    ASSERT_TRUE(UiFlag::Read(DataValue::Text("{!paused}"), running));

    EXPECT_TRUE(paused.Get(values, false));
    EXPECT_FALSE(running.Get(values, true));

    values.Set("paused", UiValue::Flag(false));

    EXPECT_FALSE(paused.Get(values, true));
    EXPECT_TRUE(running.Get(values, false));
  }

  TEST(UiFlag, FollowsANumber)
  {
    UiValues values;
    values.Set("ammo", UiValue::Number(0));

    UiFlag has_ammo;
    ASSERT_TRUE(UiFlag::Read(DataValue::Text("{ammo}"), has_ammo));
    EXPECT_FALSE(has_ammo.Get(values, true));

    values.Set("ammo", UiValue::Number(3));
    EXPECT_TRUE(has_ammo.Get(values, false));
  }

  TEST(UiFlag, CountsAsWhatItIsToldWithoutAValue)
  {
    UiFlag flag;
    UiFlag opposite;
    ASSERT_TRUE(UiFlag::Read(DataValue::Text("{paused}"), flag));
    ASSERT_TRUE(UiFlag::Read(DataValue::Text("{!paused}"), opposite));

    EXPECT_TRUE(flag.Get(UiValues{}, true));
    EXPECT_FALSE(flag.Get(UiValues{}, false));
    EXPECT_TRUE(opposite.Get(UiValues{}, true));
    EXPECT_FALSE(opposite.Get(UiValues{}, false));
  }

  TEST(UiFlag, RefusesWhatIsNeither)
  {
    UiFlag flag(true);

    EXPECT_FALSE(UiFlag::Read(DataValue::Text("yes"), flag));
    EXPECT_FALSE(UiFlag::Read(DataValue::Text("{two words}"), flag));
    EXPECT_FALSE(UiFlag::Read(DataValue::Number(1), flag));

    EXPECT_TRUE(flag.Get(UiValues{}, false));
  }
}
