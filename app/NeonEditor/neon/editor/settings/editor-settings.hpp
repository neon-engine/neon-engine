#ifndef EDITOR_SETTINGS_HPP
#define EDITOR_SETTINGS_HPP

#include <string>
#include <vector>

#include <neon/window/window-mode.hpp>

namespace neon
{
  /// The editor's own settings, which are nobody's project's: how its
  /// window was left, what it opened last, where it makes new projects,
  /// and which tool windows were shown. Kept in `editor-settings.yml` of
  /// the editor's folder, see EditorFolder and EditorSettingsFile (#582).
  struct EditorSettings
  {
    /// The size of the editor's window in points, as it was left, and
    /// how it is shown. The project's window settings are the game's.
    int window_width = 1600;
    int window_height = 1000;
    WindowMode window_mode = WindowMode::Windowed;

    /// The folder of the project that was open last, as it was given on
    /// the command line: a path of this machine, since the file is this
    /// machine's. Empty when none was.
    std::string last_project;

    /// The folder new projects are made in, a path of this machine, for
    /// the new-project dialog (#573). Empty until one is chosen.
    std::string projects_folder;

    /// The titles of the tool windows that were shown, see EditorWindows,
    /// so that they come back next time.
    std::vector<std::string> open_windows;
  };
} // neon

#endif //EDITOR_SETTINGS_HPP
