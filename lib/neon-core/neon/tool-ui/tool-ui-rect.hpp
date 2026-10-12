#ifndef TOOL_UI_RECT_HPP
#define TOOL_UI_RECT_HPP

#include "tool-ui-point.hpp"

namespace neon
{
  /// Where something of the tool interface was drawn, in points of the
  /// window: its left top, and its size.
  struct ToolUiRect
  {
    float x = 0.0f;
    float y = 0.0f;
    float width = 0.0f;
    float height = 0.0f;

    /// The middle, where a pointer is put to hit it.
    [[nodiscard]] ToolUiPoint Center() const { return {x + width / 2.0f, y + height / 2.0f}; }

    [[nodiscard]] bool Contains(const ToolUiPoint &point) const
    {
      return point.x >= x && point.x < x + width && point.y >= y && point.y < y + height;
    }

    bool operator==(const ToolUiRect &other) const = default;
  };
} // neon

#endif //TOOL_UI_RECT_HPP
