#include "ui-text-editor.hpp"

#include <algorithm>

namespace neon
{
  // Helpers of UiTextEditor, for this file alone.
  namespace
  {
    // what stands for a character of a password
    const std::string bullet = "\xE2\x80\xA2";

    bool IsContinuation(const char byte)
    {
      return (static_cast<unsigned char>(byte) & 0xC0) == 0x80;
    }

    /// The character that starts at a place, and how many bytes it has.
    /// Bytes that are no character count as one each.
    char32_t CharacterAt(const std::string &text, const std::size_t offset, std::size_t &length)
    {
      const auto first = static_cast<unsigned char>(text[offset]);

      std::size_t wanted = 1;
      char32_t character = first;

      if (first >= 0xF0 && first < 0xF8)
      {
        wanted = 4;
        character = first & 0x07;
      } else if (first >= 0xE0)
      {
        wanted = 3;
        character = first & 0x0F;
      } else if (first >= 0xC0)
      {
        wanted = 2;
        character = first & 0x1F;
      }

      if (first >= 0xF8 || (first >= 0x80 && first < 0xC0) || offset + wanted > text.size())
      {
        length = 1;
        return 0xFFFD;
      }

      for (std::size_t i = 1; i < wanted; i++)
      {
        if (!IsContinuation(text[offset + i]))
        {
          length = 1;
          return 0xFFFD;
        }
        character = (character << 6) | (static_cast<unsigned char>(text[offset + i]) & 0x3F);
      }

      length = wanted;
      return character;
    }

    /// The start of the character in front of a place.
    std::size_t CharacterBefore(const std::string &text, std::size_t offset)
    {
      if (offset == 0) { return 0; }

      offset--;

      // no character has more than three bytes behind its first
      for (int i = 0; i < 3 && offset > 0 && IsContinuation(text[offset]); i++) { offset--; }
      return offset;
    }

    bool IsBetween(const char32_t character, const char32_t first, const char32_t last)
    {
      return character >= first && character <= last;
    }

    /// What is put on top of the character in front of it, and belongs to
    /// it.
    bool Extends(const char32_t character)
    {
      return
        // combining marks
        IsBetween(character, 0x0300, 0x036F) || IsBetween(character, 0x1AB0, 0x1AFF) ||
        IsBetween(character, 0x1DC0, 0x1DFF) || IsBetween(character, 0x20D0, 0x20FF) ||
        IsBetween(character, 0xFE20, 0xFE2F) ||
        // of Cyrillic, Hebrew, Arabic, and Thai
        IsBetween(character, 0x0483, 0x0489) || IsBetween(character, 0x0591, 0x05BD) || character == 0x05BF ||
        IsBetween(character, 0x05C1, 0x05C2) || IsBetween(character, 0x05C4, 0x05C5) || character == 0x05C7 ||
        IsBetween(character, 0x0610, 0x061A) || IsBetween(character, 0x064B, 0x065F) || character == 0x0670 ||
        IsBetween(character, 0x06D6, 0x06DC) || IsBetween(character, 0x06DF, 0x06E4) ||
        IsBetween(character, 0x06E7, 0x06E8) || IsBetween(character, 0x06EA, 0x06ED) || character == 0x0E31 ||
        IsBetween(character, 0x0E34, 0x0E3A) || IsBetween(character, 0x0E47, 0x0E4E) ||
        // the voiced marks of Japanese
        IsBetween(character, 0x3099, 0x309A) ||
        // variation selectors
        IsBetween(character, 0xFE00, 0xFE0F) || IsBetween(character, 0xE0100, 0xE01EF) ||
        // the tones of skin of emoji, and what joins two of them
        IsBetween(character, 0x1F3FB, 0x1F3FF) || character == 0x200D;
    }

    bool IsRegionalIndicator(const char32_t character)
    {
      return IsBetween(character, 0x1F1E6, 0x1F1FF);
    }

    enum class Class
    {
      Space = 0,
      Word,
      Sign
    };

    Class ClassOf(const char32_t character)
    {
      if (character == U' ' || character == U'\t' || character == U'\n' || character == U'\r' ||
          character == 0xA0 || character == 0x3000)
      {
        return Class::Space;
      }

      if ((character >= U'0' && character <= U'9') || (character >= U'a' && character <= U'z') ||
          (character >= U'A' && character <= U'Z') || character == U'_' || character == U'\'')
      {
        return Class::Word;
      }

      // what is no sign of the first pages of Unicode is a letter of some
      // script
      if (character >= 0xC0 && !IsBetween(character, 0x2000, 0x206F) && !IsBetween(character, 0x3000, 0x303F) &&
          character != 0xD7 && character != 0xF7)
      {
        return Class::Word;
      }

      return Class::Sign;
    }

    Class ClassAt(const std::string &text, const std::size_t offset)
    {
      std::size_t length = 0;
      return ClassOf(CharacterAt(text, offset, length));
    }
  }

  namespace UiTextBoundaries
  {
    std::size_t Next(const std::string &text, const std::size_t offset)
    {
      if (offset >= text.size()) { return text.size(); }

      // to the start of a character, should the place be inside of one
      std::size_t at = offset;
      while (at > 0 && IsContinuation(text[at])) { at--; }

      std::size_t length = 0;
      const char32_t first = CharacterAt(text, at, length);
      at += length;

      // a carriage return and its line feed
      if (first == U'\r' && at < text.size() && text[at] == '\n') { return at + 1; }

      // two that make a flag
      if (IsRegionalIndicator(first) && at < text.size())
      {
        if (IsRegionalIndicator(CharacterAt(text, at, length))) { at += length; }
      }

      bool joins = false;

      while (at < text.size())
      {
        const char32_t character = CharacterAt(text, at, length);

        // what follows a joiner belongs to what is in front of it
        if (!Extends(character) && !joins) { break; }

        joins = character == 0x200D;
        at += length;
      }

      // a byte that is no character in front of the place is stepped
      // over, and the place is always left behind
      return std::max(at, offset + 1);
    }

    std::size_t Previous(const std::string &text, const std::size_t offset)
    {
      if (offset == 0 || text.empty()) { return 0; }

      // The start of the cluster that ends at the place, or that the
      // place is in: found from the front, since what belongs together is
      // told from its first character.
      const std::size_t place = std::min(offset, text.size());

      // from a place that is sure to be a start: behind a line feed, or
      // not further back than a cluster can be long
      std::size_t start = place;
      for (int i = 0; i < 64 && start > 0; i++)
      {
        start = CharacterBefore(text, start);
        if (start > 0 && text[start - 1] == '\n') { break; }
      }

      std::size_t before = start;
      for (std::size_t at = start; at < place;)
      {
        before = at;
        at = Next(text, at);
      }

      return before;
    }

    std::size_t Snap(const std::string &text, const std::size_t offset)
    {
      if (offset >= text.size()) { return text.size(); }
      if (offset == 0) { return 0; }

      const std::size_t before = Previous(text, offset);
      return Next(text, before) == offset ? offset : before;
    }

    std::size_t NextWord(const std::string &text, std::size_t offset)
    {
      offset = Snap(text, offset);

      // over what stands between words, and then over the word
      while (offset < text.size() && ClassAt(text, offset) != Class::Word) { offset = Next(text, offset); }
      while (offset < text.size() && ClassAt(text, offset) == Class::Word) { offset = Next(text, offset); }

      return offset;
    }

    std::size_t PreviousWord(const std::string &text, std::size_t offset)
    {
      offset = Snap(text, offset);

      while (offset > 0 && ClassAt(text, Previous(text, offset)) != Class::Word) { offset = Previous(text, offset); }
      while (offset > 0 && ClassAt(text, Previous(text, offset)) == Class::Word) { offset = Previous(text, offset); }

      return offset;
    }

    void WordAt(const std::string &text, const std::size_t offset, std::size_t &start, std::size_t &end)
    {
      start = Snap(text, offset);
      end = start;

      if (text.empty()) { return; }

      // at the end of the text, the word in front of it
      if (start >= text.size()) { start = Previous(text, text.size()); }

      const Class wanted = ClassAt(text, start);
      end = Next(text, start);

      // a sign stands alone
      if (wanted == Class::Sign) { return; }

      while (start > 0 && ClassAt(text, Previous(text, start)) == wanted) { start = Previous(text, start); }
      while (end < text.size() && ClassAt(text, end) == wanted) { end = Next(text, end); }
    }

    void LineAt(const std::string &text, const std::size_t offset, std::size_t &start, std::size_t &end)
    {
      start = std::min(offset, text.size());
      end = start;

      while (start > 0 && text[start - 1] != '\n') { start--; }
      while (end < text.size() && text[end] != '\n') { end++; }

      // the carriage return belongs to the line feed
      if (end > start && text[end - 1] == '\r') { end--; }
    }

    std::size_t Count(const std::string &text)
    {
      std::size_t count = 0;
      for (std::size_t at = 0; at < text.size(); at = Next(text, at)) { count++; }
      return count;
    }
  }

  void UiTextEditor::SetKind(const Kind kind)
  {
    _kind = kind;
  }

  UiTextEditor::Kind UiTextEditor::GetKind() const
  {
    return _kind;
  }

  void UiTextEditor::SetMultiline(const bool is_multiline)
  {
    _is_multiline = is_multiline;
  }

  bool UiTextEditor::IsMultiline() const
  {
    return _is_multiline;
  }

  void UiTextEditor::SetReadOnly(const bool is_read_only)
  {
    _is_read_only = is_read_only;
  }

  bool UiTextEditor::IsReadOnly() const
  {
    return _is_read_only;
  }

  void UiTextEditor::SetMaxLength(const std::size_t max_length)
  {
    _max_length = max_length;
  }

  std::size_t UiTextEditor::GetMaxLength() const
  {
    return _max_length;
  }

  const std::string &UiTextEditor::GetText() const
  {
    return _text;
  }

  bool UiTextEditor::SetText(const std::string &text)
  {
    // what comes from outside is held to what a text of this kind can be,
    // and not to how long it may be: that is for what is typed
    const std::size_t max_length = _max_length;
    _max_length = 0;
    const std::string filtered = Filtered(text);
    _max_length = max_length;

    if (filtered == _text) { return false; }

    _text = filtered;
    _caret = UiTextBoundaries::Snap(_text, std::min(_caret, _text.size()));
    _anchor = _caret;
    _composition.clear();

    // what was typed before belongs to a text that is gone
    _undo.clear();
    _redo.clear();
    _is_typing = false;
    return true;
  }

  std::size_t UiTextEditor::GetCaret() const
  {
    return _caret;
  }

  std::size_t UiTextEditor::GetAnchor() const
  {
    return _anchor;
  }

  bool UiTextEditor::HasSelection() const
  {
    return _caret != _anchor;
  }

  std::size_t UiTextEditor::GetSelectionStart() const
  {
    return std::min(_caret, _anchor);
  }

  std::size_t UiTextEditor::GetSelectionEnd() const
  {
    return std::max(_caret, _anchor);
  }

  std::string UiTextEditor::GetSelectedText() const
  {
    return _text.substr(GetSelectionStart(), GetSelectionEnd() - GetSelectionStart());
  }

  void UiTextEditor::SetCaret(const std::size_t offset, const bool selecting)
  {
    _caret = UiTextBoundaries::Snap(_text, std::min(offset, _text.size()));
    if (!selecting) { _anchor = _caret; }

    _is_typing = false;
  }

  void UiTextEditor::Select(const std::size_t start, const std::size_t end)
  {
    _anchor = UiTextBoundaries::Snap(_text, std::min(start, _text.size()));
    _caret = UiTextBoundaries::Snap(_text, std::min(end, _text.size()));
    _is_typing = false;
  }

  void UiTextEditor::SelectAll()
  {
    Select(0, _text.size());
  }

  void UiTextEditor::SelectWordAt(const std::size_t offset)
  {
    std::size_t start = 0;
    std::size_t end = 0;
    UiTextBoundaries::WordAt(_text, offset, start, end);
    Select(start, end);
  }

  void UiTextEditor::SelectLineAt(const std::size_t offset)
  {
    std::size_t start = 0;
    std::size_t end = 0;
    UiTextBoundaries::LineAt(_text, offset, start, end);
    Select(start, end);
  }

  void UiTextEditor::MoveLeft(const bool selecting, const bool by_word)
  {
    // without shift, what is selected is left at its start
    if (!selecting && HasSelection() && !by_word)
    {
      SetCaret(GetSelectionStart(), false);
      return;
    }

    SetCaret(
      by_word ? UiTextBoundaries::PreviousWord(_text, _caret) : UiTextBoundaries::Previous(_text, _caret),
      selecting);
  }

  void UiTextEditor::MoveRight(const bool selecting, const bool by_word)
  {
    if (!selecting && HasSelection() && !by_word)
    {
      SetCaret(GetSelectionEnd(), false);
      return;
    }

    SetCaret(
      by_word ? UiTextBoundaries::NextWord(_text, _caret) : UiTextBoundaries::Next(_text, _caret),
      selecting);
  }

  void UiTextEditor::MoveToLineStart(const bool selecting)
  {
    std::size_t start = 0;
    std::size_t end = 0;
    UiTextBoundaries::LineAt(_text, _caret, start, end);
    SetCaret(start, selecting);
  }

  void UiTextEditor::MoveToLineEnd(const bool selecting)
  {
    std::size_t start = 0;
    std::size_t end = 0;
    UiTextBoundaries::LineAt(_text, _caret, start, end);
    SetCaret(end, selecting);
  }

  void UiTextEditor::MoveToStart(const bool selecting)
  {
    SetCaret(0, selecting);
  }

  void UiTextEditor::MoveToEnd(const bool selecting)
  {
    SetCaret(_text.size(), selecting);
  }

  void UiTextEditor::Remember(const bool typing)
  {
    // what is typed letter by letter is one step, until something else
    // happens
    if (typing && _is_typing && !_undo.empty())
    {
      _redo.clear();
      return;
    }

    _undo.push_back({_text, _caret, _anchor});
    if (_undo.size() > max_history) { _undo.erase(_undo.begin()); }

    _redo.clear();
    _is_typing = typing;
  }

  std::string UiTextEditor::Filtered(const std::string &text) const
  {
    std::string filtered;

    for (std::size_t at = 0; at < text.size();)
    {
      const std::size_t next = UiTextBoundaries::Next(text, at);
      const std::string cluster = text.substr(at, next - at);
      at = next;

      const auto first = static_cast<unsigned char>(cluster[0]);

      if (cluster == "\n" || cluster == "\r\n" || cluster == "\r")
      {
        // a line feed is one, however it was written
        if (_is_multiline) { filtered += '\n'; }
        continue;
      }

      if (cluster == "\t")
      {
        if (_is_multiline) { filtered += '\t'; }
        continue;
      }

      // what draws nothing and does nothing
      if (first < 0x20 || first == 0x7F) { continue; }

      if (_kind == Kind::Number)
      {
        const bool is_part = cluster.size() == 1 &&
                             ((cluster[0] >= '0' && cluster[0] <= '9') || cluster[0] == '-' ||
                              cluster[0] == '+' || cluster[0] == '.');
        if (!is_part) { continue; }
      }

      filtered += cluster;
    }

    return filtered;
  }

  void UiTextEditor::Replace(const std::size_t start, const std::size_t end, const std::string &text)
  {
    _text = _text.substr(0, start) + text + _text.substr(end);
    _caret = start + text.size();
    _anchor = _caret;
  }

  bool UiTextEditor::Insert(const std::string &text)
  {
    if (_is_read_only) { return false; }

    std::string inserted = Filtered(text);

    if (_max_length > 0)
    {
      // what is selected makes room
      const std::size_t kept = UiTextBoundaries::Count(_text) - UiTextBoundaries::Count(GetSelectedText());
      const std::size_t room = _max_length > kept ? _max_length - kept : 0;

      std::size_t end = 0;
      for (std::size_t count = 0; count < room && end < inserted.size(); count++)
      {
        end = UiTextBoundaries::Next(inserted, end);
      }

      inserted = inserted.substr(0, end);
    }

    if (inserted.empty() && !HasSelection()) { return false; }

    // a word is undone as a whole, and the space behind it starts another
    const bool is_typed = UiTextBoundaries::Count(inserted) == 1 && !HasSelection();
    const bool ends_a_word = inserted == " " || inserted == "\n";

    Remember(is_typed && !ends_a_word);

    Replace(GetSelectionStart(), GetSelectionEnd(), inserted);
    _composition.clear();

    if (ends_a_word) { _is_typing = false; }
    return true;
  }

  bool UiTextEditor::Backspace(const bool by_word)
  {
    if (_is_read_only) { return false; }

    std::size_t start = GetSelectionStart();
    const std::size_t end = GetSelectionEnd();

    if (!HasSelection())
    {
      if (_caret == 0) { return false; }

      start = by_word ? UiTextBoundaries::PreviousWord(_text, _caret) : UiTextBoundaries::Previous(_text, _caret);
    }

    Remember(false);
    Replace(start, end, "");
    return true;
  }

  bool UiTextEditor::Delete(const bool by_word)
  {
    if (_is_read_only) { return false; }

    const std::size_t start = GetSelectionStart();
    std::size_t end = GetSelectionEnd();

    if (!HasSelection())
    {
      if (_caret >= _text.size()) { return false; }

      end = by_word ? UiTextBoundaries::NextWord(_text, _caret) : UiTextBoundaries::Next(_text, _caret);
    }

    Remember(false);
    Replace(start, end, "");
    return true;
  }

  bool UiTextEditor::Copy(ClipboardContext &clipboard) const
  {
    if (!HasSelection() || _kind == Kind::Password) { return false; }

    return clipboard.SetText(GetSelectedText());
  }

  bool UiTextEditor::Cut(ClipboardContext &clipboard)
  {
    if (_is_read_only || !Copy(clipboard)) { return false; }

    Remember(false);
    Replace(GetSelectionStart(), GetSelectionEnd(), "");
    return true;
  }

  bool UiTextEditor::Paste(ClipboardContext &clipboard)
  {
    if (_is_read_only || !clipboard.HasText()) { return false; }

    // one step, however long
    _is_typing = false;
    const bool changed = Insert(clipboard.GetText());
    _is_typing = false;
    return changed;
  }

  bool UiTextEditor::CanUndo() const
  {
    return !_undo.empty();
  }

  bool UiTextEditor::CanRedo() const
  {
    return !_redo.empty();
  }

  bool UiTextEditor::Undo()
  {
    if (_is_read_only || _undo.empty()) { return false; }

    _redo.push_back({_text, _caret, _anchor});

    const Snapshot before = _undo.back();
    _undo.pop_back();

    _text = before.text;
    _caret = before.caret;
    _anchor = before.anchor;
    _composition.clear();
    _is_typing = false;
    return true;
  }

  bool UiTextEditor::Redo()
  {
    if (_is_read_only || _redo.empty()) { return false; }

    _undo.push_back({_text, _caret, _anchor});

    const Snapshot after = _redo.back();
    _redo.pop_back();

    _text = after.text;
    _caret = after.caret;
    _anchor = after.anchor;
    _composition.clear();
    _is_typing = false;
    return true;
  }

  void UiTextEditor::SetComposition(const std::string &text, const int caret)
  {
    if (_is_read_only)
    {
      _composition.clear();
      return;
    }

    _composition = text;

    // the input method counts in characters
    std::size_t offset = 0;
    for (int i = 0; i < caret && offset < text.size(); i++)
    {
      std::size_t length = 0;
      (void) CharacterAt(text, offset, length);
      offset += length;
    }

    _composition_caret = UiTextBoundaries::Snap(text, offset);
  }

  const std::string &UiTextEditor::GetComposition() const
  {
    return _composition;
  }

  std::size_t UiTextEditor::GetCompositionCaret() const
  {
    return std::min(_composition_caret, _composition.size());
  }

  std::string UiTextEditor::GetShownText(
    std::size_t &caret,
    std::size_t &selection_start,
    std::size_t &selection_end) const
  {
    // what is put together stands where what is selected stood
    const std::size_t start = GetSelectionStart();
    const std::size_t end = _composition.empty() ? start : GetSelectionEnd();

    const auto shown = [this](const std::string &text)
    {
      if (_kind != Kind::Password) { return text; }

      std::string hidden;
      for (std::size_t i = 0; i < UiTextBoundaries::Count(text); i++) { hidden += bullet; }
      return hidden;
    };

    if (!_composition.empty())
    {
      const std::string before = shown(_text.substr(0, start));
      const std::string composed = _composition;
      const std::string after = shown(_text.substr(end));

      caret = before.size() + GetCompositionCaret();
      selection_start = before.size();
      selection_end = before.size() + composed.size();
      return before + composed + after;
    }

    caret = shown(_text.substr(0, _caret)).size();
    selection_start = shown(_text.substr(0, GetSelectionStart())).size();
    selection_end = shown(_text.substr(0, GetSelectionEnd())).size();
    return shown(_text);
  }

  std::size_t UiTextEditor::FromShown(const std::size_t shown_offset) const
  {
    if (_kind != Kind::Password)
    {
      return UiTextBoundaries::Snap(_text, std::min(shown_offset, _text.size()));
    }

    // a bullet for every cluster
    const std::size_t clusters = shown_offset / bullet.size();

    std::size_t offset = 0;
    for (std::size_t i = 0; i < clusters && offset < _text.size(); i++)
    {
      offset = UiTextBoundaries::Next(_text, offset);
    }
    return offset;
  }
} // neon
