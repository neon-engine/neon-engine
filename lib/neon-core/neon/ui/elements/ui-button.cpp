#include "ui-button.hpp"

#include <neon/ui/ui-fields.hpp>

#include <format>

namespace neon
{
  bool UiButton::TellsWhenItChanged() const
  {
    return true;
  }

  bool UiButton::GetField(const std::string &name, FieldValue &value) const
  {
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

    if (name == "action")
    {
      value = std::string(!_scene.empty() ? "scene" : _closes ? "close" : "none");
      return true;
    }

    if (name == "scene")
    {
      value = _scene;
      return true;
    }

    if (name == "on_click")
    {
      value = _on_click.AsText();
      return true;
    }

    return false;
  }

  bool UiButton::SetField(const std::string &name, const FieldValue &value, std::string &error)
  {
    const std::string what = "'" + name + "' of " + Describe();

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

      // what is set holds, and no longer follows a value of the game
      _enabled = UiFlag(flag);
      _is_enabled = flag;
      return true;
    }

    if (name == "autofocus")
    {
      return TakeFlag(value, what, _autofocus, error);
    }

    if (name == "action")
    {
      std::string action;
      if (!TakeText(value, what, action, error)) { return false; }

      if (action != "none" && action != "close" && action != "scene")
      {
        error = std::format("{} is '{}', where none, close, or scene was expected", what, action);
        return false;
      }

      // a scene asks for a change and closes the file; the path comes with
      // the scene field, which may arrive before or after
      _closes = action == "close" || action == "scene";
      if (action != "scene") { _scene.clear(); }
      return true;
    }

    if (name == "scene")
    {
      return TakeText(value, what, _scene, error);
    }

    if (name == "on_click")
    {
      std::string text;
      if (!TakeText(value, what, text, error)) { return false; }

      // an empty text takes the call away
      if (text.empty())
      {
        _on_click = {};
        return true;
      }

      UiCall call;
      if (std::string problem; !UiCall::Parse(text, call, problem))
      {
        error = std::format("{} is '{}', which is no call of a function: {}", what, text, problem);
        return false;
      }

      _on_click = call;
      return true;
    }

    return UiElement::SetField(name, value, error);
  }

  std::vector<UiElement::Field> UiButton::GetFields() const
  {
    return {
      {"text", FieldKind::String, "What is written on it, which may refer to values of the game as {name}", {}},
      {"enabled", FieldKind::Boolean, "Whether it can be chosen", {}},
      {"autofocus", FieldKind::Boolean, "Whether it has the focus when its file is shown", {}},
      {"action", FieldKind::Choice, "What choosing it does besides reporting a click: close closes its file, scene asks the game for the scene it names", {"none", "close", "scene"}},
      {"scene", FieldKind::String, "Virtual path of the scene that action: scene asks for", {}},
      {"on_click", FieldKind::String, "The function of the game that choosing it calls, with what it is handed: unlock, or open('safe', door, $event)", {}}
    };
  }

  void UiButton::ApplyDefaults(UiStyle &style) const
  {
    style.layout.padding = {
      LayoutLength::Pixels(8.0f),
      LayoutLength::Pixels(16.0f),
      LayoutLength::Pixels(8.0f),
      LayoutLength::Pixels(16.0f)
    };

    // what is inside a button is in its middle
    style.layout.justify_content = JustifyContent::Center;
    style.layout.align_items = AlignItems::Center;
    style.text_align = TextAlign::Center;

    style.background_color = {0.231f, 0.259f, 0.322f, 1.0f}; // #3b4252
    style.color = {1.0f, 1.0f, 1.0f, 1.0f};
    style.pointer_events = UiPointerEvents::Auto;
  }

  void UiButton::ApplyStateDefaults(UiStyle &style, const UiStates &states) const
  {
    if (states.disabled)
    {
      style.opacity = 0.5f;
      return;
    }

    if (states.focus)
    {
      style.outline_width = 2.0f;
      style.outline_offset = 2.0f;
      style.outline_color = Color{1.0f, 0.820f, 0.400f, 1.0f}; // #ffd166
    }

    if (states.hover) { style.background_color = {0.298f, 0.337f, 0.416f, 1.0f}; } // #4c566a
    if (states.active) { style.background_color = {0.180f, 0.204f, 0.251f, 1.0f}; } // #2e3440
  }

  void UiButton::ReadAttributes(const DataReader &reader)
  {
    _text.Read(reader);
    reader.Read("autofocus", _autofocus);

    std::size_t action = 0;
    const bool has_action = reader.ReadChoice("action", {"none", "close", "scene"}, action);
    if (has_action) { _closes = action != 0; }

    reader.Read("scene", _scene);
    if (has_action && action == 2 && _scene.empty())
    {
      reader.Report(std::format("'action' of {} is scene, which needs a 'scene' with the path of one", reader.GetWhere()));
    }
    if (!(has_action && action == 2)) { _scene.clear(); }

    if (const auto *value = reader.ReadValue("on_click"); value != nullptr)
    {
      std::string on_click;
      std::string problem;
      if (!value->GetText(on_click))
      {
        reader.Report(*value, std::format(
                        "'on_click' of {} is {}, where the call of a function was expected, such as unlock or open('safe')",
                        reader.GetWhere(),
                        DataValue::Describe(value->GetKind())));
      } else if (!UiCall::Parse(on_click, _on_click, problem))
      {
        reader.Report(*value, std::format(
                        "'on_click' of {} is '{}', which is no call of a function: {}",
                        reader.GetWhere(),
                        on_click,
                        problem));
      }
    }

    if (const auto *value = reader.ReadValue("enabled"); value != nullptr && !UiFlag::Read(*value, _enabled))
    {
      std::string text;
      reader.Report(*value, std::format(
                      "'enabled' of {} is {}, where true, false, or a value such as \"{{can_start}}\" was expected",
                      reader.GetWhere(),
                      value->GetText(text) ? "'" + text + "'" : DataValue::Describe(value->GetKind())));
    }
  }

  bool UiButton::ClosesItsFile() const
  {
    return _closes;
  }

  const std::string &UiButton::AsksForScene() const
  {
    return _scene;
  }

  const UiCall *UiButton::CallsWhenClicked() const
  {
    return _on_click.IsEmpty() ? nullptr : &_on_click;
  }

  bool UiButton::TakesChildren() const
  {
    return true;
  }

  bool UiButton::IsFocusable() const
  {
    return true;
  }

  bool UiButton::IsClickable() const
  {
    return true;
  }

  bool UiButton::WantsFocus() const
  {
    return _autofocus;
  }

  bool UiButton::IsEnabled() const
  {
    return _is_enabled;
  }

  bool UiButton::HasContent() const
  {
    return _text.IsWritten();
  }

  LayoutSize UiButton::Measure(const UiFrame &frame, const float available_width, float available_height)
  {
    return _text.Measure(GetStyle(), frame, available_width);
  }

  void UiButton::Update(const UiFrame &frame)
  {
    const std::string before = _text.Get();
    _text.Update(frame);

    if (_text.Get() != before) { Invalidate(UiDirty::Layout | UiDirty::Paint); }

    _is_enabled = _enabled.Get(*frame.values, true);
  }

  void UiButton::PaintContent(
    UiPainter &painter,
    const UiFrame &frame,
    const UiRectangle &content_box,
    const float opacity)
  {
    if (_text.IsWritten()) { _text.Paint(painter, GetStyle(), frame, content_box, opacity, true); }
  }

  const std::string &UiButton::GetText() const
  {
    return _text.Get();
  }
} // neon
