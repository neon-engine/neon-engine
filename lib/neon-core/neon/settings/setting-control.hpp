#ifndef SETTING_CONTROL_HPP
#define SETTING_CONTROL_HPP

#include <string>

#include "setting-kind.hpp"

namespace neon
{
  /// The control a settings menu shows a setting of the game with, see
  /// SettingDeclaration. Written in the project's settings file as
  /// `control: slider`; left out, the natural one for the kind is taken,
  /// see natural_control_of().
  enum class SettingControl
  {
    /// A number between its least and its most, moved along a line.
    Slider = 0,

    /// A flag, switched.
    Toggle,

    /// A flag, ticked.
    Checkbox,

    /// A choice, from a list that opens.
    Dropdown,

    /// A text or a number, typed.
    Field,

    /// An action, pressed.
    Button
  };

  /// The control as it is written.
  inline const char *setting_control_name(const SettingControl control)
  {
    switch (control)
    {
      case SettingControl::Slider: return "slider";
      case SettingControl::Toggle: return "toggle";
      case SettingControl::Checkbox: return "checkbox";
      case SettingControl::Dropdown: return "dropdown";
      case SettingControl::Field: return "field";
      case SettingControl::Button: return "button";
    }
    return "field";
  }

  /// Reads a control as it is written. Returns false for a text that is
  /// none.
  inline bool setting_control_of(const std::string &text, SettingControl &control)
  {
    for (const SettingControl each : {
           SettingControl::Slider, SettingControl::Toggle, SettingControl::Checkbox, SettingControl::Dropdown,
           SettingControl::Field, SettingControl::Button
         })
    {
      if (text == setting_control_name(each))
      {
        control = each;
        return true;
      }
    }
    return false;
  }

  /// The control a kind is shown with when the declaration names none: a
  /// flag as a toggle, a number as a slider when it has a least and a most
  /// and as a field otherwise, a choice as a dropdown, a text as a field,
  /// and an action as a button.
  inline SettingControl natural_control_of(const SettingKind kind, const bool has_range)
  {
    switch (kind)
    {
      case SettingKind::Flag: return SettingControl::Toggle;
      case SettingKind::Number: return has_range ? SettingControl::Slider : SettingControl::Field;
      case SettingKind::Choice: return SettingControl::Dropdown;
      case SettingKind::Text: return SettingControl::Field;
      case SettingKind::Action: return SettingControl::Button;
    }
    return SettingControl::Field;
  }

  /// Whether a control shows a kind: a slider a number, a toggle and a
  /// checkbox a flag, a dropdown a choice, a field a text or a number, a
  /// button an action.
  inline bool control_fits_kind(const SettingControl control, const SettingKind kind)
  {
    switch (control)
    {
      case SettingControl::Slider: return kind == SettingKind::Number;
      case SettingControl::Toggle:
      case SettingControl::Checkbox: return kind == SettingKind::Flag;
      case SettingControl::Dropdown: return kind == SettingKind::Choice;
      case SettingControl::Field: return kind == SettingKind::Text || kind == SettingKind::Number;
      case SettingControl::Button: return kind == SettingKind::Action;
    }
    return false;
  }
} // neon

#endif //SETTING_CONTROL_HPP
