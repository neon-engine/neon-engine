#ifndef TOOL_UI_DOCK_LAYOUT_HPP
#define TOOL_UI_DOCK_LAYOUT_HPP

#include <string>

namespace neon
{
  /// How the windows of the tool interface are docked when nothing has
  /// been dragged yet: a window fills the middle of the dock space, and
  /// another takes a share of the bottom. A window that is named nowhere
  /// floats. Each window is named by its title, as BeginWindow() is given
  /// it. The layout holds until someone drags a window elsewhere.
  struct ToolUiDockLayout
  {
    std::string center;
    std::string bottom;

    /// The part of the height the window at the bottom takes, above 0 and
    /// below 1.
    float bottom_share = 0.25f;
  };
} // neon

#endif //TOOL_UI_DOCK_LAYOUT_HPP
