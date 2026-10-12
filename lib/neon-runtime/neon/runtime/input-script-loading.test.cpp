#include "input-script-loading.hpp"

#include <memory>
#include <string>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <neon/input/headless-input-system.hpp>
#include <neon/testing/memory-file-system.hpp>
#include <neon/testing/recording-logger.hpp>

namespace
{
  using neon::Headless_InputSystem;
  using neon::InputScriptLoading;
  using neon::testing::MemoryFileSystem;
  using neon::testing::RecordingLogger;
  using ::testing::ElementsAre;
  using ::testing::HasSubstr;
  using ::testing::IsEmpty;
  using ::testing::Not;

  class InputScriptLoadingTest : public ::testing::Test
  {
  protected:
    std::shared_ptr<RecordingLogger> _logger = std::make_shared<RecordingLogger>();
    MemoryFileSystem _files{SettingsConfig{}, _logger};
    InputScriptLoading _loading{&_files};

    SettingsConfig _settings;
    Headless_InputSystem _input{SettingsConfig{}, _logger};
    std::vector<std::string> _errors;

    void SetUp() override
    {
      _files.Initialize();
    }

    bool Load()
    {
      return _loading.Load(_settings, _input, _errors);
    }
  };

  TEST_F(InputScriptLoadingTest, LoadsNothingWhenNoScriptIsNamed)
  {
    EXPECT_TRUE(Load());
    EXPECT_THAT(_errors, IsEmpty());
  }

  TEST_F(InputScriptLoadingTest, LoadsTheScriptOfTheOption)
  {
    _settings.input_script = "2: pointer 640 360";

    EXPECT_TRUE(Load()) << ::testing::PrintToString(_errors);
    EXPECT_THAT(_errors, IsEmpty());
  }

  TEST_F(InputScriptLoadingTest, LoadsTheScriptOfTheFile)
  {
    _files.AddNativeFile("/assets/checks/walk.input", "1: pointer 5 5\n3: click\n");
    _settings.input_script_path = "assets://checks/walk.input";

    EXPECT_TRUE(Load()) << ::testing::PrintToString(_errors);
    EXPECT_THAT(_errors, IsEmpty());
  }

  TEST_F(InputScriptLoadingTest, FailsWhenTheFileCannotBeRead)
  {
    _settings.input_script_path = "assets://checks/missing.input";

    EXPECT_FALSE(Load());
    EXPECT_THAT(_errors, ElementsAre("The script of input assets://checks/missing.input cannot be read"));
  }

  TEST_F(InputScriptLoadingTest, FailsWithTheErrorsOfAScriptThatIsWrong)
  {
    _settings.input_script = "1: device gamepad dreamcast";

    EXPECT_FALSE(Load());
    EXPECT_THAT(_errors, Not(IsEmpty()));
    EXPECT_THAT(_errors[0], HasSubstr("--input"));
  }
}
