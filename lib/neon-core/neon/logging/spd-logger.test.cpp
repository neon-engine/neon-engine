#include "spd-logger.hpp"

#include <format>
#include <memory>
#include <sstream>
#include <string>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <spdlog/pattern_formatter.h>
#include <spdlog/sinks/ostream_sink.h>

namespace
{
  using neon::Logger;
  using neon::Spd_Logger;

  /// A logger that writes into a text, one line per message, as
  /// `level|name|message`.
  class SpdLoggerTest : public ::testing::Test
  {
  protected:
    std::ostringstream _output;
    std::shared_ptr<spdlog::logger> _spdlog_logger;
    std::shared_ptr<Logger> _logger;

    void SetUp() override
    {
      const auto sink = std::make_shared<spdlog::sinks::ostream_sink_mt>(_output);
      _spdlog_logger = std::make_shared<spdlog::logger>("Test", sink);
      _spdlog_logger->set_level(spdlog::level::trace);
      _spdlog_logger->set_formatter(
        std::make_unique<spdlog::pattern_formatter>("%l|%n|%v", spdlog::pattern_time_type::local, "\n"));

      _logger = std::make_shared<Spd_Logger>(_spdlog_logger, nullptr);
    }

    std::string Output()
    {
      _spdlog_logger->flush();
      return _output.str();
    }
  };

  TEST_F(SpdLoggerTest, WritesEveryLevelUnderItsName)
  {
    _logger->Trace("one");
    _logger->Debug("two");
    _logger->Info("three");
    _logger->Warn("four");
    _logger->Error("five");
    _logger->Critical("six");

    EXPECT_EQ(
      Output(),
      "trace|Test|one\n"
      "debug|Test|two\n"
      "info|Test|three\n"
      "warning|Test|four\n"
      "error|Test|five\n"
      "critical|Test|six\n");
  }

  TEST_F(SpdLoggerTest, PutsTheArgumentsWhereTheBracesAre)
  {
    const std::string name = "cube";
    const int count = 3;
    const double seconds = 0.5;

    _logger->Info("Loaded {} in {} parts, took {} seconds", name, count, seconds);

    EXPECT_EQ(Output(), "info|Test|Loaded cube in 3 parts, took 0.5 seconds\n");
  }

  TEST_F(SpdLoggerTest, FollowsTheFormatOfAnArgument)
  {
    const int width = 1920;
    const double ratio = 1.0 / 3.0;

    _logger->Info("{:>6} {:.2f} {:04}", width, ratio, width);

    EXPECT_EQ(Output(), "info|Test|  1920 0.33 1920\n");
  }

  TEST_F(SpdLoggerTest, WritesDoubledBracesAsBraces)
  {
    // the logger formats lvalues only
    const int one = 1;

    _logger->Info("{{}} holds {{{}}}", one);

    EXPECT_EQ(Output(), "info|Test|{} holds {1}\n");
  }

  TEST_F(SpdLoggerTest, WritesBracesOfAnArgumentAsTheyAre)
  {
    const std::string text = "{not an argument} {}";

    _logger->Error("{}", text);

    EXPECT_EQ(Output(), "error|Test|{not an argument} {}\n");
  }

  TEST_F(SpdLoggerTest, WritesAnEmptyMessage)
  {
    _logger->Info("");

    EXPECT_EQ(Output(), "info|Test|\n");
  }

  TEST_F(SpdLoggerTest, IgnoresArgumentsTheMessageDoesNotName)
  {
    const int unused = 5;

    _logger->Info("nothing to fill in", unused);

    EXPECT_EQ(Output(), "info|Test|nothing to fill in\n");
  }

  TEST_F(SpdLoggerTest, ThrowsWhenTheMessageNamesMoreArgumentsThanItWasGiven)
  {
    const int one = 1;

    EXPECT_THROW(_logger->Info("{} and {}", one), std::format_error);
    EXPECT_EQ(Output(), "");
  }

  TEST_F(SpdLoggerTest, ThrowsWhenTheMessageHasABraceThatIsNotClosed)
  {
    EXPECT_THROW(_logger->Error("not closed {"), std::format_error);
    EXPECT_EQ(Output(), "");
  }

  TEST_F(SpdLoggerTest, LeavesOutWhatIsBelowTheLevelOfTheLogger)
  {
    _spdlog_logger->set_level(spdlog::level::warn);

    _logger->Trace("one");
    _logger->Debug("two");
    _logger->Info("three");
    _logger->Warn("four");
    _logger->Error("five");

    EXPECT_EQ(
      Output(),
      "warning|Test|four\n"
      "error|Test|five\n");
  }
}
