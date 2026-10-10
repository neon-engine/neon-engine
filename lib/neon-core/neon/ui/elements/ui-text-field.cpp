#include "ui-text-field.hpp"

#include <algorithm>
#include <cmath>
#include <format>

#include <neon/text/utf8.hpp>
#include <neon/ui/ui-call-field.hpp>
#include <neon/ui/ui-fields.hpp>

#include "ui-binding.hpp"

namespace neon
{
  // Helpers of UiTextField, for this file alone.
  namespace
  {
    // seconds the caret is seen, and then not seen
    constexpr float blink_time = 0.5f;

    /// Whether a text fits a pattern with `*`, `?`, and `[...]`.
    bool Fits(const std::u32string &pattern, const std::size_t p, const std::u32string &text, const std::size_t t)
    {
      if (p == pattern.size()) { return t == text.size(); }

      const char32_t token = pattern[p];

      if (token == U'*')
      {
        for (std::size_t k = t; k <= text.size(); k++)
        {
          if (Fits(pattern, p + 1, text, k)) { return true; }
        }
        return false;
      }

      if (t == text.size()) { return false; }

      if (token == U'?') { return Fits(pattern, p + 1, text, t + 1); }

      if (token == U'[')
      {
        const std::size_t close = pattern.find(U']', p + 1);
        if (close == std::u32string::npos) { return false; }

        bool is_in = false;
        for (std::size_t k = p + 1; k < close; k++)
        {
          if (k + 2 < close && pattern[k + 1] == U'-')
          {
            if (text[t] >= pattern[k] && text[t] <= pattern[k + 2]) { is_in = true; }
            k += 2;
          } else if (pattern[k] == text[t])
          {
            is_in = true;
          }
        }

        return is_in && Fits(pattern, close + 1, text, t + 1);
      }

      if (token == U'\\' && p + 1 < pattern.size())
      {
        return pattern[p + 1] == text[t] && Fits(pattern, p + 2, text, t + 1);
      }

      return token == text[t] && Fits(pattern, p + 1, text, t + 1);
    }
  }

  UiTextField::UiTextField() = default;

  UiTextEditor &UiTextField::GetEditor()
  {
    return _editor;
  }

  const UiTextEditor &UiTextField::GetEditor() const
  {
    return _editor;
  }

  bool UiTextField::SubmitsOnEnter() const
  {
    return true;
  }

  float UiTextField::WrapWidthOf(const UiRectangle &content_box) const
  {
    (void) content_box;
    return std::numeric_limits<float>::quiet_NaN();
  }

  void UiTextField::ApplyDefaults(UiStyle &style) const
  {
    style.pointer_events = UiPointerEvents::Auto;
    style.cursor = UiCursor::Text;

    style.layout.padding = {
      LayoutLength::Pixels(6.0f), LayoutLength::Pixels(10.0f), LayoutLength::Pixels(6.0f), LayoutLength::Pixels(10.0f)
    };
    style.layout.min_width = LayoutLength::Pixels(160.0f);
    style.layout.border = {1.0f, 1.0f, 1.0f, 1.0f};
    style.layout.flex_shrink = 0.0f;

    // what is typed is cut off at the box
    style.overflow_x = UiOverflow::Hidden;
    style.overflow_y = UiOverflow::Hidden;

    style.background_color = {1.0f, 1.0f, 1.0f, 0.08f};
    style.border_color = Color{1.0f, 1.0f, 1.0f, 0.3f};
    style.color = {1.0f, 1.0f, 1.0f, 1.0f};
    style.text_align = TextAlign::Left;
  }

  void UiTextField::ApplyStateDefaults(UiStyle &style, const UiStates &states) const
  {
    if (states.disabled)
    {
      style.opacity = 0.5f;
      return;
    }

    if (states.focus)
    {
      style.border_color = Color{1.0f, 0.820f, 0.400f, 1.0f}; // #ffd166
    }
  }

  void UiTextField::ApplyPartDefaults(const std::string &part, UiStyle &style) const
  {
    UiElement::ApplyPartDefaults(part, style);

    const Color &text = GetStyle().color;

    if (part == "placeholder")
    {
      style.color = {text.r, text.g, text.b, text.a * 0.5f};
    } else if (part == "selection")
    {
      style.background_color = {0.2f, 0.56f, 1.0f, 0.5f};
    }
  }

  void UiTextField::ReadAttributes(const DataReader &reader)
  {
    if (const auto *value = reader.ReadValue("value"); value != nullptr)
    {
      std::string text;
      if (UiBinding::Read(*value, text, _binding))
      {
        _editor.SetText(text);
      } else
      {
        reader.Report(*value, std::format(
                        "'value' of {} is {}, where text or a value such as \"{{player_name}}\" was expected",
                        reader.GetWhere(), DataValue::Describe(value->GetKind())));
      }
    }

    reader.Read("placeholder", _placeholder);

    if (int max_length = 0; reader.Read("max_length", max_length))
    {
      if (max_length >= 0)
      {
        _editor.SetMaxLength(static_cast<std::size_t>(max_length));
      } else
      {
        reader.Report(*reader.ReadValue("max_length"), std::format(
                        "'max_length' of {} is {}, where a whole number that is not below 0 was expected",
                        reader.GetWhere(), max_length));
      }
    }

    if (bool read_only = false; reader.Read("read_only", read_only)) { _editor.SetReadOnly(read_only); }

    reader.Read("pattern", _pattern);
    reader.Read("autofocus", _autofocus);
    UiCallField::Read(reader, "on_change", _on_change);

    if (const auto *value = reader.ReadValue("enabled"); value != nullptr && !UiFlag::Read(*value, _enabled))
    {
      std::string text;
      reader.Report(*value, std::format(
                      "'enabled' of {} is {}, where true, false, or a value such as \"{{can_type}}\" was expected",
                      reader.GetWhere(),
                      value->GetText(text) ? "'" + text + "'" : DataValue::Describe(value->GetKind())));
    }
  }

  bool UiTextField::IsFocusable() const
  {
    return true;
  }

  bool UiTextField::WantsFocus() const
  {
    return _autofocus;
  }

  bool UiTextField::IsEnabled() const
  {
    return _is_enabled;
  }

  bool UiTextField::TakesText() const
  {
    return true;
  }

  bool UiTextField::UsesDirection(const Key direction) const
  {
    // the arrows move the caret. Up and down move it in a text of several
    // lines, and to the ends of a text of one
    return direction == Key::Left || direction == Key::Right || direction == Key::Up || direction == Key::Down;
  }

  bool UiTextField::HasContent() const
  {
    return true;
  }

  bool UiTextField::TellsWhenItChanged() const
  {
    return true;
  }

  const UiCall *UiTextField::CallsWhenChanged() const
  {
    return _on_change.IsEmpty() ? nullptr : &_on_change;
  }

  bool UiTextField::IsInvalid() const
  {
    return !IsValid();
  }

  const UiFont *UiTextField::FontOf(const UiFrame &frame) const
  {
    const UiStyle &style = GetStyle();
    const int pixel_size = std::max(1, static_cast<int>(std::round(style.font_size * frame.scale)));
    return frame.resources->GetFont(style.font_family, style.font_weight, pixel_size);
  }

  UiTextMeasure::Request UiTextField::RequestOf(const UiFrame &frame, const std::string &shown) const
  {
    UiTextMeasure::Request request;
    request.font = FontOf(frame);
    request.text = shown;
    request.options.line_height = GetStyle().LineHeight() * frame.scale;
    request.options.max_width = WrapWidthOf(ToPixels(GetContentBox(), frame.scale));
    request.breaks_long_words = _editor.IsMultiline();
    return request;
  }

  const UiTextMeasure &UiTextField::MeasureOf(const UiFrame &frame) const
  {
    return frame.text_measure != nullptr ? *frame.text_measure : Atlas_UiTextMeasure::Get();
  }

  LayoutSize UiTextField::Measure(const UiFrame &frame, const float available_width, const float available_height)
  {
    (void) available_width;
    (void) available_height;

    const UiFont *font = FontOf(frame);
    if (font == nullptr) { return {0.0f, GetStyle().font_size * 1.25f}; }

    TextOptions options;
    options.line_height = GetStyle().LineHeight() * frame.scale;

    // one line, and as wide as the least width asks for
    return {0.0f, LineHeightOf(font->atlas, options) / frame.scale};
  }

  void UiTextField::Update(const UiFrame &frame)
  {
    _is_enabled = _enabled.Get(*frame.values, true);

    if (_binding.empty()) { return; }

    // what the game set, unless nothing changed since it was last taken
    if (_has_followed && _followed_revision == frame.values->GetRevision()) { return; }

    _has_followed = true;
    _followed_revision = frame.values->GetRevision();

    const UiValue *value = frame.values->Find(_binding);
    if (value == nullptr) { return; }

    if (_editor.SetText(value->AsText()))
    {
      Invalidate(UiDirty::Arrange | UiDirty::Paint);
    }
  }

  std::size_t UiTextField::OffsetAt(const UiFrame &frame, const float x, const float y) const
  {
    std::size_t caret = 0;
    std::size_t start = 0;
    std::size_t end = 0;
    const std::string shown = _editor.GetShownText(caret, start, end);

    const UiRectangle content = ToPixels(GetContentBox(), frame.scale);
    const float at_x = x * frame.scale - content.left + _shift;
    const float at_y = y * frame.scale - content.top;

    return _editor.FromShown(MeasureOf(frame).OffsetAt(RequestOf(frame, shown), at_x, at_y));
  }

  void UiTextField::MoveVertically(const UiFrame &frame, const bool up, const bool selecting)
  {
    std::size_t caret = 0;
    std::size_t start = 0;
    std::size_t end = 0;
    const std::string shown = _editor.GetShownText(caret, start, end);

    const UiTextMeasure &measure = MeasureOf(frame);
    const auto request = RequestOf(frame, shown);
    const UiTextMeasure::Caret at = measure.CaretAt(request, caret);

    // where the caret was before a short line moved it to the left
    if (std::isnan(_wanted_x)) { _wanted_x = at.x; }
    const float x = _wanted_x;

    // the same place, a line up or down
    const float y = at.y + (up ? -0.5f : 1.5f) * std::max(1.0f, at.height);
    if (y < 0.0f)
    {
      _editor.MoveToStart(selecting);
      _wanted_x = std::numeric_limits<float>::quiet_NaN();
      return;
    }

    const auto lines = measure.LinesOf(request);
    if (!lines.empty() && y >= lines.back().top + lines.back().height)
    {
      _editor.MoveToEnd(selecting);
      _wanted_x = std::numeric_limits<float>::quiet_NaN();
      return;
    }

    _editor.SetCaret(_editor.FromShown(measure.OffsetAt(request, x, y)), selecting);
    _wanted_x = x;
  }

  void UiTextField::KeepCaretInView(const UiFrame &frame)
  {
    std::size_t caret = 0;
    std::size_t start = 0;
    std::size_t end = 0;
    const std::string shown = _editor.GetShownText(caret, start, end);

    const UiTextMeasure::Caret at = MeasureOf(frame).CaretAt(RequestOf(frame, shown), caret);
    const UiRectangle content = ToPixels(GetContentBox(), frame.scale);

    if (_editor.IsMultiline())
    {
      // scrolled, as everything is that is larger than its box: once the
      // box knows how large the text is, which is when it is drawn
      const LayoutSize size = MeasureOf(frame).SizeOf(RequestOf(frame, shown));
      _content_width = size.width / frame.scale;
      _content_height = size.height / frame.scale;
      _scrolls_to_caret = true;
      Invalidate(UiDirty::Arrange | UiDirty::Paint);
      return;
    }

    // moved to the left, so that the caret is seen
    const float width = content.Width();
    if (at.x - _shift > width) { _shift = at.x - width; }
    if (at.x - _shift < 0.0f) { _shift = at.x; }

    _shift = std::max(0.0f, std::round(_shift));
  }

  void UiTextField::Changed(const UiFrame &frame)
  {
    _caret_is_shown = true;
    _blink = 0.0f;
    KeepCaretInView(frame);

    Invalidate(UiDirty::Arrange | UiDirty::Paint | (_pattern.empty() ? 0u : UiDirty::Style));
    // only the player types, and the game puts its text in with SetText()
    Notify("changed", _editor.GetText(), _binding, UiValue::Text(_editor.GetText()), true);
  }

  void UiTextField::HandleKey(UiInteraction &interaction, const UiFrame &frame)
  {
    const KeyModifiers &modifiers = interaction.modifiers;
    const bool selecting = modifiers.shift;
    bool changed = false;
    bool moved = true;

    // anything but up and down forgets where the caret wanted to be
    if (interaction.key != Key::Up && interaction.key != Key::Down)
    {
      _wanted_x = std::numeric_limits<float>::quiet_NaN();
    }

    switch (interaction.key)
    {
      case Key::Left:
        _editor.MoveLeft(selecting, modifiers.word);
        break;
      case Key::Right:
        _editor.MoveRight(selecting, modifiers.word);
        break;
      case Key::Up:
        if (_editor.IsMultiline()) { MoveVertically(frame, true, selecting); }
        else { _editor.MoveToStart(selecting); }
        break;
      case Key::Down:
        if (_editor.IsMultiline()) { MoveVertically(frame, false, selecting); }
        else { _editor.MoveToEnd(selecting); }
        break;
      case Key::Home:
        if (modifiers.shortcut || modifiers.control) { _editor.MoveToStart(selecting); }
        else { _editor.MoveToLineStart(selecting); }
        break;
      case Key::End:
        if (modifiers.shortcut || modifiers.control) { _editor.MoveToEnd(selecting); }
        else { _editor.MoveToLineEnd(selecting); }
        break;
      case Key::Backspace:
        changed = _editor.Backspace(modifiers.word);
        break;
      case Key::Delete:
        changed = _editor.Delete(modifiers.word);
        break;
      case Key::Enter:
        if (SubmitsOnEnter() || modifiers.shortcut)
        {
          Notify("submitted", _editor.GetText(), _binding, UiValue::Text(_editor.GetText()));
          moved = false;
        } else
        {
          changed = _editor.Insert("\n");
        }
        break;
      case Key::A:
        if (!modifiers.shortcut) { return; }
        _editor.SelectAll();
        break;
      case Key::C:
        if (!modifiers.shortcut) { return; }
        if (interaction.clipboard != nullptr) { (void) _editor.Copy(*interaction.clipboard); }
        moved = false;
        break;
      case Key::X:
        if (!modifiers.shortcut) { return; }
        if (interaction.clipboard != nullptr) { changed = _editor.Cut(*interaction.clipboard); }
        break;
      case Key::V:
        if (!modifiers.shortcut) { return; }
        if (interaction.clipboard != nullptr) { changed = _editor.Paste(*interaction.clipboard); }
        break;
      case Key::Z:
        if (!modifiers.shortcut) { return; }
        changed = modifiers.shift ? _editor.Redo() : _editor.Undo();
        break;
      case Key::Y:
        if (!modifiers.shortcut) { return; }
        changed = _editor.Redo();
        break;
      default:
        // the tab key moves the focus, and escape is cancel
        return;
    }

    interaction.is_used = true;

    if (changed)
    {
      Changed(frame);
    } else if (moved)
    {
      _caret_is_shown = true;
      _blink = 0.0f;
      KeepCaretInView(frame);
      Invalidate(UiDirty::Paint);
    }
  }

  void UiTextField::Interact(UiInteraction &interaction, const UiFrame &frame)
  {
    switch (interaction.kind)
    {
      case UiInteraction::Kind::PointerDown:
      {
        const std::size_t offset = OffsetAt(frame, interaction.x, interaction.y);
        _wanted_x = std::numeric_limits<float>::quiet_NaN();

        if (interaction.clicks >= 3) { _editor.SelectLineAt(offset); }
        else if (interaction.clicks == 2) { _editor.SelectWordAt(offset); }
        else { _editor.SetCaret(offset, interaction.modifiers.shift); }

        _is_selecting = interaction.clicks == 1;
        _caret_is_shown = true;
        _blink = 0.0f;
        interaction.is_used = true;
        Invalidate(UiDirty::Paint);
        break;
      }

      case UiInteraction::Kind::PointerMove:
        if (!_is_selecting) { break; }

        _editor.SetCaret(OffsetAt(frame, interaction.x, interaction.y), true);
        KeepCaretInView(frame);
        interaction.is_used = true;
        Invalidate(UiDirty::Paint);
        break;

      case UiInteraction::Kind::PointerUp:
        _is_selecting = false;
        interaction.is_used = true;
        break;

      case UiInteraction::Kind::KeyDown:
        HandleKey(interaction, frame);
        break;

      case UiInteraction::Kind::Text:
        _wanted_x = std::numeric_limits<float>::quiet_NaN();
        if (_editor.Insert(interaction.text)) { Changed(frame); }
        interaction.is_used = true;
        break;

      case UiInteraction::Kind::Composition:
        _editor.SetComposition(interaction.text, interaction.composition_cursor);
        KeepCaretInView(frame);
        interaction.is_used = true;
        Invalidate(UiDirty::Paint);
        break;

      case UiInteraction::Kind::Direction:
      {
        // from a controller, as the arrows of the keyboard
        UiInteraction key = interaction;
        key.kind = UiInteraction::Kind::KeyDown;
        HandleKey(key, frame);
        interaction.is_used = key.is_used;
        break;
      }

      case UiInteraction::Kind::Accept:
        // a click, which the pointer did already
        break;

      case UiInteraction::Kind::Focus:
        _has_focus = true;
        _caret_is_shown = true;
        _blink = 0.0f;
        Invalidate(UiDirty::Tick | UiDirty::Paint);
        break;

      case UiInteraction::Kind::Blur:
        _has_focus = false;
        _is_selecting = false;
        _editor.SetComposition("", 0);
        Invalidate(UiDirty::Paint);
        break;

      default:
        break;
    }
  }

  bool UiTextField::GetCaretBox(const UiFrame &frame, UiRectangle &box) const
  {
    std::size_t caret = 0;
    std::size_t start = 0;
    std::size_t end = 0;
    const std::string shown = _editor.GetShownText(caret, start, end);

    const UiTextMeasure::Caret at = MeasureOf(frame).CaretAt(RequestOf(frame, shown), caret);
    const UiRectangle content = ToPixels(GetContentBox(), frame.scale);

    box.left = (content.left + at.x - _shift) / frame.scale;
    box.top = (content.top + at.y) / frame.scale - GetScrollY();
    box.right = box.left + 1.0f;
    box.bottom = box.top + at.height / frame.scale;
    return true;
  }

  bool UiTextField::Tick(const UiFrame &frame, const float seconds)
  {
    (void) frame;
    if (!_has_focus) { return false; }

    _blink += seconds;
    if (_blink >= blink_time)
    {
      _blink = std::fmod(_blink, blink_time);
      _caret_is_shown = !_caret_is_shown;
      Invalidate(UiDirty::Paint);
    }

    return true;
  }

  bool UiTextField::GetContentSize(float &width, float &height) const
  {
    if (!_editor.IsMultiline()) { return false; }

    width = _content_width;
    height = _content_height;
    return true;
  }

  void UiTextField::PaintContent(
    UiPainter &painter,
    const UiFrame &frame,
    const UiRectangle &content_box,
    const float opacity)
  {
    const UiFont *font = FontOf(frame);
    if (font == nullptr) { return; }

    const UiStyle &style = GetStyle();

    std::size_t caret = 0;
    std::size_t selection_start = 0;
    std::size_t selection_end = 0;
    const std::string shown = _editor.GetShownText(caret, selection_start, selection_end);

    const UiTextMeasure &measure = MeasureOf(frame);
    const auto request = RequestOf(frame, shown);

    // how large the text is, for what scrolls
    const LayoutSize size = measure.SizeOf(request);
    const float content_width = size.width / frame.scale;
    const float content_height = size.height / frame.scale;

    if (content_width != _content_width || content_height != _content_height)
    {
      _content_width = content_width;
      _content_height = content_height;
      if (_editor.IsMultiline()) { Invalidate(UiDirty::Arrange); }
    }

    // where the text starts: moved for the caret to be seen
    if (!_editor.IsMultiline())
    {
      const UiTextMeasure::Caret at = measure.CaretAt(request, caret);
      const float width = content_box.Width();
      if (at.x - _shift > width) { _shift = at.x - width; }
      if (at.x - _shift < 0.0f) { _shift = at.x; }
      _shift = std::max(0.0f, std::min(_shift, std::max(0.0f, size.width - width)));
      _shift = std::round(_shift);
    }

    const float left = content_box.left - _shift;
    float top = content_box.top;

    if (_scrolls_to_caret)
    {
      _scrolls_to_caret = false;

      const UiTextMeasure::Caret at = measure.CaretAt(request, caret);
      const float caret_top = at.y / frame.scale;
      const float caret_bottom = (at.y + at.height) / frame.scale;
      const float room = GetContentBox().Height();
      const float before = GetScrollY();

      if (caret_top < before)
      {
        SetScroll(GetScrollX(), caret_top);
      } else if (caret_bottom > before + room)
      {
        SetScroll(GetScrollX(), caret_bottom - room);
      }

      // drawn where it will be, and not a frame late
      top -= std::round((GetScrollY() - before) * frame.scale);
    }

    // what is shown while nothing is typed
    if (shown.empty() && !_placeholder.empty())
    {
      const UiStyle &placeholder = GetPartStyle("placeholder");

      TextOptions options = request.options;
      options.box_width = content_box.Width();
      options.align = style.text_align;

      const PlacedText placed = PlaceText(font->atlas, DecodeUtf8(_placeholder), options);
      const Color &color = placeholder.color;
      painter.DrawText(*font, placed, content_box.left, top, {color.r, color.g, color.b, color.a * opacity});
    }

    // what is selected, a line at a time
    if (selection_start != selection_end && (_has_focus || !_editor.GetComposition().empty()))
    {
      const UiStyle &selection = GetPartStyle("selection");
      const Color &color = selection.background_color;

      for (const auto &line : measure.LinesOf(request))
      {
        const std::size_t start = std::max(selection_start, line.start);
        const std::size_t end = std::min(selection_end, line.end);
        if (start > end || (start == end && !(selection_start < line.start && selection_end > line.end)))
        {
          continue;
        }

        const UiTextMeasure::Caret from = measure.CaretAt(request, start);
        const UiTextMeasure::Caret to = measure.CaretAt(request, end);

        // a line that is selected as a whole shows that it ends
        const float right = end == line.end && selection_end > line.end
          ? std::max(to.x, from.x) + std::round(4.0f * frame.scale)
          : to.x;

        painter.FillRectangle(
          {left + from.x, top + line.top, left + right, top + line.top + line.height},
          {color.r, color.g, color.b, color.a * opacity});
      }
    }

    // the text itself, a line at a time, as the measure broke them
    if (!shown.empty())
    {
      TextOptions options = request.options;
      options.max_width = std::numeric_limits<float>::quiet_NaN();
      options.box_width = std::max(content_box.Width(), size.width);

      const Color color{style.color.r, style.color.g, style.color.b, style.color.a * opacity};

      for (const auto &line : measure.LinesOf(request))
      {
        if (line.start >= line.end) { continue; }

        const PlacedText placed = PlaceText(
          font->atlas, DecodeUtf8(shown.substr(line.start, line.end - line.start)), options);
        painter.DrawText(*font, placed, left, top + line.top, color);
      }
    }

    // what an input method puts together is underlined
    if (!_editor.GetComposition().empty())
    {
      const UiTextMeasure::Caret from = measure.CaretAt(request, selection_start);
      const UiTextMeasure::Caret to = measure.CaretAt(request, selection_end);

      if (from.line == to.line)
      {
        const float thickness = std::max(1.0f, std::round(frame.scale));
        painter.FillRectangle(
          {left + from.x, top + from.y + from.height - thickness, left + to.x, top + from.y + from.height},
          {style.color.r, style.color.g, style.color.b, style.color.a * opacity});
      }
    }

    // the caret
    if (_has_focus && _caret_is_shown && !_editor.IsReadOnly())
    {
      const UiTextMeasure::Caret at = measure.CaretAt(request, caret);
      const float width = std::max(1.0f, std::round(frame.scale));
      const Color color = style.CaretColor();

      painter.FillRectangle(
        {left + at.x, top + at.y, left + at.x + width, top + at.y + at.height},
        {color.r, color.g, color.b, color.a * opacity});
    }
  }

  bool UiTextField::GetField(const std::string &name, FieldValue &value) const
  {
    if (name == "value")
    {
      value = _editor.GetText();
      return true;
    }

    if (name == "placeholder")
    {
      value = _placeholder;
      return true;
    }

    if (name == "max_length")
    {
      value = static_cast<int>(_editor.GetMaxLength());
      return true;
    }

    if (name == "read_only")
    {
      value = _editor.IsReadOnly();
      return true;
    }

    if (name == "enabled")
    {
      value = _is_enabled;
      return true;
    }

    if (name == "pattern")
    {
      value = _pattern;
      return true;
    }

    if (name == "valid")
    {
      value = IsValid();
      return true;
    }

    if (name == "autofocus")
    {
      value = _autofocus;
      return true;
    }

    if (name == "on_change")
    {
      value = _on_change.AsText();
      return true;
    }

    return false;
  }

  bool UiTextField::SetField(const std::string &name, const FieldValue &value, std::string &error)
  {
    const std::string what = "'" + name + "' of " + Describe();

    if (name == "value")
    {
      std::string text;
      if (!TakeText(value, what, text, error)) { return false; }

      SetText(text);
      return true;
    }

    if (name == "placeholder")
    {
      if (!TakeText(value, what, _placeholder, error)) { return false; }
      Invalidate(UiDirty::Paint);
      return true;
    }

    if (name == "max_length")
    {
      int max_length = 0;
      if (!TakeWhole(value, what, max_length, error)) { return false; }

      if (max_length < 0)
      {
        error = what + " has to be at least 0";
        return false;
      }

      _editor.SetMaxLength(static_cast<std::size_t>(max_length));
      return true;
    }

    if (name == "read_only")
    {
      bool flag = false;
      if (!TakeFlag(value, what, flag, error)) { return false; }
      _editor.SetReadOnly(flag);
      Invalidate(UiDirty::Paint);
      return true;
    }

    if (name == "enabled")
    {
      bool flag = false;
      if (!TakeFlag(value, what, flag, error)) { return false; }
      _enabled = UiFlag(flag);
      _is_enabled = flag;
      return true;
    }

    if (name == "pattern")
    {
      if (!TakeText(value, what, _pattern, error)) { return false; }
      Invalidate(UiDirty::Style);
      return true;
    }

    if (name == "autofocus") { return TakeFlag(value, what, _autofocus, error); }

    if (name == "on_change") { return UiCallField::Set(value, what, _on_change, error); }

    if (name == "valid")
    {
      error = what + " follows from the text and its pattern, and cannot be set";
      return false;
    }

    return UiElement::SetField(name, value, error);
  }

  std::vector<UiElement::Field> UiTextField::GetFields() const
  {
    return {
      {"value", FieldKind::String, "The text, which may follow a value of the game as {name}", {}},
      {"placeholder", FieldKind::String, "What is shown while nothing is typed", {}},
      {"max_length", FieldKind::Integer, "How many characters may be typed. 0 for any number", {}},
      {"read_only", FieldKind::Boolean, "Whether the text can be selected and copied, and not changed", {}},
      {"enabled", FieldKind::Boolean, "Whether it can be typed into", {}},
      {"pattern", FieldKind::String, "What the text has to fit, with * for anything and ? for one character", {}},
      {"valid", FieldKind::Boolean, "Whether the text fits its pattern", {}},
      {"autofocus", FieldKind::Boolean, "Whether it has the focus when its file is shown", {}},
      {"on_change", FieldKind::String, "The function of the game that the player typing calls, with what it is handed: rename, or rename(3, $event)", {}}
    };
  }

  const std::string &UiTextField::GetText() const
  {
    return _editor.GetText();
  }

  void UiTextField::SetText(const std::string &text)
  {
    if (!_editor.SetText(text)) { return; }

    Invalidate(UiDirty::Arrange | UiDirty::Paint | UiDirty::Style);
  }

  const std::string &UiTextField::GetPlaceholder() const
  {
    return _placeholder;
  }

  bool UiTextField::IsValid() const
  {
    if (_pattern.empty()) { return true; }

    // an empty text fits every pattern, as a field that was not filled in
    if (_editor.GetText().empty()) { return true; }

    return Fits(DecodeUtf8(_pattern), 0, DecodeUtf8(_editor.GetText()), 0);
  }

  // one line

  void UiInput::ReadAttributes(const DataReader &reader)
  {
    UiTextField::ReadAttributes(reader);

    const std::vector<std::string> kinds = {"text", "password", "number"};
    if (std::size_t kind = 0; reader.ReadChoice("kind", kinds, kind))
    {
      GetEditor().SetKind(static_cast<UiTextEditor::Kind>(kind));
    }
  }

  bool UiInput::GetField(const std::string &name, FieldValue &value) const
  {
    if (name == "kind")
    {
      const std::vector<std::string> kinds = {"text", "password", "number"};
      value = kinds[static_cast<std::size_t>(GetEditor().GetKind())];
      return true;
    }

    return UiTextField::GetField(name, value);
  }

  bool UiInput::SetField(const std::string &name, const FieldValue &value, std::string &error)
  {
    if (name != "kind") { return UiTextField::SetField(name, value, error); }

    std::string kind;
    if (!TakeText(value, "'kind' of " + Describe(), kind, error)) { return false; }

    const std::vector<std::string> kinds = {"text", "password", "number"};
    const auto found = std::ranges::find(kinds, kind);
    if (found == kinds.end())
    {
      error = "'kind' of " + Describe() + " is '" + kind + "', where one of these was expected: text, password, number";
      return false;
    }

    GetEditor().SetKind(static_cast<UiTextEditor::Kind>(found - kinds.begin()));
    Invalidate(UiDirty::Paint);
    return true;
  }

  std::vector<UiElement::Field> UiInput::GetFields() const
  {
    auto fields = UiTextField::GetFields();
    fields.insert(fields.begin(), {"kind", FieldKind::Choice, "What is typed", {"text", "password", "number"}});
    return fields;
  }

  // several lines

  UiTextArea::UiTextArea()
  {
    GetEditor().SetMultiline(true);
  }

  bool UiTextArea::SubmitsOnEnter() const
  {
    return false;
  }

  float UiTextArea::WrapWidthOf(const UiRectangle &content_box) const
  {
    return std::max(0.0f, content_box.Width());
  }

  void UiTextArea::ApplyDefaults(UiStyle &style) const
  {
    UiTextField::ApplyDefaults(style);

    // what is typed is scrolled to
    style.overflow_y = UiOverflow::Auto;
    style.layout.min_width = LayoutLength::Pixels(240.0f);
  }

  void UiTextArea::ReadAttributes(const DataReader &reader)
  {
    UiTextField::ReadAttributes(reader);

    if (int rows = 0; reader.Read("rows", rows))
    {
      if (rows >= 1)
      {
        _rows = rows;
      } else
      {
        reader.Report(*reader.ReadValue("rows"), std::format(
                        "'rows' of {} is {}, where a whole number above 0 was expected", reader.GetWhere(), rows));
      }
    }
  }

  LayoutSize UiTextArea::Measure(const UiFrame &frame, const float available_width, const float available_height)
  {
    const LayoutSize line = UiTextField::Measure(frame, available_width, available_height);
    return {0.0f, line.height * static_cast<float>(_rows)};
  }

  bool UiTextArea::GetField(const std::string &name, FieldValue &value) const
  {
    if (name == "rows")
    {
      value = _rows;
      return true;
    }

    return UiTextField::GetField(name, value);
  }

  bool UiTextArea::SetField(const std::string &name, const FieldValue &value, std::string &error)
  {
    if (name != "rows") { return UiTextField::SetField(name, value, error); }

    int rows = 0;
    if (!TakeWhole(value, "'rows' of " + Describe(), rows, error)) { return false; }

    if (rows < 1)
    {
      error = "'rows' of " + Describe() + " has to be above 0";
      return false;
    }

    _rows = rows;
    Invalidate(UiDirty::Layout | UiDirty::Paint);
    return true;
  }

  std::vector<UiElement::Field> UiTextArea::GetFields() const
  {
    auto fields = UiTextField::GetFields();
    fields.insert(fields.begin(), {"rows", FieldKind::Integer, "How many lines are seen", {}});
    return fields;
  }
} // neon
