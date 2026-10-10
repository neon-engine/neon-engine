#ifndef SETTING_KIND_HPP
#define SETTING_KIND_HPP

#include <string>

namespace neon
{
  /// What a setting of a game holds, see SettingDeclaration. The kind is
  /// written in the project's settings file, as `kind: flag`.
  enum class SettingKind
  {
    /// On or off.
    Flag = 0,

    /// A number, whole or not, within `least` and `most` when they are
    /// declared.
    Number,

    /// Any text.
    Text,

    /// One of the texts the declaration lists as `choices`.
    Choice,

    /// Nothing: a button of the menu, such as "Reset progress", which is
    /// triggered rather than set, and whose subscribers are told so with
    /// no value. It has no default and nothing is kept of it.
    Action
  };

  /// The kind as it is written, `flag`, `number`, `text`, or `choice`.
  inline const char *setting_kind_name(const SettingKind kind)
  {
    switch (kind)
    {
      case SettingKind::Flag: return "flag";
      case SettingKind::Number: return "number";
      case SettingKind::Text: return "text";
      case SettingKind::Choice: return "choice";
      case SettingKind::Action: return "action";
    }
    return "flag";
  }

  /// Reads a kind as it is written. Returns false for a text that is none.
  inline bool setting_kind_of(const std::string &text, SettingKind &kind)
  {
    for (const SettingKind each : {
           SettingKind::Flag, SettingKind::Number, SettingKind::Text, SettingKind::Choice, SettingKind::Action
         })
    {
      if (text == setting_kind_name(each))
      {
        kind = each;
        return true;
      }
    }
    return false;
  }
} // neon

#endif //SETTING_KIND_HPP
