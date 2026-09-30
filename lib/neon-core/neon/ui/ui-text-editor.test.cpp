#include "ui-text-editor.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

// Text is UTF-8 and places in it are bytes. The texts of these tests are
// written with the bytes of their characters:
//
//     e with an acute, as one character      C3 A9        2 bytes
//     e and a combining acute                65 CC 81     3 bytes
//     the euro sign                          E2 82 AC     3 bytes
//     a grinning face                        F0 9F 98 80  4 bytes

namespace
{
  using neon::Memory_Clipboard;
  using neon::UiTextEditor;
  using ::testing::ElementsAre;

  namespace Boundaries = neon::UiTextBoundaries;

  const std::string e_acute = "\xC3\xA9";
  const std::string e_combining = "e\xCC\x81";
  const std::string euro = "\xE2\x82\xAC";
  const std::string face = "\xF0\x9F\x98\x80";

  // a thumb with a tone of skin: 1F44D 1F3FD
  const std::string thumb = "\xF0\x9F\x91\x8D\xF0\x9F\x8F\xBD";

  // a family: a man, a joiner, a woman, a joiner, a girl
  const std::string family =
    "\xF0\x9F\x91\xA8\xE2\x80\x8D\xF0\x9F\x91\xA9\xE2\x80\x8D\xF0\x9F\x91\xA7";

  // the flag of Germany: two regional indicators
  const std::string flag = "\xF0\x9F\x87\xA9\xF0\x9F\x87\xAA";

  /// Every place a caret may stand at, from the start of a text to its
  /// end.
  std::vector<std::size_t> Stops(const std::string &text)
  {
    std::vector<std::size_t> stops{0};
    for (std::size_t at = 0; at < text.size();)
    {
      at = Boundaries::Next(text, at);
      stops.push_back(at);
    }
    return stops;
  }

  // where a caret may stand

  TEST(UiTextBoundariesTest, StepsOverOneLetterOfBasicLatin)
  {
    EXPECT_THAT(Stops("abc"), ElementsAre(0u, 1u, 2u, 3u));
    EXPECT_THAT(Stops(""), ElementsAre(0u));
  }

  TEST(UiTextBoundariesTest, NeverStopsInsideTheBytesOfACharacter)
  {
    EXPECT_THAT(Stops("a" + e_acute + "b"), ElementsAre(0u, 1u, 3u, 4u));
    EXPECT_THAT(Stops(euro + "5"), ElementsAre(0u, 3u, 4u));
    EXPECT_THAT(Stops(face + face), ElementsAre(0u, 4u, 8u));
    EXPECT_THAT(Stops("Zo" + e_acute), ElementsAre(0u, 1u, 2u, 4u));
  }

  TEST(UiTextBoundariesTest, NeverStopsBetweenALetterAndWhatIsPutOnTopOfIt)
  {
    EXPECT_THAT(Stops("a" + e_combining + "b"), ElementsAre(0u, 1u, 4u, 5u));

    // two marks on one letter
    EXPECT_THAT(Stops("e\xCC\x81\xCC\xA3z"), ElementsAre(0u, 5u, 6u));
  }

  TEST(UiTextBoundariesTest, TakesAnEmojiWithItsModifiersAndJoinersForOne)
  {
    EXPECT_THAT(Stops(thumb), ElementsAre(0u, 8u));
    EXPECT_THAT(Stops(family), ElementsAre(0u, family.size()));
    EXPECT_THAT(Stops("a" + family + "b"), ElementsAre(0u, 1u, 1u + family.size(), 2u + family.size()));

    // a heart with a variation selector: 2764 FE0F
    EXPECT_THAT(Stops("\xE2\x9D\xA4\xEF\xB8\x8F!"), ElementsAre(0u, 6u, 7u));
  }

  TEST(UiTextBoundariesTest, TakesTwoRegionalIndicatorsForOneFlag)
  {
    EXPECT_THAT(Stops(flag), ElementsAre(0u, 8u));
    EXPECT_THAT(Stops(flag + flag), ElementsAre(0u, 8u, 16u));
  }

  TEST(UiTextBoundariesTest, TakesACarriageReturnWithItsLineFeedForOne)
  {
    EXPECT_THAT(Stops("a\r\nb"), ElementsAre(0u, 1u, 3u, 4u));
    EXPECT_THAT(Stops("a\nb"), ElementsAre(0u, 1u, 2u, 3u));
  }

  TEST(UiTextBoundariesTest, StepsOverBytesThatAreNoCharacterOneAtATime)
  {
    EXPECT_THAT(Stops("a\xFF\x80" "b"), ElementsAre(0u, 1u, 2u, 3u, 4u));

    // a character that is cut off at the end of the text
    EXPECT_THAT(Stops("a\xE2\x82"), ElementsAre(0u, 1u, 2u, 3u));
  }

  TEST(UiTextBoundariesTest, GoesBackTheWayItCame)
  {
    for (const std::string &text : {
           std::string("abc"), "a" + e_acute + "b", "a" + e_combining + "b", thumb + family + flag,
           "a\r\nb" + euro, std::string("")
         })
    {
      const auto stops = Stops(text);

      for (std::size_t i = stops.size(); i > 1; i--)
      {
        EXPECT_EQ(Boundaries::Previous(text, stops[i - 1]), stops[i - 2]) << text << " at " << stops[i - 1];
      }

      EXPECT_EQ(Boundaries::Previous(text, 0), 0u);
    }
  }

  TEST(UiTextBoundariesTest, GoesToTheStartOfWhatAPlaceIsInsideOf)
  {
    const std::string text = "a" + e_combining + euro;

    // inside the e with its accent, which is at 1 to 4
    EXPECT_EQ(Boundaries::Previous(text, 2), 1u);
    EXPECT_EQ(Boundaries::Previous(text, 3), 1u);
    EXPECT_EQ(Boundaries::Next(text, 2), 4u);
    EXPECT_EQ(Boundaries::Next(text, 3), 4u);

    // inside the euro sign, which is at 4 to 7
    EXPECT_EQ(Boundaries::Next(text, 5), 7u);
    EXPECT_EQ(Boundaries::Previous(text, 6), 4u);
  }

  TEST(UiTextBoundariesTest, MovesAPlaceToWhereACaretMayStand)
  {
    const std::string text = "a" + e_combining + euro;

    EXPECT_EQ(Boundaries::Snap(text, 0), 0u);
    EXPECT_EQ(Boundaries::Snap(text, 1), 1u);
    EXPECT_EQ(Boundaries::Snap(text, 2), 1u);
    EXPECT_EQ(Boundaries::Snap(text, 3), 1u);
    EXPECT_EQ(Boundaries::Snap(text, 4), 4u);
    EXPECT_EQ(Boundaries::Snap(text, 5), 4u);
    EXPECT_EQ(Boundaries::Snap(text, 6), 4u);
    EXPECT_EQ(Boundaries::Snap(text, 7), 7u);
    EXPECT_EQ(Boundaries::Snap(text, 99), 7u);
  }

  TEST(UiTextBoundariesTest, CountsWhatACaretStepsOver)
  {
    EXPECT_EQ(Boundaries::Count(""), 0u);
    EXPECT_EQ(Boundaries::Count("abc"), 3u);
    EXPECT_EQ(Boundaries::Count("Zo" + e_acute), 3u);
    EXPECT_EQ(Boundaries::Count("Zo" + e_combining), 3u);
    EXPECT_EQ(Boundaries::Count(family + flag + thumb), 3u);
  }

  TEST(UiTextBoundariesTest, MovesByWords)
  {
    //                        0123456789012345678
    const std::string text = "one two,  three-4 !";

    EXPECT_EQ(Boundaries::NextWord(text, 0), 3u);
    EXPECT_EQ(Boundaries::NextWord(text, 1), 3u);
    EXPECT_EQ(Boundaries::NextWord(text, 3), 7u);
    EXPECT_EQ(Boundaries::NextWord(text, 7), 15u);
    EXPECT_EQ(Boundaries::NextWord(text, 15), 17u);
    EXPECT_EQ(Boundaries::NextWord(text, 17), 19u);
    EXPECT_EQ(Boundaries::NextWord(text, 19), 19u);

    EXPECT_EQ(Boundaries::PreviousWord(text, 19), 16u);
    EXPECT_EQ(Boundaries::PreviousWord(text, 16), 10u);
    EXPECT_EQ(Boundaries::PreviousWord(text, 12), 10u);
    EXPECT_EQ(Boundaries::PreviousWord(text, 10), 4u);
    EXPECT_EQ(Boundaries::PreviousWord(text, 4), 0u);
    EXPECT_EQ(Boundaries::PreviousWord(text, 0), 0u);
  }

  TEST(UiTextBoundariesTest, TakesLettersOfOtherScriptsForPartsOfAWord)
  {
    // "Zoë café": with an e with a diaeresis, and an e with a combining
    // acute
    const std::string text = "Zo\xC3\xAB caf" + e_combining;

    EXPECT_EQ(Boundaries::NextWord(text, 0), 4u);
    EXPECT_EQ(Boundaries::NextWord(text, 4), text.size());
    EXPECT_EQ(Boundaries::PreviousWord(text, text.size()), 5u);
    EXPECT_EQ(Boundaries::PreviousWord(text, 5), 0u);
  }

  TEST(UiTextBoundariesTest, FindsTheWordAPlaceIsIn)
  {
    //                        0123456789012
    const std::string text = "one  two, 3!";
    std::size_t start = 99;
    std::size_t end = 99;

    Boundaries::WordAt(text, 1, start, end);
    EXPECT_EQ(start, 0u);
    EXPECT_EQ(end, 3u);

    // in front of a word, the word
    Boundaries::WordAt(text, 5, start, end);
    EXPECT_EQ(start, 5u);
    EXPECT_EQ(end, 8u);

    // spaces that stand together
    Boundaries::WordAt(text, 3, start, end);
    EXPECT_EQ(start, 3u);
    EXPECT_EQ(end, 5u);

    // a sign alone
    Boundaries::WordAt(text, 8, start, end);
    EXPECT_EQ(start, 8u);
    EXPECT_EQ(end, 9u);

    // at the end of the text, what is in front of it
    Boundaries::WordAt(text, 12, start, end);
    EXPECT_EQ(start, 11u);
    EXPECT_EQ(end, 12u);

    Boundaries::WordAt("", 0, start, end);
    EXPECT_EQ(start, 0u);
    EXPECT_EQ(end, 0u);
  }

  TEST(UiTextBoundariesTest, FindsTheLineAPlaceIsIn)
  {
    //                        0123 4567 8 9012
    const std::string text = "one\ntwo\r\n\nend";
    std::size_t start = 99;
    std::size_t end = 99;

    Boundaries::LineAt(text, 1, start, end);
    EXPECT_EQ(start, 0u);
    EXPECT_EQ(end, 3u);

    Boundaries::LineAt(text, 3, start, end);
    EXPECT_EQ(start, 0u);
    EXPECT_EQ(end, 3u);

    Boundaries::LineAt(text, 4, start, end);
    EXPECT_EQ(start, 4u);
    EXPECT_EQ(end, 7u);

    // a line with nothing in it
    Boundaries::LineAt(text, 9, start, end);
    EXPECT_EQ(start, 9u);
    EXPECT_EQ(end, 9u);

    Boundaries::LineAt(text, 13, start, end);
    EXPECT_EQ(start, 10u);
    EXPECT_EQ(end, 13u);
  }

  // the editor

  class UiTextEditorTest : public ::testing::Test
  {
  protected:
    UiTextEditor _editor;
    Memory_Clipboard _clipboard;

    /// A text with the caret at its end, and nothing to undo.
    void Start(const std::string &text)
    {
      _editor.SetText(text);
      _editor.MoveToEnd(false);
    }

    /// Types a text a cluster at a time, as a keyboard does.
    void Type(const std::string &text)
    {
      for (std::size_t at = 0; at < text.size();)
      {
        const std::size_t next = Boundaries::Next(text, at);
        _editor.Insert(text.substr(at, next - at));
        at = next;
      }
    }

    /// The text with `|` where the caret is, and `[` and `]` around what
    /// is selected.
    [[nodiscard]] std::string State() const
    {
      std::string text = _editor.GetText();

      if (!_editor.HasSelection())
      {
        text.insert(_editor.GetCaret(), "|");
        return text;
      }

      text.insert(_editor.GetSelectionEnd(), _editor.GetCaret() == _editor.GetSelectionEnd() ? "|]" : "]");
      text.insert(_editor.GetSelectionStart(), _editor.GetCaret() == _editor.GetSelectionStart() ? "[|" : "[");
      return text;
    }
  };

  TEST_F(UiTextEditorTest, StartsEmpty)
  {
    EXPECT_EQ(State(), "|");
    EXPECT_FALSE(_editor.HasSelection());
    EXPECT_FALSE(_editor.CanUndo());
    EXPECT_FALSE(_editor.CanRedo());
    EXPECT_EQ(_editor.GetKind(), UiTextEditor::Kind::Text);
  }

  // typing

  TEST_F(UiTextEditorTest, PutsWhatIsTypedAtTheCaret)
  {
    EXPECT_TRUE(_editor.Insert("a"));
    EXPECT_TRUE(_editor.Insert("b"));
    EXPECT_EQ(State(), "ab|");

    _editor.SetCaret(1, false);
    EXPECT_TRUE(_editor.Insert("-"));
    EXPECT_EQ(State(), "a-|b");
  }

  TEST_F(UiTextEditorTest, TypesCharactersOfSeveralBytes)
  {
    Type("Zo" + e_acute + " " + euro + face);
    EXPECT_EQ(_editor.GetText(), "Zo" + e_acute + " " + euro + face);
    EXPECT_EQ(_editor.GetCaret(), 12u);

    _editor.SetCaret(2, false);
    _editor.Insert(e_combining);
    EXPECT_EQ(_editor.GetText(), "Zo" + e_combining + e_acute + " " + euro + face);
    EXPECT_EQ(_editor.GetCaret(), 5u);
  }

  TEST_F(UiTextEditorTest, PutsWhatIsTypedInThePlaceOfWhatIsSelected)
  {
    Start("one two three");
    _editor.Select(4, 7);
    EXPECT_EQ(State(), "one [two|] three");

    EXPECT_TRUE(_editor.Insert("2"));
    EXPECT_EQ(State(), "one 2| three");
  }

  TEST_F(UiTextEditorTest, LeavesOutLineFeedsInATextOfOneLine)
  {
    EXPECT_TRUE(_editor.Insert("one\ntwo\r\nthree\ttabbed"));
    EXPECT_EQ(_editor.GetText(), "onetwothreetabbed");

    EXPECT_FALSE(_editor.Insert("\n"));
  }

  TEST_F(UiTextEditorTest, KeepsLineFeedsInATextOfSeveralLines)
  {
    _editor.SetMultiline(true);

    EXPECT_TRUE(_editor.Insert("one\ntwo\r\nthree\rfour"));
    EXPECT_EQ(_editor.GetText(), "one\ntwo\nthree\nfour");
  }

  TEST_F(UiTextEditorTest, LeavesOutWhatDrawsNothing)
  {
    EXPECT_TRUE(_editor.Insert("a\x01\x7F\x1B" "b"));
    EXPECT_EQ(_editor.GetText(), "ab");
  }

  TEST_F(UiTextEditorTest, TakesOnlyWhatANumberIsMadeOf)
  {
    _editor.SetKind(UiTextEditor::Kind::Number);

    EXPECT_TRUE(_editor.Insert("-12.5 apples" + euro));
    EXPECT_EQ(_editor.GetText(), "-12.5");

    EXPECT_FALSE(_editor.Insert("x"));
    EXPECT_FALSE(_editor.Insert(" "));
  }

  TEST_F(UiTextEditorTest, TypesNothingIntoATextThatIsOnlyRead)
  {
    Start("fixed");
    _editor.SetReadOnly(true);

    EXPECT_FALSE(_editor.Insert("a"));
    EXPECT_FALSE(_editor.Backspace(false));
    EXPECT_FALSE(_editor.Delete(false));
    EXPECT_FALSE(_editor.Undo());

    _clipboard.SetText("pasted");
    EXPECT_FALSE(_editor.Paste(_clipboard));
    EXPECT_EQ(_editor.GetText(), "fixed");

    // it can be selected and copied
    _editor.SelectAll();
    EXPECT_FALSE(_editor.Cut(_clipboard));
    EXPECT_TRUE(_editor.Copy(_clipboard));
    EXPECT_EQ(_clipboard.GetText(), "fixed");
    EXPECT_EQ(_editor.GetText(), "fixed");
  }

  // how long a text may be

  TEST_F(UiTextEditorTest, TakesNoMoreThanItMayBeLong)
  {
    _editor.SetMaxLength(5);

    Type("1234567");
    EXPECT_EQ(_editor.GetText(), "12345");
    EXPECT_FALSE(_editor.Insert("8"));
  }

  TEST_F(UiTextEditorTest, CountsTheLengthInWhatACaretStepsOver)
  {
    _editor.SetMaxLength(3);

    // 3 clusters of 3, 8, and 4 bytes
    EXPECT_TRUE(_editor.Insert(e_combining + thumb + face + "more"));
    EXPECT_EQ(_editor.GetText(), e_combining + thumb + face);
  }

  TEST_F(UiTextEditorTest, TakesAsMuchOfWhatIsPastedAsThereIsRoomFor)
  {
    _editor.SetMaxLength(6);
    Start("abcd");

    _clipboard.SetText("123456");
    EXPECT_TRUE(_editor.Paste(_clipboard));
    EXPECT_EQ(_editor.GetText(), "abcd12");
  }

  TEST_F(UiTextEditorTest, MakesRoomWithWhatIsSelected)
  {
    _editor.SetMaxLength(4);
    Start("abcd");
    _editor.Select(1, 3);

    EXPECT_TRUE(_editor.Insert("12345"));
    EXPECT_EQ(_editor.GetText(), "a12d");
  }

  TEST_F(UiTextEditorTest, KeepsATextFromOutsideThatIsLongerThanWhatMayBeTyped)
  {
    _editor.SetMaxLength(3);
    _editor.SetText("a long name");

    EXPECT_EQ(_editor.GetText(), "a long name");
    EXPECT_FALSE(_editor.Insert("!"));
  }

  // deleting

  TEST_F(UiTextEditorTest, DeletesWhatIsInFrontOfTheCaretWithBackspace)
  {
    Start("abc");

    EXPECT_TRUE(_editor.Backspace(false));
    EXPECT_EQ(State(), "ab|");

    _editor.SetCaret(1, false);
    EXPECT_TRUE(_editor.Backspace(false));
    EXPECT_EQ(State(), "|b");

    EXPECT_FALSE(_editor.Backspace(false));
    EXPECT_EQ(State(), "|b");
  }

  TEST_F(UiTextEditorTest, DeletesWhatIsBehindTheCaretWithDelete)
  {
    Start("abc");
    _editor.SetCaret(1, false);

    EXPECT_TRUE(_editor.Delete(false));
    EXPECT_EQ(State(), "a|c");

    EXPECT_TRUE(_editor.Delete(false));
    EXPECT_EQ(State(), "a|");

    EXPECT_FALSE(_editor.Delete(false));
  }

  TEST_F(UiTextEditorTest, DeletesAWholeCharacterOfSeveralBytes)
  {
    Start("a" + euro + e_acute + face);

    EXPECT_TRUE(_editor.Backspace(false));
    EXPECT_EQ(_editor.GetText(), "a" + euro + e_acute);

    EXPECT_TRUE(_editor.Backspace(false));
    EXPECT_EQ(_editor.GetText(), "a" + euro);

    _editor.SetCaret(1, false);
    EXPECT_TRUE(_editor.Delete(false));
    EXPECT_EQ(_editor.GetText(), "a");
  }

  TEST_F(UiTextEditorTest, DeletesALetterWithWhatIsPutOnTopOfIt)
  {
    Start("caf" + e_combining);

    EXPECT_TRUE(_editor.Backspace(false));
    EXPECT_EQ(_editor.GetText(), "caf");

    Start(e_combining + "x");
    _editor.SetCaret(0, false);
    EXPECT_TRUE(_editor.Delete(false));
    EXPECT_EQ(_editor.GetText(), "x");
  }

  TEST_F(UiTextEditorTest, DeletesAnEmojiThatIsMadeOfSeveralAsOne)
  {
    Start("a" + family + flag);

    EXPECT_TRUE(_editor.Backspace(false));
    EXPECT_EQ(_editor.GetText(), "a" + family);

    EXPECT_TRUE(_editor.Backspace(false));
    EXPECT_EQ(_editor.GetText(), "a");
  }

  TEST_F(UiTextEditorTest, DeletesWhatIsSelected)
  {
    Start("one two three");
    _editor.Select(3, 7);
    EXPECT_TRUE(_editor.Backspace(false));
    EXPECT_EQ(State(), "one| three");

    _editor.Select(9, 3);
    EXPECT_TRUE(_editor.Delete(false));
    EXPECT_EQ(State(), "one|");
  }

  TEST_F(UiTextEditorTest, DeletesByWords)
  {
    Start("one two  three");

    EXPECT_TRUE(_editor.Backspace(true));
    EXPECT_EQ(State(), "one two  |");

    EXPECT_TRUE(_editor.Backspace(true));
    EXPECT_EQ(State(), "one |");

    _editor.SetCaret(0, false);
    EXPECT_TRUE(_editor.Delete(true));
    EXPECT_EQ(State(), "| ");
  }

  // moving

  TEST_F(UiTextEditorTest, MovesLeftAndRight)
  {
    Start("ab");

    _editor.MoveLeft(false, false);
    EXPECT_EQ(State(), "a|b");

    _editor.MoveLeft(false, false);
    _editor.MoveLeft(false, false);
    EXPECT_EQ(State(), "|ab");

    _editor.MoveRight(false, false);
    EXPECT_EQ(State(), "a|b");

    _editor.MoveRight(false, false);
    _editor.MoveRight(false, false);
    EXPECT_EQ(State(), "ab|");
  }

  TEST_F(UiTextEditorTest, MovesOverAWholeClusterAtATime)
  {
    const std::string text = "a" + e_combining + euro + family + "z";
    Start(text);
    _editor.MoveToStart(false);

    std::vector<std::size_t> right;
    for (int i = 0; i < 5; i++)
    {
      _editor.MoveRight(false, false);
      right.push_back(_editor.GetCaret());
    }
    EXPECT_THAT(right, ElementsAre(1u, 4u, 7u, 7u + family.size(), 8u + family.size()));

    std::vector<std::size_t> left;
    for (int i = 0; i < 5; i++)
    {
      _editor.MoveLeft(false, false);
      left.push_back(_editor.GetCaret());
    }
    EXPECT_THAT(left, ElementsAre(7u + family.size(), 7u, 4u, 1u, 0u));
  }

  TEST_F(UiTextEditorTest, MovesByWordsWithTheModifier)
  {
    Start("one two, three");

    _editor.MoveLeft(false, true);
    EXPECT_EQ(State(), "one two, |three");

    _editor.MoveLeft(false, true);
    EXPECT_EQ(State(), "one |two, three");

    _editor.MoveRight(false, true);
    EXPECT_EQ(State(), "one two|, three");

    _editor.MoveRight(false, true);
    EXPECT_EQ(State(), "one two, three|");
  }

  TEST_F(UiTextEditorTest, MovesToTheEndsOfTheLineAndOfTheText)
  {
    _editor.SetMultiline(true);
    Start("one\ntwo three\nfour");
    _editor.SetCaret(7, false);

    _editor.MoveToLineStart(false);
    EXPECT_EQ(State(), "one\n|two three\nfour");

    _editor.MoveToLineEnd(false);
    EXPECT_EQ(State(), "one\ntwo three|\nfour");

    _editor.MoveToStart(false);
    EXPECT_EQ(State(), "|one\ntwo three\nfour");

    _editor.MoveToEnd(false);
    EXPECT_EQ(State(), "one\ntwo three\nfour|");
  }

  TEST_F(UiTextEditorTest, PutsTheCaretWhereItMayStand)
  {
    Start("a" + e_combining + euro);

    _editor.SetCaret(2, false);
    EXPECT_EQ(_editor.GetCaret(), 1u);

    _editor.SetCaret(5, false);
    EXPECT_EQ(_editor.GetCaret(), 4u);

    _editor.SetCaret(1000, false);
    EXPECT_EQ(_editor.GetCaret(), 7u);
  }

  // selecting

  TEST_F(UiTextEditorTest, SelectsWhileItMovesWithShift)
  {
    Start("abcd");
    _editor.SetCaret(2, false);

    _editor.MoveRight(true, false);
    EXPECT_EQ(State(), "ab[c|]d");
    EXPECT_EQ(_editor.GetSelectedText(), "c");

    _editor.MoveRight(true, false);
    EXPECT_EQ(State(), "ab[cd|]");

    // back over where it started
    _editor.MoveLeft(true, false);
    _editor.MoveLeft(true, false);
    _editor.MoveLeft(true, false);
    EXPECT_EQ(State(), "a[|b]cd");
    EXPECT_EQ(_editor.GetAnchor(), 2u);
    EXPECT_EQ(_editor.GetCaret(), 1u);
  }

  TEST_F(UiTextEditorTest, SelectsByWordsAndToTheEnds)
  {
    Start("one two three");
    _editor.SetCaret(4, false);

    _editor.MoveRight(true, true);
    EXPECT_EQ(State(), "one [two|] three");

    _editor.MoveToLineEnd(true);
    EXPECT_EQ(State(), "one [two three|]");

    _editor.MoveToLineStart(true);
    EXPECT_EQ(State(), "[|one ]two three");
  }

  TEST_F(UiTextEditorTest, LeavesWhatIsSelectedAtItsEndsWithoutShift)
  {
    Start("one two three");

    _editor.Select(4, 7);
    _editor.MoveLeft(false, false);
    EXPECT_EQ(State(), "one |two three");

    _editor.Select(4, 7);
    _editor.MoveRight(false, false);
    EXPECT_EQ(State(), "one two| three");

    // whichever way it was selected
    _editor.Select(7, 4);
    _editor.MoveRight(false, false);
    EXPECT_EQ(State(), "one two| three");
  }

  TEST_F(UiTextEditorTest, SelectsEverything)
  {
    Start("one " + euro);
    _editor.SetCaret(2, false);

    _editor.SelectAll();
    EXPECT_EQ(State(), "[one " + euro + "|]");
    EXPECT_EQ(_editor.GetSelectedText(), "one " + euro);
  }

  TEST_F(UiTextEditorTest, SelectsTheWordAtAPlaceAsADoubleClickDoes)
  {
    Start("one caf" + e_combining + " three");

    _editor.SelectWordAt(5);
    EXPECT_EQ(State(), "one [caf" + e_combining + "|] three");

    _editor.SelectWordAt(3);
    EXPECT_EQ(State(), "one[ |]caf" + e_combining + " three");
  }

  TEST_F(UiTextEditorTest, SelectsTheLineAtAPlaceAsATripleClickDoes)
  {
    _editor.SetMultiline(true);
    Start("one\ntwo three\nfour");

    _editor.SelectLineAt(6);
    EXPECT_EQ(State(), "one\n[two three|]\nfour");

    _editor.SetMultiline(false);
    Start("all of one line");
    _editor.SelectLineAt(3);
    EXPECT_EQ(State(), "[all of one line|]");
  }

  TEST_F(UiTextEditorTest, SelectsBetweenPlacesACaretMayStandAt)
  {
    Start("a" + e_combining + "b");

    // from inside of the e with its accent to inside of it
    _editor.Select(2, 3);
    EXPECT_FALSE(_editor.HasSelection());
    EXPECT_EQ(_editor.GetCaret(), 1u);

    _editor.Select(0, 3);
    EXPECT_EQ(_editor.GetSelectedText(), "a");
  }

  // the clipboard

  TEST_F(UiTextEditorTest, CopiesWhatIsSelected)
  {
    Start("one two three");
    _editor.Select(4, 7);

    EXPECT_TRUE(_editor.Copy(_clipboard));
    EXPECT_EQ(_clipboard.GetText(), "two");
    EXPECT_EQ(State(), "one [two|] three");
  }

  TEST_F(UiTextEditorTest, CopiesNothingWhenNothingIsSelected)
  {
    Start("one");
    _clipboard.SetText("before");

    EXPECT_FALSE(_editor.Copy(_clipboard));
    EXPECT_FALSE(_editor.Cut(_clipboard));
    EXPECT_EQ(_clipboard.GetText(), "before");
  }

  TEST_F(UiTextEditorTest, CutsWhatIsSelected)
  {
    Start("one two three");
    _editor.Select(3, 7);

    EXPECT_TRUE(_editor.Cut(_clipboard));
    EXPECT_EQ(_clipboard.GetText(), " two");
    EXPECT_EQ(State(), "one| three");
  }

  TEST_F(UiTextEditorTest, PastesAtTheCaretAndInThePlaceOfWhatIsSelected)
  {
    Start("one three");
    _editor.SetCaret(4, false);
    _clipboard.SetText("two ");

    EXPECT_TRUE(_editor.Paste(_clipboard));
    EXPECT_EQ(State(), "one two |three");

    _editor.Select(0, 3);
    _clipboard.SetText("1");
    EXPECT_TRUE(_editor.Paste(_clipboard));
    EXPECT_EQ(State(), "1| two three");
  }

  TEST_F(UiTextEditorTest, PastesTextOfSeveralBytes)
  {
    Start("a");
    _clipboard.SetText(e_combining + euro + family);

    EXPECT_TRUE(_editor.Paste(_clipboard));
    EXPECT_EQ(_editor.GetText(), "a" + e_combining + euro + family);
    EXPECT_EQ(_editor.GetCaret(), _editor.GetText().size());
  }

  TEST_F(UiTextEditorTest, PastesNothingFromAnEmptyClipboard)
  {
    Start("one");
    EXPECT_FALSE(_editor.Paste(_clipboard));
    EXPECT_EQ(State(), "one|");
  }

  TEST_F(UiTextEditorTest, LeavesOutOfWhatIsPastedWhatCannotBePartOfTheText)
  {
    Start("");
    _clipboard.SetText("two\nlines");

    EXPECT_TRUE(_editor.Paste(_clipboard));
    EXPECT_EQ(_editor.GetText(), "twolines");
  }

  TEST_F(UiTextEditorTest, NeitherCopiesNorCutsAPassword)
  {
    _editor.SetKind(UiTextEditor::Kind::Password);
    Start("secret");
    _editor.SelectAll();

    EXPECT_FALSE(_editor.Copy(_clipboard));
    EXPECT_FALSE(_editor.Cut(_clipboard));
    EXPECT_FALSE(_clipboard.HasText());
    EXPECT_EQ(_editor.GetText(), "secret");

    // it can be pasted into
    _clipboard.SetText("other");
    EXPECT_TRUE(_editor.Paste(_clipboard));
    EXPECT_EQ(_editor.GetText(), "other");
  }

  // undoing

  TEST_F(UiTextEditorTest, UndoesAndRedoesAChange)
  {
    Start("one");
    _editor.Insert(" two");

    ASSERT_TRUE(_editor.CanUndo());
    EXPECT_TRUE(_editor.Undo());
    EXPECT_EQ(State(), "one|");

    ASSERT_TRUE(_editor.CanRedo());
    EXPECT_TRUE(_editor.Redo());
    EXPECT_EQ(State(), "one two|");

    EXPECT_FALSE(_editor.Redo());
  }

  TEST_F(UiTextEditorTest, UndoesAWordThatWasTypedAsAWhole)
  {
    Type("one two three");

    EXPECT_TRUE(_editor.Undo());
    EXPECT_EQ(State(), "one two |");

    // the space behind a word is a step of its own
    EXPECT_TRUE(_editor.Undo());
    EXPECT_EQ(State(), "one two|");

    EXPECT_TRUE(_editor.Undo());
    EXPECT_EQ(State(), "one |");

    EXPECT_TRUE(_editor.Undo());
    EXPECT_TRUE(_editor.Undo());
    EXPECT_EQ(State(), "|");

    EXPECT_FALSE(_editor.Undo());
  }

  TEST_F(UiTextEditorTest, StartsANewStepWhenTheCaretMoved)
  {
    Type("ab");
    _editor.MoveLeft(false, false);
    Type("cd");

    EXPECT_EQ(State(), "acd|b");

    EXPECT_TRUE(_editor.Undo());
    EXPECT_EQ(State(), "a|b");

    EXPECT_TRUE(_editor.Undo());
    EXPECT_EQ(State(), "|");
  }

  TEST_F(UiTextEditorTest, UndoesEveryKindOfChange)
  {
    Start("one two three");

    _editor.Backspace(true);
    EXPECT_EQ(State(), "one two |");

    _editor.Select(0, 4);
    _editor.Cut(_clipboard);
    EXPECT_EQ(State(), "|two ");

    _editor.MoveToEnd(false);
    _editor.Paste(_clipboard);
    EXPECT_EQ(State(), "two one |");

    _editor.MoveToStart(false);
    _editor.Delete(false);
    EXPECT_EQ(State(), "|wo one ");

    EXPECT_TRUE(_editor.Undo());
    EXPECT_EQ(State(), "|two one ");

    EXPECT_TRUE(_editor.Undo());
    EXPECT_EQ(State(), "two |");

    // what was selected is selected again
    EXPECT_TRUE(_editor.Undo());
    EXPECT_EQ(State(), "[one |]two ");

    EXPECT_TRUE(_editor.Undo());
    EXPECT_EQ(State(), "one two three|");

    EXPECT_FALSE(_editor.Undo());

    // and all of it again
    for (int i = 0; i < 4; i++) { EXPECT_TRUE(_editor.Redo()); }
    EXPECT_EQ(State(), "|wo one ");
  }

  TEST_F(UiTextEditorTest, ForgetsWhatWasUndoneOnceSomethingElseIsTyped)
  {
    Start("one");
    _editor.Insert(" two");
    _editor.Undo();

    _editor.Insert(" 2");
    EXPECT_FALSE(_editor.CanRedo());
    EXPECT_EQ(State(), "one 2|");
  }

  TEST_F(UiTextEditorTest, UndoesTextOfSeveralBytesAsItWas)
  {
    Start("a" + e_combining);
    _editor.Insert(family);
    _editor.Backspace(false);
    _editor.Backspace(false);
    EXPECT_EQ(_editor.GetText(), "a");

    _editor.Undo();
    EXPECT_EQ(_editor.GetText(), "a" + e_combining);

    _editor.Undo();
    EXPECT_EQ(_editor.GetText(), "a" + e_combining + family);
  }

  TEST_F(UiTextEditorTest, KeepsNoMoreStepsThanItIsToldTo)
  {
    for (std::size_t i = 0; i < UiTextEditor::max_history + 50; i++)
    {
      _editor.Insert("a");
      _editor.Insert(" ");
    }

    std::size_t undone = 0;
    while (_editor.Undo()) { undone++; }

    EXPECT_EQ(undone, UiTextEditor::max_history);
    EXPECT_FALSE(_editor.GetText().empty());
  }

  TEST_F(UiTextEditorTest, HasNothingToUndoOnceTheTextWasReplacedFromOutside)
  {
    Type("typed");
    EXPECT_TRUE(_editor.SetText("from the game"));

    EXPECT_FALSE(_editor.CanUndo());
    EXPECT_EQ(_editor.GetText(), "from the game");

    // the same text again changes nothing
    EXPECT_FALSE(_editor.SetText("from the game"));
  }

  TEST_F(UiTextEditorTest, KeepsTheCaretWhenTheTextIsReplacedFromOutside)
  {
    Start("one two");
    _editor.SetCaret(3, false);

    _editor.SetText("one " + euro);
    EXPECT_EQ(_editor.GetCaret(), 3u);

    _editor.SetText("1");
    EXPECT_EQ(_editor.GetCaret(), 1u);

    // where it may stand
    _editor.SetText("abcdef");
    _editor.SetCaret(2, false);
    _editor.SetText("a" + e_combining + "z");
    EXPECT_EQ(_editor.GetCaret(), 1u);
  }

  // an input method

  TEST_F(UiTextEditorTest, ShowsWhatIsPutTogetherAtTheCaretWithoutTakingItIntoTheText)
  {
    Start("ab");
    _editor.SetCaret(1, false);

    // "ni" on its way to a character of Japanese
    _editor.SetComposition("ni", 2);

    std::size_t caret = 0;
    std::size_t start = 0;
    std::size_t end = 0;
    EXPECT_EQ(_editor.GetShownText(caret, start, end), "anib");
    EXPECT_EQ(caret, 3u);
    EXPECT_EQ(start, 1u);
    EXPECT_EQ(end, 3u);

    EXPECT_EQ(_editor.GetText(), "ab");
    EXPECT_EQ(_editor.GetComposition(), "ni");
    EXPECT_FALSE(_editor.CanUndo());
  }

  TEST_F(UiTextEditorTest, FollowsWhatIsPutTogetherAsItChanges)
  {
    Start("");

    _editor.SetComposition("n", 1);
    _editor.SetComposition("\xE3\x81\xAB", 1);
    _editor.SetComposition("\xE3\x81\xAB\xE3\x81\xBB", 1);

    std::size_t caret = 0;
    std::size_t start = 0;
    std::size_t end = 0;
    EXPECT_EQ(_editor.GetShownText(caret, start, end), "\xE3\x81\xAB\xE3\x81\xBB");

    // the caret of the input method is behind the first of the two
    EXPECT_EQ(caret, 3u);
    EXPECT_EQ(end, 6u);
    EXPECT_EQ(_editor.GetText(), "");
  }

  TEST_F(UiTextEditorTest, TakesWhatWasPutTogetherIntoTheTextWhenItIsCommitted)
  {
    Start("a");
    _editor.SetComposition("nihon", 5);

    EXPECT_TRUE(_editor.Insert("\xE6\x97\xA5\xE6\x9C\xAC"));

    EXPECT_EQ(_editor.GetText(), "a\xE6\x97\xA5\xE6\x9C\xAC");
    EXPECT_EQ(_editor.GetComposition(), "");
    EXPECT_EQ(_editor.GetCaret(), 7u);

    // as one step
    EXPECT_TRUE(_editor.Undo());
    EXPECT_EQ(_editor.GetText(), "a");
  }

  TEST_F(UiTextEditorTest, LeavesTheTextAsItWasWhenWhatWasPutTogetherIsGivenUp)
  {
    Start("ab");
    _editor.SetComposition("ni", 2);
    _editor.SetComposition("", 0);

    std::size_t caret = 0;
    std::size_t start = 0;
    std::size_t end = 0;
    EXPECT_EQ(_editor.GetShownText(caret, start, end), "ab");
    EXPECT_EQ(caret, 2u);
    EXPECT_EQ(_editor.GetText(), "ab");
  }

  TEST_F(UiTextEditorTest, PutsWhatIsPutTogetherInThePlaceOfWhatIsSelected)
  {
    Start("one two three");
    _editor.Select(4, 7);
    _editor.SetComposition("2", 1);

    std::size_t caret = 0;
    std::size_t start = 0;
    std::size_t end = 0;
    EXPECT_EQ(_editor.GetShownText(caret, start, end), "one 2 three");

    _editor.Insert("2");
    EXPECT_EQ(State(), "one 2| three");
  }

  TEST_F(UiTextEditorTest, PutsNothingTogetherInATextThatIsOnlyRead)
  {
    Start("fixed");
    _editor.SetReadOnly(true);
    _editor.SetComposition("ni", 2);

    EXPECT_EQ(_editor.GetComposition(), "");
  }

  // what is shown

  TEST_F(UiTextEditorTest, ShowsTheTextWithTheCaretAndWhatIsSelected)
  {
    Start("one " + euro + " two");
    _editor.Select(4, 7);

    std::size_t caret = 0;
    std::size_t start = 0;
    std::size_t end = 0;
    EXPECT_EQ(_editor.GetShownText(caret, start, end), "one " + euro + " two");
    EXPECT_EQ(caret, 7u);
    EXPECT_EQ(start, 4u);
    EXPECT_EQ(end, 7u);
  }

  TEST_F(UiTextEditorTest, ShowsABulletForEveryClusterOfAPassword)
  {
    _editor.SetKind(UiTextEditor::Kind::Password);
    Start("a" + e_combining + family);
    _editor.Select(1, 4);

    std::size_t caret = 0;
    std::size_t start = 0;
    std::size_t end = 0;

    // three bullets of three bytes each
    const std::string bullet = "\xE2\x80\xA2";
    EXPECT_EQ(_editor.GetShownText(caret, start, end), bullet + bullet + bullet);
    EXPECT_EQ(start, 3u);
    EXPECT_EQ(end, 6u);
    EXPECT_EQ(caret, 6u);
  }

  TEST_F(UiTextEditorTest, FindsThePlaceInTheTextForAPlaceInWhatIsShown)
  {
    Start("a" + e_combining + "z");
    EXPECT_EQ(_editor.FromShown(0), 0u);
    EXPECT_EQ(_editor.FromShown(1), 1u);
    EXPECT_EQ(_editor.FromShown(2), 1u);
    EXPECT_EQ(_editor.FromShown(4), 4u);
    EXPECT_EQ(_editor.FromShown(99), 5u);

    // of a password, behind the bullets
    _editor.SetKind(UiTextEditor::Kind::Password);
    EXPECT_EQ(_editor.FromShown(0), 0u);
    EXPECT_EQ(_editor.FromShown(3), 1u);
    EXPECT_EQ(_editor.FromShown(6), 4u);
    EXPECT_EQ(_editor.FromShown(9), 5u);
    EXPECT_EQ(_editor.FromShown(99), 5u);
  }
} // namespace
