#include "command-line.hpp"

#include <initializer_list>
#include <string>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace
{
  using neon::CommandLine;
  using ::testing::HasSubstr;

  /// Parses the arguments as if they followed the name of the program.
  bool Parse(CommandLine &command_line, const std::initializer_list<const char *> arguments)
  {
    std::vector<const char *> argv{"program"};
    argv.insert(argv.end(), arguments.begin(), arguments.end());
    return command_line.Parse(static_cast<int>(argv.size()), argv.data());
  }

  /// A command line with one option of every kind.
  class CommandLineTest : public ::testing::Test
  {
  protected:
    CommandLine _command_line{"program", "Does things."};

    void SetUp() override
    {
      _command_line.Add({.name = "path", .value_name = "PATH", .description = "A value"});
      _command_line.Add({.name = "verbose", .description = "A switch"});
      _command_line.Add({
        .name = "mode",
        .value_name = "NAME",
        .description = "One of a few",
        .allowed_values = {"fast", "slow"},
        .default_value = "fast"
      });
      _command_line.Add({.name = "count", .value_name = "N", .description = "A number", .default_value = "3"});
    }
  };

  TEST_F(CommandLineTest, AcceptsNoArguments)
  {
    EXPECT_TRUE(Parse(_command_line, {}));
    EXPECT_EQ(_command_line.GetError(), "");
    EXPECT_FALSE(_command_line.IsSet("path"));
    EXPECT_FALSE(_command_line.WantsHelp());
  }

  TEST_F(CommandLineTest, AcceptsAnEmptyArgumentList)
  {
    EXPECT_TRUE(_command_line.Parse(0, nullptr));
  }

  TEST_F(CommandLineTest, IgnoresTheNameOfTheProgram)
  {
    const char *argv[] = {"--unknown"};

    EXPECT_TRUE(_command_line.Parse(1, argv));
  }

  TEST_F(CommandLineTest, ReadsAValueThatFollowsTheOption)
  {
    ASSERT_TRUE(Parse(_command_line, {"--path", "assets://cube.obj"}));

    EXPECT_TRUE(_command_line.IsSet("path"));
    EXPECT_EQ(_command_line.GetValue("path"), "assets://cube.obj");
  }

  TEST_F(CommandLineTest, ReadsAValueThatIsJoinedWithAnEqualsSign)
  {
    ASSERT_TRUE(Parse(_command_line, {"--path=assets://cube.obj"}));

    EXPECT_TRUE(_command_line.IsSet("path"));
    EXPECT_EQ(_command_line.GetValue("path"), "assets://cube.obj");
  }

  TEST_F(CommandLineTest, KeepsEqualsSignsThatArePartOfTheValue)
  {
    ASSERT_TRUE(Parse(_command_line, {"--path=a=b=c"}));

    EXPECT_EQ(_command_line.GetValue("path"), "a=b=c");
  }

  TEST_F(CommandLineTest, KeepsAnEqualsSignInAValueThatFollows)
  {
    ASSERT_TRUE(Parse(_command_line, {"--path", "a=b"}));

    EXPECT_EQ(_command_line.GetValue("path"), "a=b");
  }

  TEST_F(CommandLineTest, ReadsAValueThatStartsWithOneDash)
  {
    ASSERT_TRUE(Parse(_command_line, {"--count", "-5"}));

    EXPECT_EQ(_command_line.GetValue("count"), "-5");
  }

  TEST_F(CommandLineTest, ReadsAJoinedValueThatStartsWithTwoDashes)
  {
    ASSERT_TRUE(Parse(_command_line, {"--path=--verbose"}));

    EXPECT_EQ(_command_line.GetValue("path"), "--verbose");
    EXPECT_FALSE(_command_line.IsSet("verbose"));
  }

  TEST_F(CommandLineTest, ReadsASwitch)
  {
    ASSERT_TRUE(Parse(_command_line, {"--verbose"}));

    EXPECT_TRUE(_command_line.IsSet("verbose"));
    EXPECT_EQ(_command_line.GetValue("verbose"), "");
  }

  TEST_F(CommandLineTest, ReadsSeveralOptions)
  {
    ASSERT_TRUE(Parse(_command_line, {"--verbose", "--path", "here", "--mode=slow", "--count", "7"}));

    EXPECT_TRUE(_command_line.IsSet("verbose"));
    EXPECT_EQ(_command_line.GetValue("path"), "here");
    EXPECT_EQ(_command_line.GetValue("mode"), "slow");
    EXPECT_EQ(_command_line.GetValue("count"), "7");
  }

  TEST_F(CommandLineTest, DoesNotTakeWhatFollowsASwitchAsItsValue)
  {
    EXPECT_FALSE(Parse(_command_line, {"--verbose", "value"}));
    EXPECT_EQ(_command_line.GetError(), "'value' is not an option. Options start with two dashes");
  }

  TEST_F(CommandLineTest, RejectsASwitchThatIsGivenAValue)
  {
    EXPECT_FALSE(Parse(_command_line, {"--verbose=yes"}));
    EXPECT_EQ(_command_line.GetError(), "Option '--verbose' takes no value");
  }

  TEST_F(CommandLineTest, RejectsAnOptionItDoesNotKnow)
  {
    EXPECT_FALSE(Parse(_command_line, {"--unknown"}));
    EXPECT_EQ(_command_line.GetError(), "Unknown option '--unknown'");
  }

  TEST_F(CommandLineTest, RejectsAnOptionItDoesNotKnowThatCarriesAValue)
  {
    EXPECT_FALSE(Parse(_command_line, {"--unknown=value"}));
    EXPECT_EQ(_command_line.GetError(), "Unknown option '--unknown'");
  }

  TEST_F(CommandLineTest, TellsOptionsApartByLetterCase)
  {
    EXPECT_FALSE(Parse(_command_line, {"--Verbose"}));
    EXPECT_EQ(_command_line.GetError(), "Unknown option '--Verbose'");
  }

  TEST_F(CommandLineTest, RejectsABareArgument)
  {
    EXPECT_FALSE(Parse(_command_line, {"scene.yml"}));
    EXPECT_EQ(_command_line.GetError(), "'scene.yml' is not an option. Options start with two dashes");
  }

  TEST_F(CommandLineTest, RejectsAnOptionWithOneDash)
  {
    EXPECT_FALSE(Parse(_command_line, {"-verbose"}));
    EXPECT_EQ(_command_line.GetError(), "'-verbose' is not an option. Options start with two dashes");
  }

  TEST_F(CommandLineTest, RejectsTwoDashesWithoutAName)
  {
    EXPECT_FALSE(Parse(_command_line, {"--"}));
    EXPECT_EQ(_command_line.GetError(), "'--' is not an option. Options start with two dashes");
  }

  TEST_F(CommandLineTest, RejectsAnEmptyArgument)
  {
    EXPECT_FALSE(Parse(_command_line, {""}));
    EXPECT_EQ(_command_line.GetError(), "'' is not an option. Options start with two dashes");
  }

  TEST_F(CommandLineTest, RejectsAnOptionWhoseValueIsMissingAtTheEnd)
  {
    EXPECT_FALSE(Parse(_command_line, {"--path"}));
    EXPECT_EQ(_command_line.GetError(), "Option '--path' needs a value");
  }

  TEST_F(CommandLineTest, RejectsAnOptionThatIsFollowedByAnOptionInsteadOfAValue)
  {
    EXPECT_FALSE(Parse(_command_line, {"--path", "--verbose"}));
    EXPECT_EQ(_command_line.GetError(), "Option '--path' needs a value");
  }

  TEST_F(CommandLineTest, RejectsAJoinedValueThatIsEmpty)
  {
    EXPECT_FALSE(Parse(_command_line, {"--path="}));
    EXPECT_EQ(_command_line.GetError(), "Option '--path' needs a value");
  }

  TEST_F(CommandLineTest, RejectsAValueThatFollowsAndIsEmpty)
  {
    EXPECT_FALSE(Parse(_command_line, {"--path", ""}));
    EXPECT_EQ(_command_line.GetError(), "Option '--path' needs a value");
  }

  TEST_F(CommandLineTest, RejectsAnOptionThatIsGivenTwice)
  {
    EXPECT_FALSE(Parse(_command_line, {"--path", "one", "--path=two"}));
    EXPECT_EQ(_command_line.GetError(), "Option '--path' was given more than once");
  }

  TEST_F(CommandLineTest, RejectsASwitchThatIsGivenTwice)
  {
    EXPECT_FALSE(Parse(_command_line, {"--verbose", "--verbose"}));
    EXPECT_EQ(_command_line.GetError(), "Option '--verbose' was given more than once");
  }

  TEST_F(CommandLineTest, AcceptsEachAllowedValue)
  {
    ASSERT_TRUE(Parse(_command_line, {"--mode", "fast"}));
    EXPECT_EQ(_command_line.GetValue("mode"), "fast");

    ASSERT_TRUE(Parse(_command_line, {"--mode=slow"}));
    EXPECT_EQ(_command_line.GetValue("mode"), "slow");
  }

  TEST_F(CommandLineTest, RejectsAValueThatIsNotAllowed)
  {
    EXPECT_FALSE(Parse(_command_line, {"--mode", "medium"}));
    EXPECT_EQ(_command_line.GetError(), "'medium' is not a value of '--mode'. It accepts fast, slow");
  }

  TEST_F(CommandLineTest, TellsAllowedValuesApartByLetterCase)
  {
    EXPECT_FALSE(Parse(_command_line, {"--mode=Fast"}));
    EXPECT_EQ(_command_line.GetError(), "'Fast' is not a value of '--mode'. It accepts fast, slow");
  }

  TEST_F(CommandLineTest, GivesTheDefaultOfAnOptionThatWasNotGiven)
  {
    ASSERT_TRUE(Parse(_command_line, {}));

    EXPECT_FALSE(_command_line.IsSet("mode"));
    EXPECT_EQ(_command_line.GetValue("mode"), "fast");
  }

  TEST_F(CommandLineTest, GivesTheValueOverTheDefault)
  {
    ASSERT_TRUE(Parse(_command_line, {"--mode", "slow"}));

    EXPECT_TRUE(_command_line.IsSet("mode"));
    EXPECT_EQ(_command_line.GetValue("mode"), "slow");
  }

  TEST_F(CommandLineTest, GivesNothingForAnOptionWithoutDefaultThatWasNotGiven)
  {
    ASSERT_TRUE(Parse(_command_line, {}));

    EXPECT_EQ(_command_line.GetValue("path"), "");
  }

  TEST_F(CommandLineTest, GivesNothingForAnOptionThatWasNeverDeclared)
  {
    ASSERT_TRUE(Parse(_command_line, {}));

    EXPECT_FALSE(_command_line.IsSet("unknown"));
    EXPECT_EQ(_command_line.GetValue("unknown"), "");
  }

  TEST_F(CommandLineTest, GivesDefaultsBeforeAnythingWasParsed)
  {
    EXPECT_EQ(_command_line.GetValue("mode"), "fast");
    EXPECT_FALSE(_command_line.IsSet("mode"));
    EXPECT_EQ(_command_line.GetError(), "");
  }

  TEST_F(CommandLineTest, ForgetsWhatAnEarlierParseRead)
  {
    ASSERT_TRUE(Parse(_command_line, {"--path", "here", "--verbose"}));
    ASSERT_TRUE(Parse(_command_line, {"--count", "9"}));

    EXPECT_FALSE(_command_line.IsSet("path"));
    EXPECT_FALSE(_command_line.IsSet("verbose"));
    EXPECT_EQ(_command_line.GetValue("count"), "9");
  }

  TEST_F(CommandLineTest, ForgetsTheErrorOfAnEarlierParse)
  {
    ASSERT_FALSE(Parse(_command_line, {"--unknown"}));
    ASSERT_TRUE(Parse(_command_line, {"--verbose"}));

    EXPECT_EQ(_command_line.GetError(), "");
  }

  TEST_F(CommandLineTest, UnderstandsHelpWithoutItBeingDeclared)
  {
    CommandLine command_line("program", "");

    ASSERT_TRUE(Parse(command_line, {"--help"}));

    EXPECT_TRUE(command_line.WantsHelp());
  }

  TEST_F(CommandLineTest, RejectsHelpWithAValue)
  {
    EXPECT_FALSE(Parse(_command_line, {"--help=all"}));
    EXPECT_EQ(_command_line.GetError(), "Option '--help' takes no value");
  }

  TEST_F(CommandLineTest, ReplacesAnOptionThatIsDeclaredAgain)
  {
    _command_line.Add({
      .name = "mode",
      .value_name = "NAME",
      .description = "Replaced",
      .allowed_values = {"other"},
      .default_value = "other"
    });

    EXPECT_EQ(_command_line.GetValue("mode"), "other");
    EXPECT_TRUE(Parse(_command_line, {"--mode", "other"}));
    EXPECT_FALSE(Parse(_command_line, {"--mode", "fast"}));
    EXPECT_THAT(_command_line.GetHelp(), HasSubstr("Replaced"));
    EXPECT_THAT(_command_line.GetHelp(), ::testing::Not(HasSubstr("One of a few")));
  }

  TEST_F(CommandLineTest, LetsAValueOptionBecomeASwitchWhenDeclaredAgain)
  {
    _command_line.Add({.name = "path", .description = "Now a switch"});

    EXPECT_TRUE(Parse(_command_line, {"--path"}));
    EXPECT_FALSE(Parse(_command_line, {"--path=here"}));
  }

  // GetInteger

  TEST_F(CommandLineTest, ReadsAWholeNumber)
  {
    ASSERT_TRUE(Parse(_command_line, {"--count", "42"}));

    long value = 0;
    EXPECT_TRUE(_command_line.GetInteger("count", value));
    EXPECT_EQ(value, 42);
  }

  TEST_F(CommandLineTest, ReadsAWholeNumberBelowZero)
  {
    ASSERT_TRUE(Parse(_command_line, {"--count=-42"}));

    long value = 0;
    EXPECT_TRUE(_command_line.GetInteger("count", value));
    EXPECT_EQ(value, -42);
  }

  TEST_F(CommandLineTest, ReadsZeroAsAWholeNumber)
  {
    ASSERT_TRUE(Parse(_command_line, {"--count", "0"}));

    long value = 5;
    EXPECT_TRUE(_command_line.GetInteger("count", value));
    EXPECT_EQ(value, 0);
  }

  TEST_F(CommandLineTest, ReadsTheDefaultAsAWholeNumber)
  {
    ASSERT_TRUE(Parse(_command_line, {}));

    long value = 0;
    EXPECT_TRUE(_command_line.GetInteger("count", value));
    EXPECT_EQ(value, 3);
  }

  class CommandLineNotAWholeNumber : public CommandLineTest, public ::testing::WithParamInterface<const char *> {};

  TEST_P(CommandLineNotAWholeNumber, IsRefusedAndLeavesTheValueAlone)
  {
    ASSERT_TRUE(Parse(_command_line, {"--path", GetParam()}));

    long value = 17;
    EXPECT_FALSE(_command_line.GetInteger("path", value));
    EXPECT_EQ(value, 17);
  }

  INSTANTIATE_TEST_SUITE_P(
    CommandLine,
    CommandLineNotAWholeNumber,
    ::testing::Values(
      "abc",
      "12abc",
      "abc12",
      "1.5",
      "1,5",
      "+5",
      " 5",
      "5 ",
      "-",
      "0x10",
      "1e3",
      "99999999999999999999999999"));

  TEST_F(CommandLineTest, RefusesAWholeNumberFromAnOptionThatWasNotGiven)
  {
    ASSERT_TRUE(Parse(_command_line, {}));

    long value = 17;
    EXPECT_FALSE(_command_line.GetInteger("path", value));
    EXPECT_FALSE(_command_line.GetInteger("unknown", value));
    EXPECT_EQ(value, 17);
  }

  TEST_F(CommandLineTest, RefusesAWholeNumberFromASwitch)
  {
    ASSERT_TRUE(Parse(_command_line, {"--verbose"}));

    long value = 17;
    EXPECT_FALSE(_command_line.GetInteger("verbose", value));
    EXPECT_EQ(value, 17);
  }

  // GetNumber

  struct NumberCase
  {
    const char *text;
    double expected;
  };

  class CommandLineNumber : public CommandLineTest, public ::testing::WithParamInterface<NumberCase> {};

  TEST_P(CommandLineNumber, IsRead)
  {
    ASSERT_TRUE(Parse(_command_line, {"--path", GetParam().text}));

    double value = 17.0;
    EXPECT_TRUE(_command_line.GetNumber("path", value));
    EXPECT_DOUBLE_EQ(value, GetParam().expected);
  }

  INSTANTIATE_TEST_SUITE_P(
    CommandLine,
    CommandLineNumber,
    ::testing::Values(
      NumberCase{"0.25", 0.25},
      NumberCase{"0.016667", 0.016667},
      NumberCase{"5", 5.0},
      NumberCase{"0", 0.0},
      NumberCase{"0.0", 0.0},
      NumberCase{"-1.5", -1.5},
      NumberCase{"-3", -3.0},
      NumberCase{".5", 0.5},
      NumberCase{"-.5", -0.5},
      NumberCase{"5.", 5.0},
      NumberCase{"007.250", 7.25},
      NumberCase{"123456.789", 123456.789}));

  class CommandLineNotANumber : public CommandLineTest, public ::testing::WithParamInterface<const char *> {};

  TEST_P(CommandLineNotANumber, IsRefusedAndLeavesTheValueAlone)
  {
    ASSERT_TRUE(Parse(_command_line, {"--path", GetParam()}));

    double value = 17.0;
    EXPECT_FALSE(_command_line.GetNumber("path", value));
    EXPECT_EQ(value, 17.0);
  }

  INSTANTIATE_TEST_SUITE_P(
    CommandLine,
    CommandLineNotANumber,
    ::testing::Values(
      "abc",
      ".",
      "-",
      "-.",
      "1,5",
      "1.5.2",
      "1.5abc",
      "abc1.5",
      "+1.5",
      "-+1",
      "-1-",
      "1-",
      " 1.5",
      "1.5 ",
      "1e3",
      "inf",
      "nan"));

  TEST_F(CommandLineTest, RefusesANumberThatIsTooLargeToHold)
  {
    const std::string digits(400, '9');
    ASSERT_TRUE(Parse(_command_line, {"--path", digits.c_str()}));

    double value = 17.0;
    EXPECT_FALSE(_command_line.GetNumber("path", value));
    EXPECT_EQ(value, 17.0);
  }

  TEST_F(CommandLineTest, ReadsTheDefaultAsANumber)
  {
    ASSERT_TRUE(Parse(_command_line, {}));

    double value = 0.0;
    EXPECT_TRUE(_command_line.GetNumber("count", value));
    EXPECT_DOUBLE_EQ(value, 3.0);
  }

  TEST_F(CommandLineTest, RefusesANumberFromAnOptionThatWasNotGiven)
  {
    ASSERT_TRUE(Parse(_command_line, {}));

    double value = 17.0;
    EXPECT_FALSE(_command_line.GetNumber("path", value));
    EXPECT_FALSE(_command_line.GetNumber("unknown", value));
    EXPECT_EQ(value, 17.0);
  }

  // GetHelp

  TEST(CommandLine, HelpOfACommandLineWithoutOptionsShowsHelpAlone)
  {
    const CommandLine command_line("program", "Does things.");

    EXPECT_EQ(
      command_line.GetHelp(),
      "Does things.\n"
      "\n"
      "Usage: program [options]\n"
      "\n"
      "  --help  Show this text\n");
  }

  TEST(CommandLine, HelpLeavesTheSummaryOutWhenThereIsNone)
  {
    const CommandLine command_line("program", "");

    EXPECT_EQ(
      command_line.GetHelp(),
      "Usage: program [options]\n"
      "\n"
      "  --help  Show this text\n");
  }

  TEST(CommandLine, HelpListsOptionsUnderTheirHeadingsInColumns)
  {
    CommandLine command_line("tool", "Does things.");
    command_line.Add({
      .name = "frames",
      .value_name = "N",
      .description = "Stop after N frames",
      .group = "Development"
    });
    command_line.Add({
      .name = "renderer",
      .value_name = "NAME",
      .description = "Renderer to draw with",
      .allowed_values = {"vulkan", "metal"},
      .default_value = "vulkan"
    });
    command_line.Add({.name = "project", .value_name = "PATH", .description = "Project to open", .group = "Editor"});
    command_line.Add({.name = "headless", .description = "Run without a window", .group = "Development"});

    EXPECT_EQ(
      command_line.GetHelp(),
      "Does things.\n"
      "\n"
      "Usage: tool [options]\n"
      "\n"
      "  --help                   Show this text\n"
      "  --renderer vulkan|metal  Renderer to draw with. Default: vulkan\n"
      "\n"
      "Development:\n"
      "  --frames N               Stop after N frames\n"
      "  --headless               Run without a window\n"
      "\n"
      "Editor:\n"
      "  --project PATH           Project to open\n");
  }

  TEST(CommandLine, HelpIsTheSameBeforeAndAfterParsing)
  {
    CommandLine command_line("program", "Does things.");
    command_line.Add({.name = "path", .value_name = "PATH", .description = "A value", .default_value = "here"});
    const std::string before = command_line.GetHelp();

    ASSERT_TRUE(Parse(command_line, {"--path", "there"}));

    EXPECT_EQ(command_line.GetHelp(), before);
  }
}
