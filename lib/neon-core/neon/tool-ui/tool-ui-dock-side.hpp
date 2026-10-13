#ifndef TOOL_UI_DOCK_SIDE_HPP
#define TOOL_UI_DOCK_SIDE_HPP

namespace neon
{
  /// A side of the dock space a window is docked to when it is first
  /// shown, next to the window that fills the middle. See ToolUiDockLayout.
  enum class ToolUiDockSide
  {
    Left = 0,
    Right,
    Top,
    Bottom
  };
} // neon

#endif //TOOL_UI_DOCK_SIDE_HPP
