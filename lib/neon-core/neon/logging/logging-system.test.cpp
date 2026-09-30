#include "logging-system.hpp"

#include <memory>
#include <string>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <neon/testing/temporary-directory.hpp>

namespace
{
  using neon::LoggingSystem;
  using neon::testing::TemporaryDirectory;
  using ::testing::HasSubstr;
  using ::testing::Not;

  /// The logging system writes a file, so each test gets a folder of its own
  /// that is removed afterwards. What is logged also goes to the standard
  /// output, which is why these tests print.
  class LoggingSystemTest : public ::testing::Test
  {
  protected:
    TemporaryDirectory _directory;
    SettingsConfig _settings;

    void SetUp() override
    {
      _settings.logpath = _directory.Native("logs/engine.log");
    }

    /// What the log file holds. Loggers write when they go away at the
    /// latest, so call this after the logging system is gone.
    [[nodiscard]] std::string LogFile() const
    {
      return _directory.Read("logs/engine.log");
    }
  };

  TEST_F(LoggingSystemTest, WritesNoFileBeforeItIsInitialized)
  {
    {
      const LoggingSystem logging_system(_settings);
    }

    EXPECT_FALSE(_directory.HasFile("logs/engine.log"));
    EXPECT_FALSE(_directory.HasDirectory("logs"));
  }

  TEST_F(LoggingSystemTest, CreatesTheLogFileAndTheFolderItIsIn)
  {
    {
      LoggingSystem logging_system(_settings);
      logging_system.Initialize();
    }

    EXPECT_TRUE(_directory.HasFile("logs/engine.log"));
    EXPECT_THAT(LogFile(), HasSubstr("[LoggingSystem] [info] LoggingSystem initialized"));
  }

  TEST_F(LoggingSystemTest, WritesWhatALoggerIsToldUnderItsNameAndLevel)
  {
    {
      LoggingSystem logging_system(_settings);
      logging_system.Initialize();

      const auto logger = logging_system.CreateLogger("Renderer");
      const int count = 3;
      logger->Debug("debug {}", count);
      logger->Info("info {}", count);
      logger->Warn("warn {}", count);
      logger->Error("error {}", count);
      logger->Critical("critical {}", count);
    }

    const std::string log = LogFile();
    EXPECT_THAT(log, HasSubstr("[Renderer] [debug] debug 3"));
    EXPECT_THAT(log, HasSubstr("[Renderer] [info] info 3"));
    EXPECT_THAT(log, HasSubstr("[Renderer] [warning] warn 3"));
    EXPECT_THAT(log, HasSubstr("[Renderer] [error] error 3"));
    EXPECT_THAT(log, HasSubstr("[Renderer] [critical] critical 3"));
  }

  TEST_F(LoggingSystemTest, LeavesOutWhatIsTraced)
  {
    {
      LoggingSystem logging_system(_settings);
      logging_system.Initialize();

      logging_system.CreateLogger("Renderer")->Trace("every little step");
    }

    EXPECT_THAT(LogFile(), Not(HasSubstr("every little step")));
  }

  TEST_F(LoggingSystemTest, SaysWhichLoggersWereCreated)
  {
    {
      LoggingSystem logging_system(_settings);
      logging_system.Initialize();

      const auto logger = logging_system.CreateLogger("Renderer");
    }

    EXPECT_THAT(LogFile(), HasSubstr("[LoggingSystem] [debug] Creating logger for Renderer"));
  }

  TEST_F(LoggingSystemTest, WritesTheMessagesOfAllLoggersIntoOneFileInOrder)
  {
    {
      LoggingSystem logging_system(_settings);
      logging_system.Initialize();

      const auto renderer = logging_system.CreateLogger("Renderer");
      const auto world = logging_system.CreateLogger("World");
      renderer->Info("first");
      world->Info("second");
      renderer->Info("third");
    }

    const std::string log = LogFile();
    const auto first = log.find("[Renderer] [info] first");
    const auto second = log.find("[World] [info] second");
    const auto third = log.find("[Renderer] [info] third");

    ASSERT_NE(first, std::string::npos);
    ASSERT_NE(second, std::string::npos);
    ASSERT_NE(third, std::string::npos);
    EXPECT_LT(first, second);
    EXPECT_LT(second, third);
  }

  TEST_F(LoggingSystemTest, HandsOutALoggerOfItsOwnEachTime)
  {
    LoggingSystem logging_system(_settings);
    logging_system.Initialize();

    EXPECT_NE(logging_system.CreateLogger("Renderer"), logging_system.CreateLogger("Renderer"));
  }

  TEST_F(LoggingSystemTest, LetsALoggerOutliveIt)
  {
    std::shared_ptr<neon::Logger> logger;
    {
      LoggingSystem logging_system(_settings);
      logging_system.Initialize();
      logger = logging_system.CreateLogger("Renderer");
    }

    logger->Info("still here");
    logger.reset();

    EXPECT_THAT(LogFile(), HasSubstr("[Renderer] [info] still here"));
  }

  TEST_F(LoggingSystemTest, SaysWhenItIsCleanedUpAndKeepsLogging)
  {
    {
      LoggingSystem logging_system(_settings);
      logging_system.Initialize();
      const auto logger = logging_system.CreateLogger("Renderer");

      logging_system.CleanUp();
      logger->Info("after the clean up");
    }

    const std::string log = LogFile();
    EXPECT_THAT(log, HasSubstr("[LoggingSystem] [warning] LoggingSystem cleanup"));
    EXPECT_THAT(log, HasSubstr("[Renderer] [info] after the clean up"));
  }

  TEST_F(LoggingSystemTest, AddsToALogFileThatExists)
  {
    _directory.Write("logs/engine.log", "what an earlier run wrote\n");

    {
      LoggingSystem logging_system(_settings);
      logging_system.Initialize();
    }

    const std::string log = LogFile();
    EXPECT_THAT(log, HasSubstr("what an earlier run wrote"));
    EXPECT_THAT(log, HasSubstr("LoggingSystem initialized"));
  }

  TEST_F(LoggingSystemTest, ThrowsWhenTheLogFileCannotBeCreated)
  {
    // a file is where the folder of the log file would have to be
    _directory.Write("logs", "in the way");

    LoggingSystem logging_system(_settings);

    EXPECT_THROW(logging_system.Initialize(), std::exception);
  }
}
