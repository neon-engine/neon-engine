#ifndef EDITOR_WINDOWS_HPP
#define EDITOR_WINDOWS_HPP

#include <string>
#include <string_view>
#include <vector>

#include <neon/tool-ui/tool-ui-context.hpp>
#include <neon/tool-ui/tool-ui-dock-layout.hpp>

#include "editor-window.hpp"

namespace neon
{
  /// The tool windows of the editor: the log, and whatever else is
  /// registered, each hidden until it is shown from the View menu or with
  /// its shortcut, and hidden again the same way, so that the editor opens
  /// with the viewport alone (#590). The View menu is drawn from what is
  /// registered, in the order of registration, and so are the windows that
  /// are shown.
  ///
  /// The viewport is not a tool window: it is always shown, it is what the
  /// others dock around, and it is not in the View menu.
  class EditorWindows final
  {
    std::vector<EditorWindow> _windows;

    [[nodiscard]] EditorWindow *Find(const std::string &title);

    [[nodiscard]] const EditorWindow *Find(const std::string &title) const;

  public:
    /// The label of the menu the windows are listed in.
    static constexpr std::string_view menu = "View";

    /// Registers a window, hidden. Returns false, and registers nothing,
    /// for a title that is empty or taken.
    bool Add(const EditorWindow &window);

    [[nodiscard]] bool Has(const std::string &title) const;

    [[nodiscard]] bool IsShown(const std::string &title) const;

    /// Shows or hides a window. Returns false for a title that is not
    /// registered.
    bool Show(const std::string &title, bool shown);

    /// Shows a hidden window, and hides a shown one. Returns false for a
    /// title that is not registered.
    bool Toggle(const std::string &title);

    [[nodiscard]] const std::vector<EditorWindow> &GetWindows() const { return _windows; }

    /// The titles of the windows that are shown, in the order they were
    /// registered: what the editor keeps for next time.
    [[nodiscard]] std::vector<std::string> GetShownTitles() const;

    /// Declares the View menu of the menu bar, which is begun: one item a
    /// window, marked while it is shown, which shows or hides it.
    void DrawViewMenu(ToolUiContext &ui);

    /// Shows or hides the window whose shortcut was pressed in this
    /// frame. Called once a frame, before the windows are drawn.
    void HandleShortcuts(ToolUiContext &ui);

    /// Declares the windows that are shown.
    void DrawShown(ToolUiContext &ui);

    /// Where the windows dock the first time each is shown, around the
    /// window that fills the middle.
    [[nodiscard]] ToolUiDockLayout LayoutAround(const std::string &center) const;
  };
} // neon

#endif //EDITOR_WINDOWS_HPP
