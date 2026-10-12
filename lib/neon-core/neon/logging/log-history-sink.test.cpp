#include "log-history-sink.hpp"

#include <memory>
#include <string>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "spdlog/logger.h"

namespace
{
  using neon::LogHistorySink;
  using ::testing::ElementsAre;
  using ::testing::EndsWith;

  TEST(LogHistorySink, KeepsTheLastLinesAsTheyWereFormatted)
  {
    const auto sink = std::make_shared<LogHistorySink>(2);
    spdlog::logger logger("test", sink);
    logger.set_pattern("%n: %v");

    logger.info("one");
    logger.info("two");
    logger.warn("three");

    std::vector<std::string> lines;
    sink->CopyLines(lines);
    EXPECT_THAT(lines, ElementsAre("test: two", "test: three"));
    EXPECT_EQ(sink->Revision(), 3u);
  }

  TEST(LogHistorySink, IsEmptyBeforeAnythingIsLogged)
  {
    const auto sink = std::make_shared<LogHistorySink>();

    std::vector<std::string> lines{"stale"};
    sink->CopyLines(lines);
    EXPECT_TRUE(lines.empty());
    EXPECT_EQ(sink->Revision(), 0u);
  }
}
