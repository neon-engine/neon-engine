#ifndef TOOL_UI_DOCK_PLACEMENT_HPP
#define TOOL_UI_DOCK_PLACEMENT_HPP

#include <string>

#include "tool-ui-dock-side.hpp"

namespace neon
{
  /// Where a window goes the first time it is shown: a share of one side
  /// of the dock space, split off from what fills the middle. See
  /// ToolUiDockLayout.
  struct ToolUiDockPlacement
  {
    /// The title of the window, as BeginWindow() is given it.
    std::string window;

    ToolUiDockSide side = ToolUiDockSide::Bottom;

    /// The part of the width, for the left and the right, or of the
    /// height, for the top and the bottom, the window takes, above 0 and
    /// below 1.
    float share = 0.25f;
  };
} // neon

#endif //TOOL_UI_DOCK_PLACEMENT_HPP
