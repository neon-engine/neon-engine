#ifndef TOOL_UI_ITEM_HPP
#define TOOL_UI_ITEM_HPP

#include <string>

#include "tool-ui-item-kind.hpp"
#include "tool-ui-rect.hpp"

namespace neon
{
  /// Something the tool interface drew in a frame, under the label it was
  /// declared with and where it went. The backend records one for every
  /// window, menu, menu item, button, checkbox, field, and text, so that a
  /// script finds what to click by its label, and a check asks what was
  /// shown, without knowing the toolkit. See ToolUiContext::GetItemsDrawn().
  struct ToolUiItem
  {
    ToolUiItemKind kind = ToolUiItemKind::Window;
    std::string label;
    ToolUiRect rect;

    bool operator==(const ToolUiItem &other) const = default;
  };
} // neon

#endif //TOOL_UI_ITEM_HPP
