#ifndef TOOL_UI_POINT_HPP
#define TOOL_UI_POINT_HPP

namespace neon
{
  /// A place in the tool interface, in points of the window, from its
  /// left top.
  struct ToolUiPoint
  {
    float x = 0.0f;
    float y = 0.0f;

    bool operator==(const ToolUiPoint &other) const = default;
  };
} // neon

#endif //TOOL_UI_POINT_HPP
