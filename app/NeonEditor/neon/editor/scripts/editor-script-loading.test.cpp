#include "editor-script-loading.hpp"

#include <memory>
#include <string>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <neon/data/ryml-document-format.hpp>
#include <neon/testing/memory-file-system.hpp>
#include <neon/testing/recording-logger.hpp>

namespace
{
  using neon::EditorScript;
  using neon::EditorScriptLoading;
  using neon::RYML_DocumentFormat;
  using neon::testing::MemoryFileSystem;
  using neon::testing::RecordingLogger;
  using ::testing::Contains;
  using ::testing::HasSubstr;
  using ::testing::IsEmpty;
  using ::testing::SizeIs;

  class EditorScriptLoadingTest : public ::testing::Test
  {
  protected:
    std::shared_ptr<RecordingLogger> _logger = std::make_shared<RecordingLogger>();
    MemoryFileSystem _files{SettingsConfig{}, _logger};
    RYML_DocumentFormat _yaml;
    EditorScriptLoading _loading{&_files, &_yaml};
    SettingsConfig _settings;
    EditorScript _script;
    std::vector<std::string> _errors;

    void SetUp() override
    {
      _files.Initialize();
    }
  };

  TEST_F(EditorScriptLoadingTest, WithoutAPathThereIsNoScript)
  {
    EXPECT_TRUE(_loading.Load(_settings, _script, _errors));
    EXPECT_TRUE(_script.IsEmpty());
    EXPECT_THAT(_errors, IsEmpty());
  }

  TEST_F(EditorScriptLoadingTest, ReadsTheScriptTheSettingsName)
  {
    _files.AddNativeFile("/user/view-log.editor-script.yml", "steps:\n  - menu: View > Log\n  - wait: 2\n");
    _settings.editor_script_path = "user://view-log.editor-script.yml";

    ASSERT_TRUE(_loading.Load(_settings, _script, _errors)) << ::testing::PrintToString(_errors);
    EXPECT_THAT(_script.GetSteps(), SizeIs(2));
  }

  TEST_F(EditorScriptLoadingTest, AFileThatIsNotThereIsSaid)
  {
    _settings.editor_script_path = "user://missing.editor-script.yml";

    EXPECT_FALSE(_loading.Load(_settings, _script, _errors));
    EXPECT_THAT(_errors, Contains(HasSubstr("user://missing.editor-script.yml cannot be read")));
  }

  TEST_F(EditorScriptLoadingTest, AFileThatIsNoDocumentIsSaid)
  {
    _files.AddNativeFile("/user/broken.editor-script.yml", "steps: [\n");
    _settings.editor_script_path = "user://broken.editor-script.yml";

    EXPECT_FALSE(_loading.Load(_settings, _script, _errors));
    ASSERT_THAT(_errors, SizeIs(1));
    EXPECT_THAT(_errors[0], HasSubstr("broken.editor-script.yml"));
  }

  TEST_F(EditorScriptLoadingTest, AStepThatIsWrongIsSaidWithItsLine)
  {
    _files.AddNativeFile("/user/wrong.editor-script.yml", "steps:\n  - wait: 1\n  - press: Close\n");
    _settings.editor_script_path = "user://wrong.editor-script.yml";

    EXPECT_FALSE(_loading.Load(_settings, _script, _errors));
    EXPECT_THAT(_errors, Contains(HasSubstr("user://wrong.editor-script.yml:3: 'press' is not a step")));
  }
}
