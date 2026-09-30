#include "ui-values.hpp"

#include <string>
#include <vector>

#include <gtest/gtest.h>

// Values that fall back on others: those of one user interface on those
// every user interface shares.

namespace
{
  using neon::UiTemplate;
  using neon::UiValue;
  using neon::UiValues;

  class UiValuesFallbackTest : public ::testing::Test
  {
  protected:
    UiValues _shared;
    UiValues _own;

    void SetUp() override
    {
      _own.SetParent(&_shared);
    }
  };

  TEST_F(UiValuesFallbackTest, FindsWhatItDoesNotHaveWhereItFallsBackOn)
  {
    _shared.Set("health", UiValue::Number(75));

    const UiValue *found = _own.Find("health");

    ASSERT_NE(found, nullptr);
    EXPECT_DOUBLE_EQ(found->number, 75);
    EXPECT_TRUE(_own.Has("health"));
  }

  TEST_F(UiValuesFallbackTest, ItsOwnValueWins)
  {
    _shared.Set("health", UiValue::Number(75));
    _own.Set("health", UiValue::Number(3));

    EXPECT_DOUBLE_EQ(_own.Find("health")->number, 3);
    EXPECT_DOUBLE_EQ(_shared.Find("health")->number, 75) << "and what it falls back on stays as it is";
  }

  TEST_F(UiValuesFallbackTest, ChangesWhenWhatItFallsBackOnChanges)
  {
    const auto before = _own.GetRevision();

    _shared.Set("health", UiValue::Number(75));
    const auto after_shared = _own.GetRevision();
    EXPECT_GT(after_shared, before) << "so that what shows the value is made again";

    _own.Set("door", UiValue::Text("open"));
    EXPECT_GT(_own.GetRevision(), after_shared);

    // and what it falls back on knows nothing of it
    const auto shared = _shared.GetRevision();
    _own.Set("door", UiValue::Text("shut"));
    EXPECT_EQ(_shared.GetRevision(), shared);
  }

  TEST_F(UiValuesFallbackTest, WhatIsFoundNowhereIsMissedOnce)
  {
    EXPECT_EQ(_own.Find("missing"), nullptr);
    EXPECT_EQ(_own.Find("missing"), nullptr);
    EXPECT_FALSE(_own.Has("missing"));

    // by what is asked last, which is where the game would set it for all
    EXPECT_EQ(_shared.TakeMissed(), (std::vector<std::string>{"missing"}));
    EXPECT_TRUE(_own.TakeMissed().empty());
    EXPECT_TRUE(_shared.TakeMissed().empty());
  }

  TEST_F(UiValuesFallbackTest, AskingWhetherAValueIsThereIsNotAskingInVain)
  {
    EXPECT_FALSE(_own.Has("missing"));
    EXPECT_FALSE(_shared.Has("missing"));

    EXPECT_TRUE(_own.TakeMissed().empty());
    EXPECT_TRUE(_shared.TakeMissed().empty());
  }

  TEST_F(UiValuesFallbackTest, ATextShowsTheValueThatIsNearest)
  {
    UiTemplate text;
    std::string error;
    ASSERT_TRUE(UiTemplate::Parse("{health} of {most}", text, error)) << error;

    _shared.Set("health", UiValue::Number(75));
    _shared.Set("most", UiValue::Number(100));
    _own.Set("health", UiValue::Number(3));

    EXPECT_EQ(text.Format(_own), "3 of 100");
    EXPECT_EQ(text.Format(_shared), "75 of 100");
  }

  TEST_F(UiValuesFallbackTest, WhatAFileStartsAValueWithDoesNotReplaceItsOwn)
  {
    _own.Set("health", UiValue::Number(3));
    _own.SetDefault("health", UiValue::Number(10));
    EXPECT_DOUBLE_EQ(_own.Find("health")->number, 3);

    // what it falls back on is no value of its own
    _shared.Set("door", UiValue::Text("open"));
    _own.SetDefault("door", UiValue::Text("locked"));
    EXPECT_EQ(_own.Find("door")->text, "locked");
  }

  TEST_F(UiValuesFallbackTest, DoesNotFallBackOnItself)
  {
    _own.SetParent(&_own);

    EXPECT_EQ(_own.Find("missing"), nullptr);
    EXPECT_FALSE(_own.Has("missing"));
    (void) _own.GetRevision();
  }

  TEST_F(UiValuesFallbackTest, StandsAloneOnceItFallsBackOnNothing)
  {
    _shared.Set("health", UiValue::Number(75));
    _own.SetParent(nullptr);

    EXPECT_EQ(_own.Find("health"), nullptr);
    EXPECT_EQ(_own.TakeMissed(), (std::vector<std::string>{"health"}));
  }
}
