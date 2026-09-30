#ifndef UI_TEXT_EDITOR_HPP
#define UI_TEXT_EDITOR_HPP

#include <cstddef>
#include <string>
#include <vector>

#include <neon/input/clipboard-context.hpp>

namespace neon
{
  /// Where a caret may stand in a text, and what belongs together.
  ///
  /// Text is UTF-8, and places in it are counted in bytes. A caret never
  /// stands inside the bytes of a character, and never between a character
  /// and what is put on top of it, such as the accent of `e` and a
  /// combining acute. What is taken for one from the point of view of a
  /// caret is called a cluster here.
  ///
  /// This covers what can be told without the tables of Unicode: combining
  /// marks of the scripts that are common, variation selectors, the
  /// modifiers and joiners of emoji, flags, and a carriage return with its
  /// line feed. It is not all of https://www.unicode.org/reports/tr29/:
  /// the syllables of Hangul that are written in parts, the conjuncts of
  /// the scripts of India, and marks that take room of their own are
  /// stepped through part by part.
  namespace UiTextBoundaries
  {
    /// The start of the cluster behind the one a place is in or in front
    /// of: where a caret goes that moves right. The length of the text at
    /// its end.
    [[nodiscard]] std::size_t Next(const std::string &text, std::size_t offset);

    /// The start of the cluster in front of a place: where a caret goes
    /// that moves left.
    [[nodiscard]] std::size_t Previous(const std::string &text, std::size_t offset);

    /// The nearest place at or in front of `offset` where a caret may
    /// stand.
    [[nodiscard]] std::size_t Snap(const std::string &text, std::size_t offset);

    /// Where the word ends that a place is in or in front of, and where
    /// the word starts that a place is in or behind, as moving by a word
    /// goes.
    [[nodiscard]] std::size_t NextWord(const std::string &text, std::size_t offset);

    [[nodiscard]] std::size_t PreviousWord(const std::string &text, std::size_t offset);

    /// The word a place is in, as a double click selects it: letters and
    /// digits that stand together, or spaces that do, or one sign.
    void WordAt(const std::string &text, std::size_t offset, std::size_t &start, std::size_t &end);

    /// The line a place is in, without the line feed that ends it.
    void LineAt(const std::string &text, std::size_t offset, std::size_t &start, std::size_t &end);

    /// How many clusters a text has, which is how long it is to whoever
    /// types it.
    [[nodiscard]] std::size_t Count(const std::string &text);
  }

  /// A text that is typed and changed, with its caret and what is
  /// selected. It knows nothing of where the text is shown: what needs the
  /// places of characters, such as moving up a line, is told where to go.
  class UiTextEditor
  {
  public:
    enum class Kind
    {
      Text = 0,

      /// What is typed is not shown, and is neither cut nor copied.
      Password,

      /// Digits, a sign, and a point.
      Number
    };

    /// What can be undone is kept up to this many steps.
    static constexpr std::size_t max_history = 200;

  private:
    struct Snapshot
    {
      std::string text;
      std::size_t caret = 0;
      std::size_t anchor = 0;
    };

    std::string _text;

    // where the caret is, and where what is selected starts. The two are
    // the same when nothing is selected
    std::size_t _caret = 0;
    std::size_t _anchor = 0;

    Kind _kind = Kind::Text;
    bool _is_multiline = false;
    bool _is_read_only = false;

    // in clusters. 0 stands for no limit
    std::size_t _max_length = 0;

    // what an input method is putting together, which is no part of the
    // text yet
    std::string _composition;
    std::size_t _composition_caret = 0;

    std::vector<Snapshot> _undo;
    std::vector<Snapshot> _redo;

    // whether what is typed next joins the step before it, so that a word
    // is undone as a whole
    bool _is_typing = false;

    void Remember(bool typing);

    /// What of a text can be part of this one.
    [[nodiscard]] std::string Filtered(const std::string &text) const;

    void Replace(std::size_t start, std::size_t end, const std::string &text);

  public:
    void SetKind(Kind kind);

    [[nodiscard]] Kind GetKind() const;

    void SetMultiline(bool is_multiline);

    [[nodiscard]] bool IsMultiline() const;

    void SetReadOnly(bool is_read_only);

    [[nodiscard]] bool IsReadOnly() const;

    /// In clusters. 0 stands for no limit.
    void SetMaxLength(std::size_t max_length);

    [[nodiscard]] std::size_t GetMaxLength() const;

    [[nodiscard]] const std::string &GetText() const;

    /// Replaces the text from outside, as a game does. The caret is kept
    /// where it is as far as the text allows, and nothing of it can be
    /// undone. Returns whether the text changed.
    bool SetText(const std::string &text);

    [[nodiscard]] std::size_t GetCaret() const;

    [[nodiscard]] std::size_t GetAnchor() const;

    [[nodiscard]] bool HasSelection() const;

    /// From the start of what is selected to its end.
    [[nodiscard]] std::size_t GetSelectionStart() const;

    [[nodiscard]] std::size_t GetSelectionEnd() const;

    [[nodiscard]] std::string GetSelectedText() const;

    /// Puts the caret at a place, which is moved to the nearest place a
    /// caret may stand at. With `selecting`, what is between where the
    /// selection started and the caret is selected.
    void SetCaret(std::size_t offset, bool selecting);

    void Select(std::size_t start, std::size_t end);

    void SelectAll();

    void SelectWordAt(std::size_t offset);

    void SelectLineAt(std::size_t offset);

    // moving the caret

    void MoveLeft(bool selecting, bool by_word);

    void MoveRight(bool selecting, bool by_word);

    /// To the start and the end of the line the caret is in, or of the
    /// whole text.
    void MoveToLineStart(bool selecting);

    void MoveToLineEnd(bool selecting);

    void MoveToStart(bool selecting);

    void MoveToEnd(bool selecting);

    // changing the text. Each returns whether the text changed.

    /// Puts a text in the place of what is selected, or at the caret. What
    /// cannot be part of the text is left out: a line feed in a text of
    /// one line, anything but digits in a number, and what would make the
    /// text longer than it may be.
    bool Insert(const std::string &text);

    bool Backspace(bool by_word);

    bool Delete(bool by_word);

    // the clipboard

    /// Returns whether something was copied. Nothing is, of a password.
    bool Copy(ClipboardContext &clipboard) const;

    bool Cut(ClipboardContext &clipboard);

    bool Paste(ClipboardContext &clipboard);

    // undoing

    [[nodiscard]] bool CanUndo() const;

    [[nodiscard]] bool CanRedo() const;

    bool Undo();

    bool Redo();

    // an input method

    /// What an input method is putting together, shown at the caret.
    /// `caret` is where the input method has its own caret, in characters
    /// from the start of what it holds. An empty text ends it.
    void SetComposition(const std::string &text, int caret);

    [[nodiscard]] const std::string &GetComposition() const;

    /// Where the caret of the input method is, in bytes from the start of
    /// what it puts together.
    [[nodiscard]] std::size_t GetCompositionCaret() const;

    /// The text as it is shown: with what is put together at the caret,
    /// and with a bullet for every cluster of a password. `caret` is where
    /// the caret is in it, and the two that follow where what is selected
    /// starts and ends.
    [[nodiscard]] std::string GetShownText(
      std::size_t &caret,
      std::size_t &selection_start,
      std::size_t &selection_end) const;

    /// The place in the text for a place in the text as it is shown.
    [[nodiscard]] std::size_t FromShown(std::size_t shown_offset) const;
  };
} // neon

#endif //UI_TEXT_EDITOR_HPP
