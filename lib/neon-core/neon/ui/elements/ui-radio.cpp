#include "ui-radio.hpp"

#include <algorithm>
#include <cmath>
#include <format>

#include <neon/ui/ui-fields.hpp>

#include "ui-binding.hpp"

namespace neon
{
  // Helpers of UiRadio, for this file alone.
  namespace
  {
    constexpr float box_size = 18.0f;
    constexpr float gap = 8.0f;

    Color Faded(const Color &color, const float opacity)
    {
      return {color.r, color.g, color.b, color.a * opacity};
    }

    void CollectRadios(UiElement &element, std::vector<UiRadio *> &radios)
    {
      if (auto *radio = dynamic_cast<UiRadio *>(&element); radio != nullptr) { radios.push_back(radio); }
      for (const auto &child : element.GetChildren()) { CollectRadios(*child, radios); }
    }
  }

  void UiRadio::LetGoOfTheOthers()
  {
    UiElement *root = this;
    while (root->GetParent() != nullptr) { root = root->GetParent(); }

    std::vector<UiRadio *> radios;
    CollectRadios(*root, radios);

    for (UiRadio *other : radios)
    {
      if (other != this && other->_group == _group) { other->LetGo(); }
    }
  }

  void UiRadio::ApplyDefaults(UiStyle &style) const
  {
    style.pointer_events = UiPointerEvents::Auto;
    style.cursor = UiCursor::Pointer;
    style.layout.align_items = AlignItems::Center;
    style.layout.padding = {
      LayoutLength::Pixels(4.0f), LayoutLength::Pixels(4.0f), LayoutLength::Pixels(4.0f), LayoutLength::Pixels(4.0f)
    };
    style.color = {1.0f, 1.0f, 1.0f, 1.0f};
    style.text_align = TextAlign::Left;
  }

  void UiRadio::ApplyStateDefaults(UiStyle &style, const UiStates &states) const
  {
    if (states.disabled)
    {
      style.opacity = 0.5f;
      return;
    }

    if (states.focus)
    {
      style.outline_width = 2.0f;
      style.outline_offset = 0.0f;
      style.outline_color = Color{1.0f, 0.820f, 0.400f, 1.0f}; // #ffd166
    }
  }

  void UiRadio::ApplyPartDefaults(const std::string &part, UiStyle &style) const
  {
    UiElement::ApplyPartDefaults(part, style);

    if (part == "box")
    {
      style.background_color = {1.0f, 1.0f, 1.0f, 0.08f};
      style.border_color = Color{1.0f, 1.0f, 1.0f, 0.5f};
      style.layout.border = {2.0f, 2.0f, 2.0f, 2.0f};
    } else if (part == "mark")
    {
      style.background_color = GetStyle().accent_color;
    }
  }

  void UiRadio::ReadAttributes(const DataReader &reader)
  {
    _text.Read(reader);
    reader.Read("autofocus", _autofocus);
    reader.Read("group", _group);

    if (const auto *value = reader.ReadValue("value"); value != nullptr)
    {
      std::string binding;
      if (!UiBinding::Read(*value, _value, binding) || !binding.empty())
      {
        reader.Report(*value, std::format(
                        "'value' of {} is {}, where the text it stands for was expected",
                        reader.GetWhere(), DataValue::Describe(value->GetKind())));
      }
    }

    if (_group.empty())
    {
      reader.Report(std::format("{} has no 'group', which says which radios belong together", reader.GetWhere()));
    }

    if (const auto *value = reader.ReadValue("checked"); value != nullptr)
    {
      std::string text;
      const bool is_read = UiBinding::Read(*value, text, _binding) && (text == "true" || text == "false" || !_binding.empty());

      if (is_read)
      {
        _checked = text == "true";
      } else
      {
        reader.Report(*value, std::format(
                        "'checked' of {} is {}, where true, false, or a value such as \"{{quality}}\" was expected",
                        reader.GetWhere(),
                        value->GetText(text) ? "'" + text + "'" : DataValue::Describe(value->GetKind())));
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

  bool UiRadio::IsFocusable() const
  {
    return true;
  }

  bool UiRadio::IsClickable() const
  {
    return true;
  }

  bool UiRadio::WantsFocus() const
  {
    return _autofocus;
  }

  bool UiRadio::IsEnabled() const
  {
    return _is_enabled;
  }

  bool UiRadio::IsChecked() const
  {
    return _checked;
  }

  bool UiRadio::HasContent() const
  {
    return true;
  }

  bool UiRadio::TellsWhenItChanged() const
  {
    return true;
  }

  LayoutSize UiRadio::Measure(const UiFrame &frame, const float available_width, const float available_height)
  {
    (void) available_height;

    LayoutSize size{box_size, box_size};
    if (!_text.IsWritten()) { return size; }

    const float room = std::isnan(available_width) ? available_width : std::max(0.0f, available_width - size.width - gap);
    const LayoutSize text = _text.Measure(GetStyle(), frame, room);

    size.width += gap + text.width;
    size.height = std::max(size.height, text.height);
    return size;
  }

  void UiRadio::Update(const UiFrame &frame)
  {
    const std::string before = _text.Get();
    _text.Update(frame);
    if (_text.Get() != before) { Invalidate(UiDirty::Layout | UiDirty::Paint); }

    _is_enabled = _enabled.Get(*frame.values, true);

    if (_binding.empty()) { return; }
    if (_has_followed && _followed_revision == frame.values->GetRevision()) { return; }

    _has_followed = true;
    _followed_revision = frame.values->GetRevision();

    // chosen while the value of the game is what it stands for
    if (const UiValue *value = frame.values->Find(_binding); value != nullptr)
    {
      const bool checked = value->AsText() == _value;
      if (checked != _checked)
      {
        _checked = checked;
        Invalidate(UiDirty::Style | UiDirty::Paint);
      }
    }
  }

  void UiRadio::Interact(UiInteraction &interaction, const UiFrame &frame)
  {
    (void) frame;

    if (interaction.kind == UiInteraction::Kind::Accept)
    {
      Choose();
      interaction.is_used = true;
    }
  }

  void UiRadio::PaintContent(
    UiPainter &painter,
    const UiFrame &frame,
    const UiRectangle &content_box,
    const float opacity)
  {
    const float size = std::round(box_size * frame.scale);
    const float top = std::round(content_box.top + (content_box.Height() - size) / 2.0f);
    const UiRectangle box{content_box.left, top, content_box.left + size, top + size};

    const UiStyle &of_box = GetPartStyle("box");
    const float border = std::max(1.0f, std::round(of_box.layout.border.left * frame.scale));

    painter.FillRectangle(box, Faded(of_box.background_color, opacity));
    painter.FillBorder(box, {border, border, border, border}, Faded(of_box.BorderColor(), opacity));

    if (_checked)
    {
      const float inset = std::round(size * 0.3f);
      painter.FillRectangle(
        {box.left + inset, box.top + inset, box.right - inset, box.bottom - inset},
        Faded(GetPartStyle("mark").background_color, opacity));
    }

    if (!_text.IsWritten()) { return; }

    const float text_left = content_box.left + size + std::round(gap * frame.scale);
    _text.Paint(
      painter, GetStyle(), frame, {text_left, content_box.top, content_box.right, content_box.bottom}, opacity, true);
  }

  bool UiRadio::GetField(const std::string &name, FieldValue &value) const
  {
    if (name == "checked")
    {
      value = _checked;
      return true;
    }

    if (name == "value")
    {
      value = _value;
      return true;
    }

    if (name == "group")
    {
      value = _group;
      return true;
    }

    if (name == "text")
    {
      if (!_text.IsWritten()) { return false; }
      value = _text.Get();
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

  bool UiRadio::SetField(const std::string &name, const FieldValue &value, std::string &error)
  {
    const std::string what = "'" + name + "' of " + Describe();

    if (name == "checked")
    {
      bool flag = false;
      if (!TakeFlag(value, what, flag, error)) { return false; }

      if (flag) { Choose(); }
      else { LetGo(); }
      return true;
    }

    if (name == "value") { return TakeText(value, what, _value, error); }

    if (name == "group")
    {
      error = what + " is set in the file, and cannot be changed";
      return false;
    }

    if (name == "text")
    {
      std::string text;
      if (!TakeText(value, what, text, error)) { return false; }

      if (std::string problem; !_text.Set(text, problem))
      {
        error = what + " cannot be read: " + problem;
        return false;
      }

      Invalidate(UiDirty::Layout | UiDirty::Paint);
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

    if (name == "autofocus") { return TakeFlag(value, what, _autofocus, error); }

    return UiElement::SetField(name, value, error);
  }

  std::vector<UiElement::Field> UiRadio::GetFields() const
  {
    return {
      {"checked", FieldKind::Boolean, "Whether it is the one chosen, which may follow a value of the game as {name}", {}},
      {"value", FieldKind::String, "What it stands for, which the value of the game becomes when it is chosen", {}},
      {"group", FieldKind::String, "Which radios belong together", {}},
      {"text", FieldKind::String, "What is written next to it, which may refer to values of the game as {name}", {}},
      {"enabled", FieldKind::Boolean, "Whether it can be chosen", {}},
      {"autofocus", FieldKind::Boolean, "Whether it has the focus when its file is shown", {}}
    };
  }

  void UiRadio::Choose()
  {
    if (_checked) { return; }

    _checked = true;
    Invalidate(UiDirty::Style | UiDirty::Paint);
    LetGoOfTheOthers();

    Notify("changed", "true", _binding, UiValue::Text(_value));
  }

  void UiRadio::LetGo()
  {
    if (!_checked) { return; }

    _checked = false;
    Invalidate(UiDirty::Style | UiDirty::Paint);
    Notify("changed", "false");
  }

  const std::string &UiRadio::GetGroup() const
  {
    return _group;
  }

  const std::string &UiRadio::GetValue() const
  {
    return _value;
  }

  const std::string &UiRadio::GetText() const
  {
    return _text.Get();
  }
} // neon
