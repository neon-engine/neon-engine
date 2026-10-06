#include "ui-call.hpp"

#include <string>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace
{
  using neon::UiCall;
  using neon::UiCallArgument;
  using ::testing::HasSubstr;

  using Kind = UiCallArgument::Kind;

  UiCall Read(const std::string &text)
  {
    UiCall call;
    std::string problem;
    EXPECT_TRUE(UiCall::Parse(text, call, problem)) << text << ": " << problem;
    return call;
  }

  std::string ProblemOf(const std::string &text)
  {
    UiCall call;
    call.function = "left";
    std::string problem;
    EXPECT_FALSE(UiCall::Parse(text, call, problem)) << text;
    EXPECT_TRUE(call.IsEmpty()) << text;
    return problem;
  }

  TEST(UiCall, ANameAloneIsACallWithoutArguments)
  {
    EXPECT_EQ(Read("unlock").function, "unlock");
    EXPECT_TRUE(Read("unlock").arguments.empty());
    EXPECT_EQ(Read("  set_door_2 () "), Read("set_door_2"));
    EXPECT_EQ(Read("_private()").function, "_private");
  }

  TEST(UiCall, ReadsNumbersTextsFlagsValuesAndTheEvent)
  {
    const UiCall call = Read("open('safe', \"it's\", 2, -0.5, .25, true, false, door, $event)");

    EXPECT_EQ(call.function, "open");
    ASSERT_EQ(call.arguments.size(), 9u);
    EXPECT_EQ(call.arguments[0].kind, Kind::Text);
    EXPECT_EQ(call.arguments[0].text, "safe");
    EXPECT_EQ(call.arguments[1].kind, Kind::Text);
    EXPECT_EQ(call.arguments[1].text, "it's");
    EXPECT_EQ(call.arguments[2].kind, Kind::Number);
    EXPECT_DOUBLE_EQ(call.arguments[2].number, 2.0);
    EXPECT_DOUBLE_EQ(call.arguments[3].number, -0.5);
    EXPECT_DOUBLE_EQ(call.arguments[4].number, 0.25);
    EXPECT_EQ(call.arguments[5].kind, Kind::Flag);
    EXPECT_TRUE(call.arguments[5].flag);
    EXPECT_EQ(call.arguments[6].kind, Kind::Flag);
    EXPECT_FALSE(call.arguments[6].flag);
    EXPECT_EQ(call.arguments[7].kind, Kind::Value);
    EXPECT_EQ(call.arguments[7].text, "door");
    EXPECT_EQ(call.arguments[8].kind, Kind::Event);
  }

  TEST(UiCall, ATextKeepsItsSpacesCommasAndBrackets)
  {
    const UiCall call = Read("say('a, (b) ', '')");

    ASSERT_EQ(call.arguments.size(), 2u);
    EXPECT_EQ(call.arguments[0].text, "a, (b) ");
    EXPECT_EQ(call.arguments[1].kind, Kind::Text);
    EXPECT_EQ(call.arguments[1].text, "");
  }

  TEST(UiCall, IsWrittenAsItIsRead)
  {
    for (const std::string text : {"unlock", "open('safe', \"it's\", 2, -0.5, true, false, door, $event)"})
    {
      EXPECT_EQ(Read(text).AsText(), text);
      EXPECT_EQ(Read(Read(text).AsText()), Read(text));
    }

    EXPECT_EQ(Read("unlock()").AsText(), "unlock");
    EXPECT_EQ(UiCall{}.AsText(), "");
  }

  TEST(UiCall, SaysWhatIsWrongWithATextThatIsNoCall)
  {
    EXPECT_THAT(ProblemOf(""), HasSubstr("name of a function"));
    EXPECT_THAT(ProblemOf("2fast()"), HasSubstr("name of a function"));
    EXPECT_THAT(ProblemOf("unlock now"), HasSubstr("( or nothing"));
    EXPECT_THAT(ProblemOf("unlock.door()"), HasSubstr("( or nothing"));
    EXPECT_THAT(ProblemOf("unlock("), HasSubstr("not closed"));
    EXPECT_THAT(ProblemOf("unlock(1"), HasSubstr("not closed"));
    EXPECT_THAT(ProblemOf("unlock(1,)"), HasSubstr("missing"));
    EXPECT_THAT(ProblemOf("unlock(1 2)"), HasSubstr(", or )"));
    EXPECT_THAT(ProblemOf("unlock('safe)"), HasSubstr("not closed with '"));
    EXPECT_THAT(ProblemOf("unlock($door)"), HasSubstr("$event"));
    EXPECT_THAT(ProblemOf("unlock(1.2.3)"), HasSubstr("not a number"));
    EXPECT_THAT(ProblemOf("unlock(-)"), HasSubstr("not a number"));
    EXPECT_THAT(ProblemOf("unlock(#)"), HasSubstr("no argument"));
    EXPECT_THAT(ProblemOf("unlock() again"), HasSubstr("after the )"));
  }
} // namespace
