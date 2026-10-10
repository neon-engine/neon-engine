#include "ui-select.hpp"

#include <algorithm>
#include <cmath>
#include <format>

#include <neon/text/utf8.hpp>
#include <neon/ui/ui-fields.hpp>

#include "ui-binding.hpp"

namespace neon
{
  // Helpers of UiSelect, for this file alone.
  namespace
  {
    // the room around a text of the box and of a row, in units of the file
    constexpr float padding_x = 10.0f;
    constexpr float padding_y = 6.0f;

    // the arrow at the right, in units of the file
    constexpr float arrow_size = 8.0f;

    Color Faded(const Color &color, const float opacity)
    {
      return {color.r, color.g, color.b, color.a * opacity};
    }

    const UiFont *FontOf(const UiStyle &style, const UiFrame &frame)
    {
      const int pixel_size = std::max(1, static_cast<int>(std::round(style.font_size * frame.scale)));
      return frame.resources->GetFont(style.font_family, style.font_weight, pixel_size);
    }

    /// Draws one line of text in the middle of the height of a box, cut
    /// off at its right.
    void DrawLine(
      UiPainter &painter,
      const UiFrame &frame,
      const UiStyle &style,
      const std::string &text,
      const UiRectangle &box,
      const Color &color)
    {
      const UiFont *font = FontOf(style, frame);
      if (font == nullptr || text.empty()) { return; }

      TextOptions options;
      options.line_height = style.LineHeight() * frame.scale;
      options.box_width = box.Width();
      options.align = style.text_align;

      const PlacedText placed = PlaceText(font->atlas, DecodeUtf8(text), options);
      const float top = std::round(box.top + (box.Height() - placed.height) / 2.0f);

      painter.PushClip(box);
      painter.DrawText(*font, placed, box.left, top, color);
      painter.PopClip();
    }

    /// A small arrow that points down, made of rows that get narrower.
    void DrawArrow(UiPainter &painter, const float scale, const float right, const float middle, const Color &color)
    {
      const float size = std::round(arrow_size * scale);
      const float rows = std::max(1.0f, std::round(size / 2.0f));
      const float left = right - size;
      const float top = middle - std::round(rows / 2.0f);

      for (float row = 0.0f; row < rows; row += 1.0f)
      {
        const float inset = std::round(row * (size / 2.0f) / rows);
        painter.FillRectangle({left + inset, top + row, right - inset, top + row + 1.0f}, color);
      }
    }
  }

  float UiSelect::RowHeight() const
  {
    const float line = _line_height > 0.0f ? _line_height : GetStyle().font_size * 1.25f;
    return std::round(line + 2.0f * padding_y);
  }

  UiRectangle UiSelect::ListBox() const
  {
    const UiRectangle &box = GetBox();
    const float rows = static_cast<float>(std::min(_options.size(), shown_at_most));
    const float height = std::max(RowHeight(), rows * RowHeight());
    return {box.left, box.bottom, box.right, box.bottom + height};
  }

  void UiSelect::Open()
  {
    if (_is_open || _options.empty()) { return; }

    _is_open = true;
    _highlighted = _has_chosen ? _chosen : 0;
    ShowHighlighted();
    Invalidate(UiDirty::Paint);
  }

  void UiSelect::Close()
  {
    if (!_is_open) { return; }

    _is_open = false;
    Invalidate(UiDirty::Paint);
  }

  void UiSelect::Choose(const std::size_t index)
  {
    if (index >= _options.size()) { return; }
    if (_has_chosen && index == _chosen) { return; }

    _chosen = index;
    _has_chosen = true;
    Invalidate(UiDirty::Paint);

    Notify("changed", _options[index].value, _binding, UiValue::Text(_options[index].value));
  }

  void UiSelect::ShowHighlighted()
  {
    if (_highlighted < _first_shown) { _first_shown = _highlighted; }
    if (_highlighted >= _first_shown + shown_at_most) { _first_shown = _highlighted - shown_at_most + 1; }
  }

  std::size_t UiSelect::OptionAt(const float y) const
  {
    const UiRectangle list = ListBox();
    if (y < list.top || y >= list.bottom) { return _options.size(); }

    const auto row = static_cast<std::size_t>((y - list.top) / RowHeight());
    return std::min(_first_shown + row, _options.size());
  }

  std::size_t UiSelect::IndexOf(const std::string &value) const
  {
    for (std::size_t i = 0; i < _options.size(); i++)
    {
      if (_options[i].value == value) { return i; }
    }
    return _options.size();
  }

  void UiSelect::ApplyDefaults(UiStyle &style) const
  {
    style.pointer_events = UiPointerEvents::Auto;
    style.cursor = UiCursor::Pointer;
    style.layout.padding = {
      LayoutLength::Pixels(padding_y),
      LayoutLength::Pixels(padding_x),
      LayoutLength::Pixels(padding_y),
      LayoutLength::Pixels(padding_x)
    };
    style.layout.min_width = LayoutLength::Pixels(160.0f);
    style.layout.border = {1.0f, 1.0f, 1.0f, 1.0f};
    style.layout.flex_shrink = 0.0f;

    style.background_color = {1.0f, 1.0f, 1.0f, 0.08f};
    style.border_color = Color{1.0f, 1.0f, 1.0f, 0.3f};
    style.color = {1.0f, 1.0f, 1.0f, 1.0f};
    style.text_align = TextAlign::Left;
  }

  void UiSelect::ApplyStateDefaults(UiStyle &style, const UiStates &states) const
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

    if (states.hover) { style.background_color = {1.0f, 1.0f, 1.0f, 0.14f}; }
  }

  void UiSelect::ApplyPartDefaults(const std::string &part, UiStyle &style) const
  {
    UiElement::ApplyPartDefaults(part, style);

    const UiStyle &of_element = GetStyle();

    if (part == "list")
    {
      style.background_color = {0.180f, 0.204f, 0.251f, 1.0f}; // #2e3440
      style.border_color = Color{1.0f, 1.0f, 1.0f, 0.3f};
      style.layout.border = {1.0f, 1.0f, 1.0f, 1.0f};
    } else if (part == "option")
    {
      style.background_color = {0.0f, 0.0f, 0.0f, 0.0f};
      style.color = of_element.color;
    } else if (part == "highlight")
    {
      style.background_color = of_element.accent_color;
      style.color = {1.0f, 1.0f, 1.0f, 1.0f};
    } else if (part == "arrow")
    {
      style.background_color = {of_element.color.r, of_element.color.g, of_element.color.b, of_element.color.a * 0.7f};
    } else if (part == "placeholder")
    {
      style.color = {of_element.color.r, of_element.color.g, of_element.color.b, of_element.color.a * 0.5f};
    }
  }

  void UiSelect::ReadAttributes(const DataReader &reader)
  {
    reader.Read("autofocus", _autofocus);
    reader.Read("placeholder", _placeholder);

    if (const auto *options = reader.ReadValue("options"); options != nullptr)
    {
      if (options->GetKind() != DataValue::Kind::List)
      {
        reader.Report(*options, std::format(
                        "'options' of {} is {}, where a list was expected", reader.GetWhere(),
                        DataValue::Describe(options->GetKind())));
      } else
      {
        for (const DataValue &item : options->GetItems())
        {
          Option option;
          std::string binding;

          if (item.GetKind() == DataValue::Kind::Map)
          {
            const DataValue *value = item.Find("value");
            const DataValue *text = item.Find("text");

            if (value == nullptr || !UiBinding::Read(*value, option.value, binding) || !binding.empty())
            {
              reader.Report(item, std::format(
                              "an option of {} has no 'value', where the text it stands for was expected",
                              reader.GetWhere()));
              continue;
            }

            if (text == nullptr || !text->GetText(option.text)) { option.text = option.value; }
          } else if (UiBinding::Read(item, option.value, binding) && binding.empty())
          {
            option.text = option.value;
          } else
          {
            reader.Report(item, std::format(
                            "an option of {} is {}, where text or a map with 'value' and 'text' was expected",
                            reader.GetWhere(), DataValue::Describe(item.GetKind())));
            continue;
          }

          _options.push_back(option);
        }
      }
    }

    if (const auto *value = reader.ReadValue("value"); value != nullptr)
    {
      std::string text;
      if (!UiBinding::Read(*value, text, _binding))
      {
        reader.Report(*value, std::format(
                        "'value' of {} is {}, where text or a value such as \"{{quality}}\" was expected",
                        reader.GetWhere(), DataValue::Describe(value->GetKind())));
      } else if (_binding.empty())
      {
        const std::size_t index = IndexOf(text);
        if (index < _options.size())
        {
          _chosen = index;
          _has_chosen = true;
        } else
        {
          reader.Report(*value, std::format(
                          "'value' of {} is '{}', which is none of its options", reader.GetWhere(), text));
        }
      }
    }

    if (const auto *value = reader.ReadValue("enabled"); value != nullptr && !UiFlag::Read(*value, _enabled))
    {
      std::string text;
      reader.Report(*value, std::format(
                      "'enabled' of {} is {}, where true, false, or a value such as \"{{can_choose}}\" was expected",
                      reader.GetWhere(),
                      value->GetText(text) ? "'" + text + "'" : DataValue::Describe(value->GetKind())));
    }
  }

  bool UiSelect::IsFocusable() const
  {
    return true;
  }

  bool UiSelect::IsClickable() const
  {
    return true;
  }

  bool UiSelect::WantsFocus() const
  {
    return _autofocus;
  }

  bool UiSelect::IsEnabled() const
  {
    return _is_enabled;
  }

  bool UiSelect::UsesDirection(const Key direction) const
  {
    // up and down are its own: they move through the choices. Left and
    // right move the focus, unless the list is open, which they then do
    // nothing in
    return direction == Key::Up || direction == Key::Down || _is_open;
  }

  bool UiSelect::HasContent() const
  {
    return true;
  }

  bool UiSelect::TellsWhenItChanged() const
  {
    return true;
  }

  LayoutSize UiSelect::Measure(const UiFrame &frame, const float available_width, const float available_height)
  {
    (void) available_width;
    (void) available_height;

    const UiStyle &style = GetStyle();
    const UiFont *font = FontOf(style, frame);

    TextOptions options;
    options.line_height = style.LineHeight() * frame.scale;

    const float height = font != nullptr ? LineHeightOf(font->atlas, options) / frame.scale : style.font_size * 1.25f;
    _line_height = height;

    // as wide as the widest option, with the arrow behind it
    float width = 0.0f;
    if (font != nullptr)
    {
      for (const auto &option : _options)
      {
        width = std::max(width, PlaceText(font->atlas, DecodeUtf8(option.text), options).width / frame.scale);
      }
      if (!_placeholder.empty())
      {
        width = std::max(width, PlaceText(font->atlas, DecodeUtf8(_placeholder), options).width / frame.scale);
      }
    }

    return {width + padding_x + arrow_size, height};
  }

  void UiSelect::Update(const UiFrame &frame)
  {
    _is_enabled = _enabled.Get(*frame.values, true);

    if (_binding.empty()) { return; }
    if (_has_followed && _followed_revision == frame.values->GetRevision()) { return; }

    _has_followed = true;
    _followed_revision = frame.values->GetRevision();

    if (const UiValue *value = frame.values->Find(_binding); value != nullptr)
    {
      const std::size_t index = IndexOf(value->AsText());
      const bool has_chosen = index < _options.size();

      if (has_chosen != _has_chosen || (has_chosen && index != _chosen))
      {
        _has_chosen = has_chosen;
        _chosen = has_chosen ? index : 0;
        Invalidate(UiDirty::Paint);
      }
    }
  }

  void UiSelect::Interact(UiInteraction &interaction, const UiFrame &frame)
  {
    (void) frame;

    switch (interaction.kind)
    {
      case UiInteraction::Kind::PointerDown:
        if (_is_open)
        {
          // a press on a choice takes it, and one anywhere else closes
          const std::size_t index = OptionAt(interaction.y);
          if (index < _options.size() && ListBox().Contains(interaction.x, interaction.y)) { Choose(index); }
          Close();
        } else
        {
          Open();
        }
        interaction.is_used = true;
        break;

      case UiInteraction::Kind::Accept:
        // the press opened or chose already, unless accept came from the
        // keys or a controller
        if (!interaction.by_pointer)
        {
          if (_is_open)
          {
            Choose(_highlighted);
            Close();
          } else
          {
            Open();
          }
        }
        interaction.is_used = true;
        break;

      case UiInteraction::Kind::KeyDown:
        if (_is_open && interaction.key == Key::Home)
        {
          _highlighted = 0;
          ShowHighlighted();
          Invalidate(UiDirty::Paint);
          interaction.is_used = true;
        } else if (_is_open && interaction.key == Key::End)
        {
          _highlighted = _options.empty() ? 0 : _options.size() - 1;
          ShowHighlighted();
          Invalidate(UiDirty::Paint);
          interaction.is_used = true;
        }
        break;

      case UiInteraction::Kind::Direction:
      {
        if (interaction.key != Key::Up && interaction.key != Key::Down)
        {
          interaction.is_used = _is_open;
          break;
        }

        if (_options.empty()) { break; }

        if (_is_open)
        {
          // through the list, without going around
          if (interaction.key == Key::Down && _highlighted + 1 < _options.size()) { _highlighted++; }
          if (interaction.key == Key::Up && _highlighted > 0) { _highlighted--; }
          ShowHighlighted();
          Invalidate(UiDirty::Paint);
        } else
        {
          // the next choice, right away
          const std::size_t now = _has_chosen ? _chosen : 0;
          if (interaction.key == Key::Down && now + 1 < _options.size()) { Choose(now + 1); }
          if (interaction.key == Key::Up && now > 0) { Choose(now - 1); }
          if (!_has_chosen) { Choose(0); }
        }
        interaction.is_used = true;
        break;
      }

      case UiInteraction::Kind::Wheel:
        if (!_is_open || interaction.wheel_y == 0.0f) { break; }

        // the list is scrolled by rows, a notch at a time
        if (interaction.wheel_y > 0.0f && _first_shown + shown_at_most < _options.size()) { _first_shown++; }
        if (interaction.wheel_y < 0.0f && _first_shown > 0) { _first_shown--; }
        Invalidate(UiDirty::Paint);
        interaction.is_used = true;
        break;

      case UiInteraction::Kind::Cancel:
        if (!_is_open) { break; }
        Close();
        interaction.is_used = true;
        break;

      case UiInteraction::Kind::Blur:
        Close();
        break;

      default:
        break;
    }
  }

  void UiSelect::PaintContent(
    UiPainter &painter,
    const UiFrame &frame,
    const UiRectangle &content_box,
    const float opacity)
  {
    const UiStyle &style = GetStyle();
    const float arrow = std::round(arrow_size * frame.scale);
    const UiRectangle text_box{content_box.left, content_box.top, content_box.right - arrow - std::round(padding_x * frame.scale), content_box.bottom};

    if (_has_chosen)
    {
      DrawLine(painter, frame, style, _options[_chosen].text, text_box, Faded(style.color, opacity));
    } else if (!_placeholder.empty())
    {
      DrawLine(painter, frame, style, _placeholder, text_box, Faded(GetPartStyle("placeholder").color, opacity));
    }

    DrawArrow(
      painter, frame.scale, content_box.right, std::round(content_box.top + content_box.Height() / 2.0f),
      Faded(GetPartStyle("arrow").background_color, opacity));
  }

  bool UiSelect::HasTopLayer() const
  {
    return _is_open;
  }

  bool UiSelect::TopLayerContains(const UiFrame &frame, const float x, const float y) const
  {
    if (!_is_open) { return false; }
    return ToPixels(ListBox(), frame.scale).Contains(x, y);
  }

  void UiSelect::PaintTopLayer(UiPainter &painter, const UiFrame &frame)
  {
    if (!_is_open) { return; }

    const UiStyle &style = GetStyle();
    const UiStyle &of_list = GetPartStyle("list");
    const UiStyle &of_option = GetPartStyle("option");
    const UiStyle &of_highlight = GetPartStyle("highlight");

    const UiRectangle list = ToPixels(ListBox(), frame.scale);
    const float border = std::round(of_list.layout.border.left * frame.scale);
    const float row_height = std::round(RowHeight() * frame.scale);
    const float padding = std::round(padding_x * frame.scale);

    painter.FillRectangle(list, of_list.background_color);
    if (border > 0.0f) { painter.FillBorder(list, {border, border, border, border}, of_list.BorderColor()); }

    painter.PushClip({list.left + border, list.top + border, list.right - border, list.bottom - border});

    const std::size_t last = std::min(_options.size(), _first_shown + shown_at_most);
    for (std::size_t i = _first_shown; i < last; i++)
    {
      const float top = list.top + row_height * static_cast<float>(i - _first_shown);
      const UiRectangle row{list.left, top, list.right, top + row_height};
      const bool is_highlighted = i == _highlighted;
      const UiStyle &of_row = is_highlighted ? of_highlight : of_option;

      if (of_row.background_color.a > 0.0f) { painter.FillRectangle(row, of_row.background_color); }

      DrawLine(
        painter, frame, style, _options[i].text,
        {row.left + padding, row.top, row.right - padding, row.bottom}, of_row.color);
    }

    painter.PopClip();
  }

  bool UiSelect::GetField(const std::string &name, FieldValue &value) const
  {
    if (name == "value")
    {
      value = GetValue();
      return true;
    }

    if (name == "text")
    {
      value = GetText();
      return true;
    }

    if (name == "options")
    {
      std::vector<std::string> values;
      for (const auto &option : _options) { values.push_back(option.value); }
      value = values;
      return true;
    }

    if (name == "placeholder")
    {
      value = _placeholder;
      return true;
    }

    if (name == "open")
    {
      value = _is_open;
      return true;
    }

    if (name == "enabled")
    {
      value = _is_enabled;
      return true;
    }

    if (name == "autofocus")
    {
      value = _autofocus;
      return true;
    }

    return false;
  }

  bool UiSelect::SetField(const std::string &name, const FieldValue &value, std::string &error)
  {
    const std::string what = "'" + name + "' of " + Describe();

    if (name == "value")
    {
      std::string text;
      if (!TakeText(value, what, text, error)) { return false; }

      const std::size_t index = IndexOf(text);
      if (index >= _options.size())
      {
        error = what + " is '" + text + "', which is none of its options";
        return false;
      }

      Choose(index);
      return true;
    }

    if (name == "options")
    {
      const auto *values = std::get_if<std::vector<std::string>>(&value);
      if (values == nullptr)
      {
        error = what + " has to be a list of text";
        return false;
      }

      const std::string chosen = GetValue();

      // an option that was there keeps the text the file gave it, a new one
      // is shown as its value
      std::vector<Option> options;
      for (const auto &each : *values)
      {
        const std::size_t before = IndexOf(each);
        options.push_back(before < _options.size() ? _options[before] : Option{each, each});
      }
      _options = std::move(options);

      // what was chosen stays chosen while it is still there
      const std::size_t index = IndexOf(chosen);
      _has_chosen = index < _options.size();
      _chosen = _has_chosen ? index : 0;
      _first_shown = 0;
      _highlighted = 0;
      Close();

      Invalidate(UiDirty::Layout | UiDirty::Paint);
      return true;
    }

    if (name == "placeholder")
    {
      if (!TakeText(value, what, _placeholder, error)) { return false; }
      Invalidate(UiDirty::Layout | UiDirty::Paint);
      return true;
    }

    if (name == "open")
    {
      bool flag = false;
      if (!TakeFlag(value, what, flag, error)) { return false; }

      if (flag) { Open(); }
      else { Close(); }
      return true;
    }

    if (name == "text")
    {
      error = what + " follows from what is chosen, and cannot be set";
      return false;
    }

    if (name == "enabled")
    {
      bool flag = false;
      if (!TakeFlag(value, what, flag, error)) { return false; }

      _enabled = UiFlag(flag);
      _is_enabled = flag;
      return true;
    }

    if (name == "autofocus") { return TakeFlag(value, what, _autofocus, error); }

    return UiElement::SetField(name, value, error);
  }

  std::vector<UiElement::Field> UiSelect::GetFields() const
  {
    return {
      {"value", FieldKind::String, "What is chosen, which may follow a value of the game as {name}", {}},
      {"text", FieldKind::String, "What is shown for what is chosen", {}},
      {"options", FieldKind::StringList, "What can be chosen, by value", {}},
      {"placeholder", FieldKind::String, "What is shown while nothing is chosen", {}},
      {"open", FieldKind::Boolean, "Whether the list is open", {}},
      {"enabled", FieldKind::Boolean, "Whether it can be chosen from", {}},
      {"autofocus", FieldKind::Boolean, "Whether it has the focus when its file is shown", {}}
    };
  }

  std::string UiSelect::GetValue() const
  {
    return _has_chosen ? _options[_chosen].value : "";
  }

  std::string UiSelect::GetText() const
  {
    return _has_chosen ? _options[_chosen].text : _placeholder;
  }

  const std::vector<UiSelect::Option> &UiSelect::GetOptions() const
  {
    return _options;
  }

  bool UiSelect::IsOpen() const
  {
    return _is_open;
  }

  std::size_t UiSelect::GetHighlighted() const
  {
    return _highlighted;
  }
} // neon
