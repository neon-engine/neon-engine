#ifndef TOOL_UI_DOCK_LAYOUT_HPP
#define TOOL_UI_DOCK_LAYOUT_HPP

#include <string>
#include <vector>

#include "tool-ui-dock-placement.hpp"

namespace neon
{
  /// How the windows of the tool interface are docked when nothing has
  /// been dragged yet: a window fills the middle of the dock space, and
  /// every other window takes a share of a side the first time it is
  /// shown, split off from the middle. A window that is named nowhere
  /// floats. Each window is named by its title, as BeginWindow() is given
  /// it.
  ///
  /// The layout decides where a window goes the first time it is ever
  /// shown; after that it goes where it was last, which is kept by the
  /// layout of the toolkit, see ToolUiSystem::SaveLayout(), and holds
  /// until someone drags it elsewhere.
  struct ToolUiDockLayout
  {
    std::string center;
    std::vector<ToolUiDockPlacement> sides;
  };
} // neon

#endif //TOOL_UI_DOCK_LAYOUT_HPP
