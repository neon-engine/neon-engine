#include "ui-fixture.hpp"

#include <neon/input/clipboard-context.hpp>
#include <neon/ui/elements/ui-text-field.hpp>

// Typing into an input and a textarea: the caret, the selection, the
// clipboard, undo, the input method, and what the game is told.
//
// The font of the tests is 8 wide per character at the size of 16. An input
// has a border of 1 and a padding of 6 by 10, so its text starts 11 to the
// right of its left edge, and the input is 30 high.

namespace
{
  using neon::Action;
  using neon::FieldValue;
  using neon::Key;
  using neon::KeyModifiers;
  using neon::Memory_Clipboard;
  using neon::TextComposition;
  using neon::TextInputArea;
  using neon::UiHandle;
  using neon::UiValue;
  using neon::testing::LogLevel;
  using neon::testing::UiTest;
  using ::testing::ElementsAre;
  using ::testing::IsEmpty;

  constexpr float text_left = 11.0f;
  constexpr float glyph = 8.0f;

  class UiTextInputTest : public UiTest
  {
  protected:
    Memory_Clipboard _clipboard;

    void SetUp() override
    {
      UiTest::SetUp();
      _ui->SetClipboard(&_clipboard);
    }

    /// An input at the left top corner, under a label.
    void ShowInput(const std::string &attributes = "", const std::string &top = "", const std::string &width = "200")
    {
      ASSERT_GE(Show(
        top +
        "ui: test\n"
        "root:\n"
        "  type: panel\n"
        "  width: 100%\n"
        "  height: 100%\n"
        "  flex_direction: column\n"
        "  align_items: flex-start\n"
        "  children:\n"
        "    - type: input\n"
        "      name: field\n"
        "      width: " + width + "\n" +
        Indented(attributes, "      ") +
        "    - type: button\n"
        "      name: ok\n"
        "      text: OK\n"), 0)
        << _logger->Messages(LogLevel::Error);

      Frame();
    }

    void ShowTextArea(const std::string &attributes = "", const std::string &width = "200")
    {
      ASSERT_GE(ShowUnderRoot(
        "- type: textarea\n"
        "  name: field\n"
        "  width: " + width + "\n" +
        Indented(attributes, "  "),
        "align_items: flex-start\n"), 0)
        << _logger->Messages(LogLevel::Error);

      Frame();
    }

    [[nodiscard]] UiHandle Field() const
    {
      return _ui->FindByName("field");
    }

    [[nodiscard]] std::string Value() const
    {
      FieldValue value;
      return _ui->GetField(Field(), "value", value) ? std::get<std::string>(value) : "(none)";
    }

    [[nodiscard]] const neon::UiTextEditor &Editor() const
    {
      return dynamic_cast<const neon::UiTextField &>(Element("field")).GetEditor();
    }

    void Type(const std::string &text)
    {
      Release();
      _input.state.AddText(text);
      Frame();
      Release();
    }

    void PressKey(const Key key, const KeyModifiers modifiers = {})
    {
      Release();
      _input.state.AddKeyEvent({key, true, false, modifiers});
      _input.state.AddKeyEvent({key, false, false, modifiers});
      Frame();
      Release();
    }

    /// The key that makes a shortcut, with shift or not.
    void Shortcut(const Key key, const bool shift = false)
    {
      KeyModifiers modifiers;
      modifiers.shortcut = true;
      modifiers.shift = shift;
      PressKey(key, modifiers);
    }

    /// The key that moves by a word, with shift or not.
    void WordKey(const Key key, const bool shift = false)
    {
      KeyModifiers modifiers;
      modifiers.word = true;
      modifiers.shift = shift;
      PressKey(key, modifiers);
    }

    void Shifted(const Key key)
    {
      KeyModifiers modifiers;
      modifiers.shift = true;
      PressKey(key, modifiers);
    }

    /// Clicks into the field before the character at `index`.
    void ClickBefore(const int index, const int times = 1)
    {
      for (int i = 0; i < times; i++)
      {
        ClickAt(text_left + glyph * static_cast<float>(index) + 1.0f, 15.0);
      }
    }

    [[nodiscard]] std::vector<std::string> Happened() const
    {
      std::vector<std::string> happened;
      for (const auto &event : _ui->GetElementEvents())
      {
        // every key is an event of its own, which is not what is asked
        if (event.name == "key_down" || event.name == "key_up") { continue; }

        happened.push_back(event.name + " " + event.target_name + (event.value.empty() ? "" : " " + event.value));
      }
      return happened;
    }

    /// The rectangles of the frame that are not textured: the boxes, the
    /// selection, and the caret.
    [[nodiscard]] std::vector<neon::testing::RecordedQuad> Boxes() const
    {
      std::vector<neon::testing::RecordedQuad> boxes;
      for (const auto &quad : _renderer.Quads())
      {
        if (!quad.textured) { boxes.push_back(quad); }
      }
      return boxes;
    }

    /// Whether a box of the width of the caret stands at `x` of the field.
    [[nodiscard]] bool HasCaretAt(const float x) const
    {
      for (const auto &box : Boxes())
      {
        if (std::abs(box.Width() - 1.0f) < 0.01f && std::abs(box.left - x) < 0.01f && box.Height() > 10.0f)
        {
          return true;
        }
      }
      return false;
    }

    /// How many characters are drawn in the field: the ones in its top
    /// 30 units, which leaves out the button under it.
    [[nodiscard]] int CountOfGlyphs() const
    {
      int count = 0;
      for (const auto &quad : _renderer.Quads())
      {
        if (quad.textured && quad.top < 30.0f) { count++; }
      }
      return count;
    }
  };

  // typing

  TEST_F(UiTextInputTest, IsThirtyHighAndAsWideAsItIsToldPlusItsPaddingAndBorder)
  {
    ShowInput();
    ExpectBox("field", 0.0f, 0.0f, 222.0f, 30.0f);
  }

  TEST_F(UiTextInputTest, TakesTheFocusWhenItIsClickedAndShowsWhatIsTyped)
  {
    ShowInput();

    ClickBefore(0);
    EXPECT_EQ(_ui->GetFocused(), "field");

    Type("Hi");
    EXPECT_EQ(Value(), "Hi");
    EXPECT_THAT(Happened(), ElementsAre("changed field Hi"));
    EXPECT_EQ(CountOfGlyphs(), 2);
  }

  TEST_F(UiTextInputTest, DoesNotTakeTextWhileNothingHasTheFocus)
  {
    ShowInput();

    Type("Hi");
    EXPECT_EQ(Value(), "");
    EXPECT_THAT(Happened(), IsEmpty());
  }

  TEST_F(UiTextInputTest, ShowsItsValueFromTheFile)
  {
    ShowInput("value: Player One\n");
    EXPECT_EQ(Value(), "Player One");
    EXPECT_EQ(CountOfGlyphs(), 9) << "the space is not drawn";
  }

  TEST_F(UiTextInputTest, ShowsTheCaretAtTheEndAfterTyping)
  {
    ShowInput();
    ClickBefore(0);
    Type("abc");

    EXPECT_TRUE(HasCaretAt(text_left + 3 * glyph));
  }

  TEST_F(UiTextInputTest, TheCaretBlinksTwiceASecondWhileItHasTheFocus)
  {
    ShowInput();
    ClickBefore(0);
    EXPECT_TRUE(HasCaretAt(text_left));

    for (int i = 0; i < 31; i++)
    {
      _ui->Advance(1.0 / 60.0);
      Frame();
    }
    EXPECT_FALSE(HasCaretAt(text_left)) << "after half a second";

    for (int i = 0; i < 30; i++)
    {
      _ui->Advance(1.0 / 60.0);
      Frame();
    }
    EXPECT_TRUE(HasCaretAt(text_left)) << "after a second";
  }

  TEST_F(UiTextInputTest, HasNoCaretWithoutTheFocus)
  {
    ShowInput("value: abc\n");
    EXPECT_FALSE(HasCaretAt(text_left + 3 * glyph));

    ClickBefore(3);
    EXPECT_TRUE(HasCaretAt(text_left + 3 * glyph));

    ClickAt(100.0, 300.0);
    EXPECT_EQ(_ui->GetFocused(), "");
    EXPECT_FALSE(HasCaretAt(text_left + 3 * glyph));
  }

  // the caret

  TEST_F(UiTextInputTest, AClickPutsTheCaretBetweenCharacters)
  {
    ShowInput("value: hello\n");

    ClickBefore(2);
    EXPECT_EQ(Editor().GetCaret(), 2u);
    EXPECT_TRUE(HasCaretAt(text_left + 2 * glyph));

    // behind the text
    ClickAt(150.0, 15.0);
    EXPECT_EQ(Editor().GetCaret(), 5u);

    // the nearer side of a character
    ClickAt(text_left + 2 * glyph + 6.0f, 15.0);
    EXPECT_EQ(Editor().GetCaret(), 3u);
  }

  TEST_F(UiTextInputTest, TheCaretMovesByCharactersOfSeveralBytes)
  {
    // e with an accent (2 bytes), and a character of Japanese (3 bytes)
    ShowInput("value: \"\xC3\xA9\xE6\x97\xA5x\"\n");
    ClickBefore(0);

    PressKey(Key::Right);
    EXPECT_EQ(Editor().GetCaret(), 2u);
    EXPECT_TRUE(HasCaretAt(text_left + glyph));

    PressKey(Key::Right);
    EXPECT_EQ(Editor().GetCaret(), 5u);

    PressKey(Key::Right);
    EXPECT_EQ(Editor().GetCaret(), 6u);

    PressKey(Key::Right);
    EXPECT_EQ(Editor().GetCaret(), 6u) << "at the end";

    PressKey(Key::Left);
    PressKey(Key::Left);
    EXPECT_EQ(Editor().GetCaret(), 2u);
  }

  TEST_F(UiTextInputTest, ACombiningMarkStaysWithItsCharacter)
  {
    // e with a combining acute accent (e + 2 bytes), then x
    ShowInput("value: \"e\xCC\x81x\"\n");
    ClickBefore(0);

    PressKey(Key::Right);
    EXPECT_EQ(Editor().GetCaret(), 3u);

    PressKey(Key::Backspace);
    EXPECT_EQ(Value(), "x");
  }

  TEST_F(UiTextInputTest, HomeAndEndGoToTheEnds)
  {
    ShowInput("value: hello\n");
    ClickBefore(2);

    PressKey(Key::End);
    EXPECT_EQ(Editor().GetCaret(), 5u);

    PressKey(Key::Home);
    EXPECT_EQ(Editor().GetCaret(), 0u);

    // up and down do the same in a text of one line
    PressKey(Key::Down);
    EXPECT_EQ(Editor().GetCaret(), 5u);

    PressKey(Key::Up);
    EXPECT_EQ(Editor().GetCaret(), 0u);
  }

  TEST_F(UiTextInputTest, TheWordKeyMovesByWords)
  {
    ShowInput("value: one two  three\n");
    ClickBefore(0);

    WordKey(Key::Right);
    EXPECT_EQ(Editor().GetCaret(), 3u);

    WordKey(Key::Right);
    EXPECT_EQ(Editor().GetCaret(), 7u);

    WordKey(Key::Right);
    EXPECT_EQ(Editor().GetCaret(), 14u);

    WordKey(Key::Left);
    EXPECT_EQ(Editor().GetCaret(), 9u);

    WordKey(Key::Left);
    EXPECT_EQ(Editor().GetCaret(), 4u);
  }

  // editing

  TEST_F(UiTextInputTest, BackspaceAndDeleteTakeOneCharacterEachSide)
  {
    ShowInput("value: abcd\n");
    ClickBefore(2);

    PressKey(Key::Backspace);
    EXPECT_EQ(Value(), "acd");
    EXPECT_EQ(Editor().GetCaret(), 1u);

    PressKey(Key::Delete);
    EXPECT_EQ(Value(), "ad");
    EXPECT_EQ(Editor().GetCaret(), 1u);

    EXPECT_THAT(Happened(), ElementsAre("changed field ad"));
  }

  TEST_F(UiTextInputTest, TheWordKeyDeletesAWord)
  {
    ShowInput("value: one two three\n");
    ClickBefore(7);

    WordKey(Key::Backspace);
    EXPECT_EQ(Value(), "one  three");

    WordKey(Key::Delete);
    EXPECT_EQ(Value(), "one ");
  }

  TEST_F(UiTextInputTest, TypingReplacesTheSelection)
  {
    ShowInput("value: hello world\n");
    ClickBefore(0);
    Shifted(Key::Right);
    Shifted(Key::Right);
    Shifted(Key::Right);
    Shifted(Key::Right);
    Shifted(Key::Right);
    EXPECT_EQ(Editor().GetSelectedText(), "hello");

    Type("bye");
    EXPECT_EQ(Value(), "bye world");
    EXPECT_FALSE(Editor().HasSelection());
  }

  TEST_F(UiTextInputTest, EnterSubmits)
  {
    ShowInput("value: name\n");
    ClickBefore(4);

    PressKey(Key::Enter);
    EXPECT_THAT(Happened(), ElementsAre("submitted field name"));
    EXPECT_EQ(Value(), "name") << "nothing is inserted";
  }

  TEST_F(UiTextInputTest, HoldsNoMoreThanItsMaxLength)
  {
    ShowInput("max_length: 3\n");
    ClickBefore(0);

    Type("abcd");
    EXPECT_EQ(Value(), "abc");

    // characters, and not bytes
    Shortcut(Key::A);
    Type("\xC3\xA9\xC3\xA9\xC3\xA9\xC3\xA9");
    EXPECT_EQ(Value(), "\xC3\xA9\xC3\xA9\xC3\xA9");
  }

  TEST_F(UiTextInputTest, ANumberTakesOnlyDigits)
  {
    ShowInput("kind: number\n");
    ClickBefore(0);

    Type("-1a2.5e");
    EXPECT_EQ(Value(), "-12.5");
  }

  TEST_F(UiTextInputTest, APasswordShowsDotsAndHoldsTheText)
  {
    ShowInput("kind: password\n");
    ClickBefore(0);

    Type("abc");
    EXPECT_EQ(Value(), "abc");

    std::size_t caret = 0;
    std::size_t start = 0;
    std::size_t end = 0;
    EXPECT_EQ(Editor().GetShownText(caret, start, end), "\xE2\x80\xA2\xE2\x80\xA2\xE2\x80\xA2");
    EXPECT_TRUE(HasCaretAt(text_left + 3 * glyph));
  }

  TEST_F(UiTextInputTest, WhatIsReadOnlyCanBeSelectedAndNotChanged)
  {
    ShowInput("value: fixed\nread_only: true\n");
    ClickBefore(0);
    EXPECT_EQ(_ui->GetFocused(), "field");

    Type("x");
    PressKey(Key::Backspace);
    EXPECT_EQ(Value(), "fixed");
    EXPECT_THAT(Happened(), IsEmpty());

    Shortcut(Key::A);
    EXPECT_EQ(Editor().GetSelectedText(), "fixed");

    // and there is no caret, since nothing can be typed
    EXPECT_FALSE(HasCaretAt(text_left + 5 * glyph));
  }

  TEST_F(UiTextInputTest, WhatIsNotEnabledCannotBeTypedInto)
  {
    ShowInput("value: fixed\nenabled: \"{can_type}\"\n");
    _ui->SetFlag("can_type", false);
    Frame();

    ClickBefore(0);
    EXPECT_EQ(_ui->GetFocused(), "");

    _ui->SetFlag("can_type", true);
    Frame();

    ClickBefore(0);
    EXPECT_EQ(_ui->GetFocused(), "field");
  }

  // selecting

  TEST_F(UiTextInputTest, ShiftAndTheArrowsSelect)
  {
    ShowInput("value: hello\n");
    ClickBefore(1);

    Shifted(Key::Right);
    Shifted(Key::Right);
    EXPECT_EQ(Editor().GetSelectedText(), "el");

    Shifted(Key::End);
    EXPECT_EQ(Editor().GetSelectedText(), "ello");

    Shifted(Key::Home);
    EXPECT_EQ(Editor().GetSelectedText(), "h");

    PressKey(Key::Right);
    EXPECT_FALSE(Editor().HasSelection());
    EXPECT_EQ(Editor().GetCaret(), 1u) << "to the end of the selection";
  }

  TEST_F(UiTextInputTest, TheSelectionIsDrawnBehindTheText)
  {
    ShowInput("value: hello\n");
    ClickBefore(1);
    Shifted(Key::Right);
    Shifted(Key::Right);

    bool found = false;
    for (const auto &box : Boxes())
    {
      if (std::abs(box.left - (text_left + glyph)) < 0.01f && std::abs(box.Width() - 2 * glyph) < 0.01f)
      {
        found = true;
        EXPECT_NEAR(box.top, 7.0f, 0.01f);
        EXPECT_NEAR(box.Height(), 16.0f, 0.01f);
      }
    }
    EXPECT_TRUE(found);
  }

  TEST_F(UiTextInputTest, ADoubleClickSelectsAWordAndATripleClickTheLine)
  {
    ShowInput("value: one two three\n");

    ClickBefore(5, 2);
    EXPECT_EQ(Editor().GetSelectedText(), "two");

    ClickBefore(5, 3);
    EXPECT_EQ(Editor().GetSelectedText(), "one two three");
  }

  TEST_F(UiTextInputTest, DraggingSelects)
  {
    ShowInput("value: hello world\n");

    Release();
    PointAt(text_left + 1.0f, 15.0);
    _input.state.SetAction(Action::Pointer_Primary);
    Frame();

    PointAt(text_left + 5 * glyph + 1.0f, 15.0);
    Frame();
    EXPECT_EQ(Editor().GetSelectedText(), "hello");

    // beyond the box: the rest is selected
    PointAt(300.0, 15.0);
    Frame();
    EXPECT_EQ(Editor().GetSelectedText(), "hello world");

    Release();
    Frame();

    // and letting go keeps it
    PointAt(text_left + 1.0f, 15.0);
    Frame();
    EXPECT_EQ(Editor().GetSelectedText(), "hello world");
  }

  TEST_F(UiTextInputTest, ShiftAndAClickSelectFromTheCaret)
  {
    ShowInput("value: hello world\n");
    ClickBefore(2);

    Release();
    _input.state.SetAction(Action::Pointer_Primary);
    _input.state.AddKeyEvent({Key::Unknown, true, false, {.shift = true}});
    PointAt(text_left + 7 * glyph + 1.0f, 15.0);
    Frame();
    Release();
    Frame();

    EXPECT_EQ(Editor().GetSelectedText(), "llo w");
  }

  // the clipboard

  TEST_F(UiTextInputTest, CopiesAndCutsWhatIsSelectedAndPastesIt)
  {
    ShowInput("value: hello world\n");
    ClickBefore(0);
    Shifted(Key::Right);
    Shifted(Key::Right);
    Shifted(Key::Right);
    Shifted(Key::Right);
    Shifted(Key::Right);

    Shortcut(Key::C);
    EXPECT_EQ(_clipboard.GetText(), "hello");
    EXPECT_EQ(Value(), "hello world");

    Shortcut(Key::X);
    EXPECT_EQ(Value(), " world");

    PressKey(Key::End);
    Shortcut(Key::V);
    EXPECT_EQ(Value(), " worldhello");
    EXPECT_THAT(Happened(), ElementsAre("changed field  worldhello"));
  }

  TEST_F(UiTextInputTest, PastesTextOfSeveralBytesAndNoLineBreaksIntoAnInput)
  {
    ShowInput();
    ClickBefore(0);

    // the line break is left out, as HTML does
    _clipboard.SetText("\xE6\x97\xA5\xE6\x9C\xAC\nline");
    Shortcut(Key::V);
    EXPECT_EQ(Value(), "\xE6\x97\xA5\xE6\x9C\xACline");
  }

  TEST_F(UiTextInputTest, TheShortcutsNeedTheShortcutKey)
  {
    ShowInput("value: hello\n");
    ClickBefore(5);

    PressKey(Key::A);
    EXPECT_FALSE(Editor().HasSelection());

    _clipboard.SetText("no");
    PressKey(Key::V);
    EXPECT_EQ(Value(), "hello");
  }

  // undo

  TEST_F(UiTextInputTest, UndoesAndRedoesWhatWasTyped)
  {
    ShowInput();
    ClickBefore(0);

    Type("one");
    Type(" ");
    Type("two");

    // a word is one step, and so is the space that ends it
    Shortcut(Key::Z);
    EXPECT_EQ(Value(), "one ");

    Shortcut(Key::Z);
    EXPECT_EQ(Value(), "one");

    Shortcut(Key::Z);
    EXPECT_EQ(Value(), "");

    Shortcut(Key::Z, true);
    EXPECT_EQ(Value(), "one");

    Shortcut(Key::Y);
    Shortcut(Key::Y);
    EXPECT_EQ(Value(), "one two");
    EXPECT_THAT(Happened(), ElementsAre("changed field one two"));
  }

  TEST_F(UiTextInputTest, UndoBringsTheSelectionBackWithTheText)
  {
    ShowInput("value: hello world\n");
    ClickBefore(0);
    Shifted(Key::Right);
    Shifted(Key::Right);
    Shifted(Key::Right);
    Shifted(Key::Right);
    Shifted(Key::Right);

    PressKey(Key::Delete);
    EXPECT_EQ(Value(), " world");

    Shortcut(Key::Z);
    EXPECT_EQ(Value(), "hello world");
    EXPECT_EQ(Editor().GetSelectedText(), "hello");
  }

  // the input method

  TEST_F(UiTextInputTest, ShowsWhatTheInputMethodPutsTogetherUntilItIsDone)
  {
    ShowInput("value: ab\n");
    ClickBefore(1);

    Release();
    _input.state.SetComposition({"ni", 2});
    Frame();
    Release();

    std::size_t caret = 0;
    std::size_t start = 0;
    std::size_t end = 0;
    EXPECT_EQ(Editor().GetShownText(caret, start, end), "anib");
    EXPECT_EQ(caret, 3u);
    EXPECT_EQ(Value(), "ab") << "what is put together is not in the text yet";
    EXPECT_EQ(CountOfGlyphs(), 4);
    EXPECT_THAT(Happened(), IsEmpty());

    // an underline, one pixel high, under what is put together
    bool underlined = false;
    for (const auto &box : Boxes())
    {
      if (std::abs(box.left - (text_left + glyph)) < 0.01f && std::abs(box.Width() - 2 * glyph) < 0.01f &&
          std::abs(box.Height() - 1.0f) < 0.01f)
      {
        underlined = true;
      }
    }
    EXPECT_TRUE(underlined);

    // done: the text arrives, and the composition is empty
    Release();
    _input.state.SetComposition({});
    _input.state.AddText("\xE3\x81\xAB");
    Frame();
    Release();

    EXPECT_EQ(Value(), "a\xE3\x81\xAB" "b");
    EXPECT_EQ(Editor().GetShownText(caret, start, end), "a\xE3\x81\xAB" "b");
    EXPECT_THAT(Happened(), ElementsAre("changed field a\xE3\x81\xAB" "b"));
  }

  TEST_F(UiTextInputTest, TellsThePlatformWhereTheCaretIsWhileATextIsTyped)
  {
    ShowInput("value: abc\n");
    EXPECT_FALSE(_input.is_text_input_started);

    ClickBefore(3);
    EXPECT_TRUE(_input.is_text_input_started);

    const TextInputArea area = _input.text_input_area;
    EXPECT_EQ(area.x, static_cast<int>(text_left + 3 * glyph));
    EXPECT_EQ(area.y, 7);
    EXPECT_EQ(area.height, 16);

    ClickAt(100.0, 300.0);
    EXPECT_FALSE(_input.is_text_input_started);
  }

  // the game

  TEST_F(UiTextInputTest, FollowsAValueOfTheGameAndWritesBackWhatIsTyped)
  {
    ShowInput("value: \"{player_name}\"\n");
    EXPECT_EQ(Value(), "");

    _ui->SetText("player_name", "Ada");
    Frame();
    EXPECT_EQ(Value(), "Ada");

    ClickBefore(3);
    Type("m");
    EXPECT_EQ(Value(), "Adam");
    EXPECT_EQ(_ui->GetValue("player_name", nullptr), "Adam");

    // the game sets it again
    _ui->SetText("player_name", "Eve");
    Frame();
    EXPECT_EQ(Value(), "Eve");
  }

  TEST_F(UiTextInputTest, ANumberFollowsANumberOfTheGame)
  {
    ShowInput("kind: number\nvalue: \"{volume}\"\n");
    _ui->SetNumber("volume", 75);
    Frame();
    EXPECT_EQ(Value(), "75");

    ClickBefore(2);
    PressKey(Key::Backspace);
    EXPECT_EQ(Value(), "7");
    EXPECT_EQ(_ui->GetValue("volume", nullptr), "7");
  }

  TEST_F(UiTextInputTest, TheGameDoesNotSeeTheKeysWhileATextIsTyped)
  {
    ShowInput();
    ClickBefore(0);

    Release();
    _input.state.SetKeyboardAction(Action::Ui_Left);
    _input.state.SetKeyboardAction(Action::Ui_Cancel);
    _input.state.SetKeyboardAction(Action::L_Up);
    _input.state.AddText("x");
    _input.state.AddKeyEvent({Key::Left, true, false, {}});
    Frame();

    const neon::InputState &game = _ui->GetGameInput()->GetInputState();
    EXPECT_FALSE(game[Action::Ui_Left]);
    EXPECT_FALSE(game[Action::Ui_Cancel]);
    EXPECT_FALSE(game[Action::L_Up]) << "the keys type, and do not move the game";
    EXPECT_TRUE(game.GetText().empty());
    EXPECT_TRUE(game.GetKeyEvents().empty());
    EXPECT_EQ(Value(), "x");
  }

  TEST_F(UiTextInputTest, AControllerStillMovesTheGameWhileATextIsTyped)
  {
    ShowInput();
    ClickBefore(0);

    Release();
    _input.state.SetAction(Action::L_Up);
    Frame();

    EXPECT_TRUE(_ui->GetGameInput()->GetInputState()[Action::L_Up]);
  }

  TEST_F(UiTextInputTest, TabLeavesTheFieldAndEscapeCancels)
  {
    ShowInput("value: abc\n");
    ClickBefore(3);

    PressKey(Key::Tab);
    EXPECT_EQ(_ui->GetFocused(), "ok");

    Shifted(Key::Tab);
    EXPECT_EQ(_ui->GetFocused(), "field");

    // backspace is cancel to the keys of the interface, and deletes here:
    // escape is what cancels
    Release();
    _input.state.SetKeyboardAction(Action::Ui_Cancel);
    _input.state.AddKeyEvent({Key::Backspace, true, false, {}});
    Frame();
    Release();
    Frame();
    EXPECT_EQ(_ui->GetFocused(), "field");
    EXPECT_EQ(Value(), "ab");

    PressKey(Key::Escape);
    EXPECT_EQ(_ui->GetFocused(), "") << "escape takes the focus away";
  }

  TEST_F(UiTextInputTest, ScrollsSidewaysToKeepTheCaretInView)
  {
    // an input is at least 160 wide unless it is told otherwise
    ShowInput("min_width: 0\n", "", "78");
    ClickBefore(0);

    // 78 wide inside, which is 9 characters
    Type("abcdefghijkl");
    EXPECT_EQ(Value(), "abcdefghijkl");

    // the caret stands at the right edge
    EXPECT_TRUE(HasCaretAt(text_left + 78.0f));

    PressKey(Key::Home);
    EXPECT_TRUE(HasCaretAt(text_left));

    // what is drawn is cut off at the box
    for (const auto &quad : _renderer.Quads())
    {
      if (quad.textured && quad.top < 30.0f) { EXPECT_TRUE(quad.clipped); }
    }
  }

  // validation

  TEST_F(UiTextInputTest, IsInvalidWhenTheTextDoesNotFitThePattern)
  {
    WriteAsset("ui/theme.css", "input:invalid { border-color: #ff0000; }\n");
    ShowInput("pattern: \"[a-z]*@[a-z]*.com\"\n", "styles: [theme.css]\n");

    FieldValue valid;
    ASSERT_TRUE(_ui->GetField(Field(), "valid", valid));
    EXPECT_TRUE(std::get<bool>(valid)) << "an empty text fits";

    ClickBefore(0);
    Type("ada@");
    ASSERT_TRUE(_ui->GetField(Field(), "valid", valid));
    EXPECT_FALSE(std::get<bool>(valid));
    EXPECT_EQ(_ui->GetComputed(Field(), "border_color"), "rgb(255, 0, 0)");

    Type("home.com");
    ASSERT_TRUE(_ui->GetField(Field(), "valid", valid));
    EXPECT_TRUE(std::get<bool>(valid));
    EXPECT_NE(_ui->GetComputed(Field(), "border_color"), "rgb(255, 0, 0)");
  }

  // the placeholder

  TEST_F(UiTextInputTest, ShowsThePlaceholderWhileItIsEmpty)
  {
    ShowInput("placeholder: Name\n");
    EXPECT_EQ(CountOfGlyphs(), 4);

    ClickBefore(0);
    Type("A");
    EXPECT_EQ(CountOfGlyphs(), 1);

    PressKey(Key::Backspace);
    EXPECT_EQ(CountOfGlyphs(), 4);
  }

  TEST_F(UiTextInputTest, ThePlaceholderIsStyledAsAPart)
  {
    WriteAsset("ui/theme.css", "input::placeholder { color: #00ff00; }\n");
    ShowInput("placeholder: Name\n", "styles: [theme.css]\n");

    bool green = false;
    for (const auto &quad : _renderer.Quads())
    {
      if (quad.textured && quad.color.g > 0.99f && quad.color.r < 0.01f) { green = true; }
    }
    EXPECT_TRUE(green);
  }

  // the fields

  TEST_F(UiTextInputTest, HasFieldsAScriptSets)
  {
    ShowInput();

    EXPECT_TRUE(_ui->SetField(Field(), "value", std::string("set")));
    EXPECT_TRUE(_ui->SetField(Field(), "max_length", 5));
    EXPECT_TRUE(_ui->SetField(Field(), "read_only", true));
    EXPECT_TRUE(_ui->SetField(Field(), "kind", std::string("password")));
    EXPECT_EQ(Value(), "set");
    EXPECT_THAT(_logger->Messages(LogLevel::Warn), IsEmpty());

    EXPECT_FALSE(_ui->SetField(Field(), "kind", std::string("date")));
    EXPECT_FALSE(_ui->SetField(Field(), "max_length", -1));
    EXPECT_FALSE(_ui->SetField(Field(), "valid", true));

    std::vector<std::string> warnings;
    for (const auto &[level, message] : _logger->Entries())
    {
      if (level == LogLevel::Warn) { warnings.push_back(message); }
    }

    EXPECT_THAT(warnings, ElementsAre(
      "'kind' of input 'field' is 'date', where one of these was expected: text, password, number. "
      "Nothing is set",
      "'max_length' of input 'field' has to be at least 0. Nothing is set",
      "'valid' of input 'field' follows from the text and its pattern, and cannot be set. Nothing is set"));
  }

  TEST_F(UiTextInputTest, SaysWhatIsWrongInTheFile)
  {
    ExpectProblemsUnderRoot(
      "- {type: input, name: a, kind: date}\n"
      "- {type: input, name: b, max_length: -2}\n"
      "- {type: textarea, name: c, rows: 0}\n"
      "- {type: input, name: d, value: [1]}\n",
      {
        "assets://ui/test.ui.yml:4: 'kind' of input 'a' is 'date', where one of these was expected: "
        "text, password, number",
        "assets://ui/test.ui.yml:5: 'max_length' of input 'b' is -2, where a whole number that is not below 0 "
        "was expected",
        "assets://ui/test.ui.yml:6: 'rows' of textarea 'c' is 0, where a whole number above 0 was expected",
        "assets://ui/test.ui.yml:7: 'value' of input 'd' is a list, where text or a value such as "
        "\"{player_name}\" was expected"
      });
  }

  // a textarea

  TEST_F(UiTextInputTest, ATextAreaIsAsHighAsItsRowsAndEnterBreaksTheLine)
  {
    ShowTextArea("rows: 3\n");
    ExpectBox("field", 0.0f, 0.0f, 262.0f, 3 * 16.0f + 14.0f);

    ClickBefore(0);
    Type("one");
    PressKey(Key::Enter);
    Type("two");
    EXPECT_EQ(Value(), "one\ntwo");
    EXPECT_THAT(Happened(), ElementsAre("changed field one\ntwo"));

    // the caret is on the second line
    EXPECT_TRUE(HasCaretAt(text_left + 3 * glyph));
    bool on_second_line = false;
    for (const auto &box : Boxes())
    {
      if (std::abs(box.Width() - 1.0f) < 0.01f && std::abs(box.top - (7.0f + 16.0f)) < 0.01f)
      {
        on_second_line = true;
      }
    }
    EXPECT_TRUE(on_second_line);
  }

  TEST_F(UiTextInputTest, UpAndDownMoveBetweenTheLinesOfATextArea)
  {
    ShowTextArea("value: \"first line\\nsecond\\nthird one\"\n");
    ClickBefore(0);

    PressKey(Key::Right);
    PressKey(Key::Right);
    PressKey(Key::Right);
    PressKey(Key::Right);
    PressKey(Key::Right);
    PressKey(Key::Right);
    PressKey(Key::Right);
    PressKey(Key::Right);
    EXPECT_EQ(Editor().GetCaret(), 8u);

    PressKey(Key::Down);
    EXPECT_EQ(Editor().GetCaret(), 17u) << "the end of a shorter line";

    PressKey(Key::Down);
    EXPECT_EQ(Editor().GetCaret(), 26u);

    // back where it was, since the caret remembers where it wants to be
    PressKey(Key::Up);
    PressKey(Key::Up);
    EXPECT_EQ(Editor().GetCaret(), 8u);

    PressKey(Key::Up);
    EXPECT_EQ(Editor().GetCaret(), 0u) << "the start, from the first line";

    // home and end are of the line
    PressKey(Key::Down);
    PressKey(Key::End);
    EXPECT_EQ(Editor().GetCaret(), 17u);
    PressKey(Key::Home);
    EXPECT_EQ(Editor().GetCaret(), 11u);
  }

  TEST_F(UiTextInputTest, ATextAreaWrapsLongLinesAndScrollsToTheCaret)
  {
    ShowTextArea("rows: 2\nmin_width: 0\n", "176");
    ClickBefore(0);

    // 176 wide inside is 22 characters a line
    for (int i = 0; i < 5; i++) { Type("aaaaaaaaaa"); }
    EXPECT_EQ(Value().size(), 50u);

    // three lines, of which two are seen, scrolled to the last
    const auto &field = Element("field");

    EXPECT_NEAR(field.GetScrollY(), 16.0f, 0.01f);

    PressKey(Key::Up);
    PressKey(Key::Up);
    EXPECT_NEAR(field.GetScrollY(), 0.0f, 0.01f);
  }

  TEST_F(UiTextInputTest, TheShortcutAndEnterSubmitATextArea)
  {
    ShowTextArea("value: note\n");
    ClickBefore(4);

    Shortcut(Key::Enter);
    EXPECT_THAT(Happened(), ElementsAre("submitted field note"));
    EXPECT_EQ(Value(), "note");
  }

  TEST_F(UiTextInputTest, PastesLineBreaksIntoATextArea)
  {
    ShowTextArea();
    ClickBefore(0);

    _clipboard.SetText("one\r\ntwo");
    Shortcut(Key::V);
    EXPECT_EQ(Value(), "one\ntwo");
  }

  // the kinds

  TEST_F(UiTextInputTest, DescribesItself)
  {
    ShowInput();

    std::vector<std::string> names;
    for (const auto &field : Element("field").GetFields()) { names.push_back(field.name); }

    EXPECT_THAT(names, ElementsAre(
      "kind", "value", "placeholder", "max_length", "read_only", "enabled", "pattern", "valid", "autofocus"));
  }
}
