#include "input-map-loading.hpp"

#include <memory>
#include <string>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <neon/data/ryml-document-format.hpp>
#include <neon/input/headless-input-system.hpp>
#include <neon/testing/memory-file-system.hpp>
#include <neon/testing/recording-logger.hpp>
#include <neon/testing/recording-logging-context.hpp>

namespace
{
  using neon::Headless_InputSystem;
  using neon::InputMapLoading;
  using neon::RYML_DocumentFormat;
  using neon::testing::MemoryFileSystem;
  using neon::testing::RecordingLogger;
  using neon::testing::RecordingLoggingContext;
  using ::testing::ElementsAre;
  using ::testing::HasSubstr;
  using ::testing::IsEmpty;
  using ::testing::Not;

  const std::string map_with_jump =
    "version: 1\n"
    "actions:\n"
    "  jump: { type: button, keys: [space] }\n"
    "states:\n"
    "  playing: [jump]\n";

  const std::string map_with_fire =
    "version: 1\n"
    "actions:\n"
    "  fire: { type: button, keys: [f] }\n"
    "states:\n"
    "  playing: [fire]\n";

  class InputMapLoadingTest : public ::testing::Test
  {
  protected:
    std::shared_ptr<RecordingLogger> _logger = std::make_shared<RecordingLogger>();
    MemoryFileSystem _files{SettingsConfig{}, _logger};
    RYML_DocumentFormat _yaml;
    RecordingLoggingContext _logging;
    InputMapLoading _loading{&_files, &_yaml, &_logging, "the runtime"};

    Headless_InputSystem _input{SettingsConfig{}, _logger};
    std::vector<std::string> _remarks;
    std::vector<std::string> _errors;

    void SetUp() override
    {
      _files.Initialize();
    }

    bool Load(const std::string &project_input)
    {
      return _loading.Load(project_input, _input, _remarks, _errors);
    }
  };

  TEST_F(InputMapLoadingTest, LoadsTheMapTheProjectNames)
  {
    _files.AddNativeFile("/assets/input/game.input.yml", map_with_fire);
    _files.AddNativeFile("/engine/input/default.input.yml", map_with_jump);

    ASSERT_TRUE(Load("assets://input/game.input.yml")) << ::testing::PrintToString(_errors);
    EXPECT_NE(_input.GetInputMap().FindAction("fire"), nullptr);
    EXPECT_EQ(_input.GetInputMap().FindAction("jump"), nullptr);
    EXPECT_THAT(_remarks, IsEmpty());
  }

  TEST_F(InputMapLoadingTest, LoadsTheEnginesMapWhenTheProjectNamesNone)
  {
    _files.AddNativeFile("/engine/input/default.input.yml", map_with_jump);

    ASSERT_TRUE(Load("")) << ::testing::PrintToString(_errors);
    EXPECT_NE(_input.GetInputMap().FindAction("jump"), nullptr);
    EXPECT_THAT(_remarks, IsEmpty());
  }

  TEST_F(InputMapLoadingTest, RemarksThatTheBuiltInMapIsUsedWithoutTheEngines)
  {
    EXPECT_TRUE(Load(""));
    EXPECT_THAT(_errors, IsEmpty());
    EXPECT_THAT(
      _remarks,
      ElementsAre(
        "engine://input/default.input.yml is not next to the runtime, so the input map built into the engine is used"));
  }

  TEST_F(InputMapLoadingTest, FailsWithTheErrorsOfAMapThatCannotBeRead)
  {
    EXPECT_FALSE(Load("assets://input/missing.input.yml"));
    EXPECT_THAT(_errors, Not(IsEmpty()));
    EXPECT_THAT(_errors[0], HasSubstr("assets://input/missing.input.yml"));
  }
}
