#include "editor-script-runner.hpp"

#include <memory>
#include <string>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <neon/data/ryml-document-format.hpp>
#include <neon/testing/mock-render-system.hpp>
#include <neon/testing/recording-logger.hpp>
#include <neon/testing/recording-tool-ui.hpp>

namespace
{
  using neon::DataValue;
  using neon::EditorScript;
  using neon::EditorScriptRunner;
  using neon::Key;
  using neon::RYML_DocumentFormat;
  using neon::ToolUiEvents;
  using neon::ToolUiItemKind;
  using neon::testing::LogLevel;
  using neon::testing::MockRenderSystem;
  using neon::testing::RecordingLogger;
  using neon::testing::RecordingToolUi;
  using ::testing::HasSubstr;
  using ::testing::Return;

  /// An editor the size of a test on the fake: a File menu with Quit, a
  /// View menu that switches the Log on and off, a field, and a button.
  class EditorScriptRunnerTest : public ::testing::Test
  {
  protected:
    std::shared_ptr<RecordingLogger> _logger = std::make_shared<RecordingLogger>();
    MockRenderSystem _render_system{_logger};
    RYML_DocumentFormat _yaml;
    RecordingToolUi _ui;
    EditorScript _script;

    bool _show_log = false;
    bool _quit = false;
    bool _closed = false;
    std::string _name;

    void SetUp() override
    {
      _ui.menus_follow_clicks = true;
    }

    void Draw()
    {
      _ui.BeginMainMenuBar();
      if (_ui.BeginMenu("File"))
      {
        if (_ui.MenuItem("Quit", "", false, true)) { _quit = true; }
        _ui.EndMenu();
      }
      if (_ui.BeginMenu("View"))
      {
        if (_ui.MenuItem("Log", "", _show_log, true)) { _show_log = !_show_log; }
        _ui.EndMenu();
      }
      _ui.EndMainMenuBar();

      if (_show_log)
      {
        _ui.BeginWindow("Log", true);
        _ui.Text("a line of the log");
        _ui.EndWindow();
      }

      _ui.InputText("Name", _name);
      if (_ui.Button("Close")) { _closed = true; }
    }

    /// The runner of a script written as its steps, one a line.
    EditorScriptRunner Runner(const std::string &steps)
    {
      DataValue document;
      std::string error;
      EXPECT_TRUE(_yaml.Read("script", "steps:\n" + steps, document, error)) << error;
      std::vector<std::string> errors;
      EXPECT_TRUE(EditorScript::Read(document, "test.editor-script.yml", _script, errors))
        << ::testing::PrintToString(errors);
      return EditorScriptRunner(&_script, &_render_system, _logger);
    }

    /// Runs frames until the runner is done or failed, or `most` frames
    /// went by, as the editor's loop does. Returns how many it took.
    std::size_t Run(EditorScriptRunner &runner, const std::size_t most = 40)
    {
      std::size_t frames = 0;
      while (!runner.IsDone() && !runner.HasFailed() && frames < most)
      {
        runner.BeforeFrame(_ui);
        _ui.NextFrame();
        Draw();
        runner.AfterFrame(_ui);
        frames++;
      }
      return frames;
    }
  };

  TEST_F(EditorScriptRunnerTest, AnEmptyScriptIsDoneAtOnce)
  {
    EditorScriptRunner runner = Runner(" []\n");
    EXPECT_TRUE(runner.IsDone());
    EXPECT_FALSE(runner.HasFailed());
  }

  TEST_F(EditorScriptRunnerTest, AMenuPathOpensTheMenuAndChoosesTheItem)
  {
    EditorScriptRunner runner = Runner(
      "  - menu: View > Log\n"
      "  - expect: {window: Log, shown: true}\n"
      "  - expect: {text: line of the}\n");

    Run(runner);

    EXPECT_TRUE(runner.IsDone()) << _logger->Messages(LogLevel::Error);
    EXPECT_TRUE(_show_log);
    EXPECT_FALSE(_quit);
    // the View menu was opened with a click, and closed by the choice
    EXPECT_TRUE(_ui.GetOpenMenus().empty());
    EXPECT_TRUE(_logger->Contains(LogLevel::Info, "'menu View > Log' on line 2, is done"));
    EXPECT_TRUE(_logger->Contains(LogLevel::Info, "The script is done, all 3 steps"));
  }

  TEST_F(EditorScriptRunnerTest, TheSameItemChosenAgainSwitchesTheWindowOff)
  {
    EditorScriptRunner runner = Runner(
      "  - menu: View > Log\n"
      "  - expect: {window: Log, shown: true}\n"
      "  - menu: View > Log\n"
      "  - expect: {window: Log, shown: false}\n");

    Run(runner);

    EXPECT_TRUE(runner.IsDone()) << _logger->Messages(LogLevel::Error);
    EXPECT_FALSE(_show_log);
  }

  TEST_F(EditorScriptRunnerTest, AClickFindsAButtonByItsLabel)
  {
    EditorScriptRunner runner = Runner("  - click: Close\n");

    // a frame to see what is drawn, then the pointer is moved, the button
    // goes down, and up: four frames
    EXPECT_EQ(Run(runner), 4u);
    EXPECT_TRUE(runner.IsDone());
    EXPECT_TRUE(_closed);
  }

  TEST_F(EditorScriptRunnerTest, TypingClicksTheFieldAndTypesIntoIt)
  {
    EditorScriptRunner runner = Runner("  - type: {field: Name, text: my-game}\n");

    Run(runner);

    EXPECT_TRUE(runner.IsDone());
    EXPECT_EQ(_name, "my-game");
  }

  TEST_F(EditorScriptRunnerTest, AKeyIsFedDownAndUpWithWhatIsHeld)
  {
    EditorScriptRunner runner = Runner("  - key: Ctrl+L\n");

    Run(runner);

    EXPECT_TRUE(runner.IsDone());
    ASSERT_EQ(_ui.events_fed.size(), 1u);
    const ToolUiEvents &events = _ui.events_fed[0];
    ASSERT_EQ(events.keys.size(), 2u);
    EXPECT_EQ(events.keys[0].key, Key::L);
    EXPECT_TRUE(events.keys[0].is_down);
    EXPECT_TRUE(events.keys[0].modifiers.control);
    EXPECT_FALSE(events.keys[1].is_down);
  }

  TEST_F(EditorScriptRunnerTest, ThePointerIsPutWhereTheScriptSays)
  {
    EditorScriptRunner runner = Runner("  - pointer: {x: 200, y: 100}\n");

    Run(runner);

    EXPECT_TRUE(runner.IsDone());
    ASSERT_TRUE(_ui.GetPointer().has_value());
    EXPECT_FLOAT_EQ(_ui.GetPointer()->x, 200.0f);
    EXPECT_FLOAT_EQ(_ui.GetPointer()->y, 100.0f);
  }

  TEST_F(EditorScriptRunnerTest, ADragHoldsTheButtonFromOneEndToTheOther)
  {
    EditorScriptRunner runner = Runner("  - drag: {from: Close, to: \"300,200\"}\n");

    Run(runner);

    EXPECT_TRUE(runner.IsDone()) << _logger->Messages(LogLevel::Error);
    ASSERT_GE(_ui.events_fed.size(), 4u);
    EXPECT_EQ(_ui.events_fed[1].button_down, true);
    EXPECT_EQ(_ui.events_fed.back().button_down, false);
    EXPECT_FLOAT_EQ(_ui.events_fed.back().pointer->x, 300.0f);
    EXPECT_FLOAT_EQ(_ui.events_fed.back().pointer->y, 200.0f);
  }

  TEST_F(EditorScriptRunnerTest, AWaitLetsThatManyFramesPass)
  {
    EditorScriptRunner runner = Runner("  - wait: 4\n");

    EXPECT_EQ(Run(runner), 4u);
    EXPECT_TRUE(runner.IsDone());
  }

  TEST_F(EditorScriptRunnerTest, AScreenshotSavesTheFrame)
  {
    EXPECT_CALL(_render_system, CaptureFrame("output://log-shown.png")).WillOnce(Return(true));
    EditorScriptRunner runner = Runner("  - screenshot: output://log-shown.png\n");

    EXPECT_EQ(Run(runner), 1u);
    EXPECT_TRUE(runner.IsDone());
  }

  TEST_F(EditorScriptRunnerTest, AScreenshotThatCannotBeSavedFails)
  {
    EXPECT_CALL(_render_system, CaptureFrame("output://log-shown.png")).WillOnce(Return(false));
    EditorScriptRunner runner = Runner("  - screenshot: output://log-shown.png\n");

    Run(runner);

    EXPECT_TRUE(runner.HasFailed());
    EXPECT_TRUE(_logger->Contains(LogLevel::Error, "Step 1 of the script, 'screenshot output://log-shown.png' on line 2, failed"));
  }

  TEST_F(EditorScriptRunnerTest, AFailedExpectationNamesTheStepAndItsLineAndStopsTheScript)
  {
    EditorScriptRunner runner = Runner(
      "  - wait: 1\n"
      "  - expect: {window: Log, shown: true}\n"
      "  - click: Close\n");

    Run(runner);

    EXPECT_TRUE(runner.HasFailed());
    EXPECT_FALSE(runner.IsDone());
    EXPECT_EQ(runner.GetStepsDone(), 1u);
    EXPECT_FALSE(_closed);
    EXPECT_THAT(
      _logger->Messages(LogLevel::Error),
      HasSubstr("Step 2 of the script, 'expect window Log shown' on line 3, failed: the window 'Log' is not shown. Shown: no window"));
  }

  TEST_F(EditorScriptRunnerTest, AnItemIsExpectedPresentOrAbsent)
  {
    EditorScriptRunner runner = Runner(
      "  - expect: {item: Close, present: true}\n"
      "  - expect: {item: Log, present: false}\n"
      "  - expect: {item: Log}\n");

    Run(runner);

    EXPECT_TRUE(runner.HasFailed());
    EXPECT_EQ(runner.GetStepsDone(), 2u);
    EXPECT_THAT(_logger->Messages(LogLevel::Error), HasSubstr("'Log' is not drawn in this frame"));
  }

  TEST_F(EditorScriptRunnerTest, ALabelThatIsNeverDrawnFailsAfterAFewFrames)
  {
    EditorScriptRunner runner = Runner("  - click: Build\n");

    Run(runner);

    EXPECT_TRUE(runner.HasFailed());
    EXPECT_THAT(_logger->Messages(LogLevel::Error), HasSubstr("'Build' was not drawn in 10 frames. Drawn: "));
    EXPECT_THAT(_logger->Messages(LogLevel::Error), HasSubstr("button 'Close'"));
  }

  TEST_F(EditorScriptRunnerTest, QuitIsChosenThroughTheFileMenu)
  {
    EditorScriptRunner runner = Runner("  - menu: File > Quit\n");

    Run(runner);

    EXPECT_TRUE(runner.IsDone()) << _logger->Messages(LogLevel::Error);
    EXPECT_TRUE(_quit);
  }
}
