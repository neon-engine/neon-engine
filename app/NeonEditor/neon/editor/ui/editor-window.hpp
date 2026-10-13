#ifndef EDITOR_WINDOW_HPP
#define EDITOR_WINDOW_HPP

#include <functional>
#include <string>

#include <neon/tool-ui/tool-ui-context.hpp>
#include <neon/tool-ui/tool-ui-dock-side.hpp>

namespace neon
{
  /// A tool window of the editor as EditorWindows knows it: what the View
  /// menu lists it as, the shortcut that shows and hides it, where it
  /// docks the first time it is shown, and how it is drawn. Hidden until
  /// it is shown, as every tool window is (#590).
  struct EditorWindow
  {
    /// The title, which names the window in the View menu, in a layout,
    /// and in the editor's settings. No two windows share one.
    std::string title;

    /// The shortcut that shows and hides it, as ToolUiContext::MenuItem()
    /// shows it and ToolUiContext::IsShortcutPressed() asks for it, such
    /// as `Ctrl+L`. Empty for none.
    std::string shortcut;

    /// Where it docks the first time it is ever shown, see
    /// ToolUiDockPlacement.
    ToolUiDockSide side = ToolUiDockSide::Bottom;
    float share = 0.25f;

    /// Declares the window, from BeginWindow() to EndWindow(), in a frame
    /// it is shown.
    std::function<void(ToolUiContext &)> draw;

    bool shown = false;
  };
} // neon

#endif //EDITOR_WINDOW_HPP
