#ifndef SETTING_DECLARATION_HPP
#define SETTING_DECLARATION_HPP

#include <optional>
#include <string>
#include <vector>

#include <neon/data/data-value.hpp>

#include "setting-control.hpp"
#include "setting-kind.hpp"

namespace neon
{
  /// A setting a game declares for itself, under `settings` of its project
  /// file: its name, what it holds, its default, and what it may be set
  /// to. The settings files set it under `game`, and the store keeps one of
  /// these for every setting, see SettingsStore.
  ///
  ///     settings:
  ///       difficulty:
  ///         kind: choice
  ///         default: normal
  ///         choices: [easy, normal, hard]
  ///       subtitles:
  ///         kind: flag
  ///         default: true
  ///       field_of_view:
  ///         kind: number
  ///         default: 90
  ///         least: 60
  ///         most: 120
  ///         category: Controls
  ///         group: Camera
  ///         order: 2
  ///         control: slider
  ///         step: 5
  ///         label: Field of view
  ///
  /// Where and how the settings menu shows it is part of the declaration,
  /// so that the menu builds its rows without the game writing them, see
  /// GameMenu: the `category` is a section of the menu, the `group` a
  /// heading inside it, the `order` sorts categories, groups, and rows,
  /// and the `control` is what the player sees.
  struct SettingDeclaration
  {
    /// The key under `settings`, and under `game` of the settings files:
    /// letters, digits, and underscores.
    std::string name;

    SettingKind kind = SettingKind::Flag;

    /// What the setting holds unless a layer above says otherwise: a flag,
    /// a number, or a text, as the kind says.
    DataValue default_value;

    /// The least and the most a number may be, when the declaration says.
    std::optional<double> least;
    std::optional<double> most;

    /// The texts a choice may be, in the order the menu shows them.
    std::vector<std::string> choices;

    /// What the setting starts with: the default, unless a layer read on
    /// top of the project's file, the player's, wrote another. Empty takes
    /// the default.
    DataValue value;

    /// The section of the settings menu the setting is shown in, next to
    /// the engine's own, as it is written: `Gameplay`, `Controls`. Empty
    /// is shown in a section called Game.
    std::string category;

    /// A heading inside the category that lumps rows together, such as
    /// `Camera`. Empty puts the row under no heading, in front of the
    /// groups.
    std::string group;

    /// Sorts rows within a group, and a category or a group takes the
    /// order of the first setting that names it. Ties keep the order of
    /// declaration.
    double order = 0.0;

    /// The control the menu shows, when the declaration names one; the
    /// natural one for the kind otherwise, see natural_control_of().
    std::optional<SettingControl> control;

    /// What a slider moves by, when the declaration says.
    std::optional<double> step;

    /// The text the row shows. Empty is the name with its underscores as
    /// spaces and its first letter capitalized.
    std::string label;

    /// The control as it is shown: the one named, or the natural one.
    [[nodiscard]] SettingControl GetControl() const
    {
      return control.has_value() ? *control : natural_control_of(kind, least.has_value() && most.has_value());
    }

    /// The text the row shows, see `label`.
    [[nodiscard]] std::string GetLabel() const
    {
      if (!label.empty()) { return label; }

      std::string text = name;
      for (char &c : text) { if (c == '_') { c = ' '; } }
      if (!text.empty() && text.front() >= 'a' && text.front() <= 'z') { text.front() = static_cast<char>(text.front() - 'a' + 'A'); }
      return text;
    }
  };
} // neon

#endif //SETTING_DECLARATION_HPP
