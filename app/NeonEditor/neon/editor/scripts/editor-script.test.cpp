#include "editor-script.hpp"

#include <string>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <neon/data/ryml-document-format.hpp>

namespace
{
  using neon::DataValue;
  using neon::EditorScript;
  using neon::Key;
  using neon::KeyEvent;
  using neon::RYML_DocumentFormat;
  using ::testing::Contains;
  using ::testing::ElementsAre;
  using ::testing::HasSubstr;
  using ::testing::IsEmpty;
  using ::testing::SizeIs;

  class EditorScriptTest : public ::testing::Test
  {
  protected:
    RYML_DocumentFormat _yaml;
    EditorScript _script;
    std::vector<std::string> _errors;

    DataValue Document(const std::string &text)
    {
      DataValue document;
      std::string error;
      EXPECT_TRUE(_yaml.Read("script", text, document, error)) << error;
      return document;
    }

    void Read(const std::string &text)
    {
      ASSERT_TRUE(EditorScript::Read(Document(text), "view-log.editor-script.yml", _script, _errors))
        << ::testing::PrintToString(_errors);
    }

    std::vector<std::string> ProblemsOf(const std::string &text)
    {
      std::vector<std::string> errors;
      EditorScript script;
      EXPECT_FALSE(EditorScript::Read(Document(text), "bad.editor-script.yml", script, errors));
      EXPECT_TRUE(script.IsEmpty());
      return errors;
    }
  };

  TEST_F(EditorScriptTest, AMenuAloneIsAPathOfOnePart)
  {
    Read("steps:\n  - menu: View\n");

    const auto &steps = _script.GetSteps();
    ASSERT_THAT(steps, SizeIs(1));
    EXPECT_EQ(steps[0].kind, EditorScript::Kind::Menu);
    EXPECT_THAT(steps[0].path, ElementsAre("View"));
    EXPECT_EQ(steps[0].Describe(), "menu View");
  }

  TEST_F(EditorScriptTest, ReadsEveryKindOfStepWithItsLine)
  {
    Read(
      "description: the Log through the View menu\n"
      "steps:\n"
      "  - menu: View > Log\n"
      "  - click: Close\n"
      "  - type: {field: Name, text: my-game}\n"
      "  - key: Ctrl+Shift+L\n"
      "  - pointer: {x: 200, y: 100}\n"
      "  - drag: {from: Log, to: \"900,300\"}\n"
      "  - wait: 2\n"
      "  - screenshot: output://log-shown.png\n"
      "  - expect: {window: Log, shown: false}\n"
      "  - expect: {text: a line}\n"
      "  - expect: {item: Close}\n");

    EXPECT_EQ(_script.GetDescription(), "the Log through the View menu");
    const auto &steps = _script.GetSteps();
    ASSERT_THAT(steps, SizeIs(11));

    EXPECT_EQ(steps[0].kind, EditorScript::Kind::Menu);
    EXPECT_THAT(steps[0].path, ElementsAre("View", "Log"));
    EXPECT_EQ(steps[0].line, 3u);
    EXPECT_EQ(steps[0].Describe(), "menu View > Log");

    EXPECT_EQ(steps[1].kind, EditorScript::Kind::Click);
    EXPECT_EQ(steps[1].label, "Close");
    EXPECT_EQ(steps[1].line, 4u);

    EXPECT_EQ(steps[2].kind, EditorScript::Kind::Type);
    EXPECT_EQ(steps[2].label, "Name");
    EXPECT_EQ(steps[2].text, "my-game");

    EXPECT_EQ(steps[3].kind, EditorScript::Kind::Key);
    EXPECT_EQ(steps[3].key.key, Key::L);
    EXPECT_TRUE(steps[3].key.modifiers.control);
    EXPECT_TRUE(steps[3].key.modifiers.shift);
    EXPECT_FALSE(steps[3].key.modifiers.alt);

    EXPECT_EQ(steps[4].kind, EditorScript::Kind::Pointer);
    EXPECT_FLOAT_EQ(steps[4].point.x, 200.0f);
    EXPECT_FLOAT_EQ(steps[4].point.y, 100.0f);

    EXPECT_EQ(steps[5].kind, EditorScript::Kind::Drag);
    EXPECT_FALSE(steps[5].from.is_point);
    EXPECT_EQ(steps[5].from.label, "Log");
    EXPECT_TRUE(steps[5].to.is_point);
    EXPECT_FLOAT_EQ(steps[5].to.point.x, 900.0f);
    EXPECT_FLOAT_EQ(steps[5].to.point.y, 300.0f);

    EXPECT_EQ(steps[6].kind, EditorScript::Kind::Wait);
    EXPECT_EQ(steps[6].frames, 2u);

    EXPECT_EQ(steps[7].kind, EditorScript::Kind::Screenshot);
    EXPECT_EQ(steps[7].label, "output://log-shown.png");

    EXPECT_EQ(steps[8].kind, EditorScript::Kind::ExpectWindow);
    EXPECT_EQ(steps[8].label, "Log");
    EXPECT_FALSE(steps[8].expected);
    EXPECT_EQ(steps[8].line, 11u);

    EXPECT_EQ(steps[9].kind, EditorScript::Kind::ExpectText);
    EXPECT_EQ(steps[9].text, "a line");

    EXPECT_EQ(steps[10].kind, EditorScript::Kind::ExpectItem);
    EXPECT_EQ(steps[10].label, "Close");
    EXPECT_TRUE(steps[10].expected);
  }

  TEST_F(EditorScriptTest, AnUnknownStepIsRefusedWithItsLine)
  {
    const auto errors = ProblemsOf(
      "steps:\n"
      "  - wait: 1\n"
      "  - press: Close\n");

    ASSERT_THAT(errors, SizeIs(1));
    EXPECT_THAT(errors[0], HasSubstr("bad.editor-script.yml:3:"));
    EXPECT_THAT(errors[0], HasSubstr("'press' is not a step"));
    EXPECT_THAT(errors[0], HasSubstr("menu, click, type"));
  }

  TEST_F(EditorScriptTest, EveryWrongStepIsReportedOnItsOwnLine)
  {
    const auto errors = ProblemsOf(
      "steps:\n"
      "  - menu: View >\n"
      "  - key: Hyper+L\n"
      "  - type: {field: Name}\n"
      "  - wait: 0\n"
      "  - expect: {window: Log, text: a}\n"
      "  - pointer: 200\n"
      "  - drag: {from: Log}\n");

    ASSERT_THAT(errors, SizeIs(7));
    EXPECT_THAT(errors[0], HasSubstr(":2: 'View >' is no path through the menus"));
    EXPECT_THAT(errors[1], HasSubstr(":3: 'Hyper+L' is no key"));
    EXPECT_THAT(errors[2], HasSubstr(":4: 'type' needs 'field'"));
    EXPECT_THAT(errors[3], HasSubstr(":5: 'wait' is followed by a number of frames"));
    EXPECT_THAT(errors[4], HasSubstr(":6: 'expect' names one of"));
    EXPECT_THAT(errors[5], HasSubstr(":7: 'pointer' is followed by a place"));
    EXPECT_THAT(errors[6], HasSubstr(":8: 'drag' needs 'from' and 'to'"));
  }

  TEST_F(EditorScriptTest, AScriptIsAMapWithAListOfSteps)
  {
    EXPECT_THAT(ProblemsOf("- menu: View > Log\n"), Contains(HasSubstr("a map with a list of steps")));
    EXPECT_THAT(ProblemsOf("description: nothing\n"), Contains(HasSubstr("a list of steps under 'steps'")));
    EXPECT_THAT(ProblemsOf("steps:\n  - View > Log\n"), Contains(HasSubstr("a step is a map of one key")));
    EXPECT_THAT(ProblemsOf("steps:\n  - {menu: View > Log, wait: 1}\n"), Contains(HasSubstr("a step is a map of one key")));
  }

  TEST_F(EditorScriptTest, ANameTheScriptDoesNotKnowIsReported)
  {
    EXPECT_THAT(ProblemsOf("steps: []\nname: x\n"), Contains(HasSubstr("name")));
  }

  TEST_F(EditorScriptTest, ReadsAChordInAnyCaseWithTheKeyLast)
  {
    KeyEvent key;
    ASSERT_TRUE(EditorScript::ReadChord("cmd+shift+l", key));
    EXPECT_EQ(key.key, Key::L);
    EXPECT_TRUE(key.modifiers.super);
    EXPECT_TRUE(key.modifiers.shift);

    ASSERT_TRUE(EditorScript::ReadChord("Shortcut+S", key));
    EXPECT_EQ(key.key, Key::S);
    EXPECT_TRUE(key.modifiers.shortcut);
    EXPECT_FALSE(key.modifiers.control);

    ASSERT_TRUE(EditorScript::ReadChord("enter", key));
    EXPECT_EQ(key.key, Key::Enter);

    ASSERT_TRUE(EditorScript::ReadChord("Alt+F5", key));
    EXPECT_EQ(key.key, Key::F5);
    EXPECT_TRUE(key.modifiers.alt);

    EXPECT_FALSE(EditorScript::ReadChord("Ctrl+", key));
    EXPECT_FALSE(EditorScript::ReadChord("Ctrl+Hyper", key));
    EXPECT_FALSE(EditorScript::ReadChord("", key));
  }
}
