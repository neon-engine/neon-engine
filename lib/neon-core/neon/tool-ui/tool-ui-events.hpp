#ifndef TOOL_UI_EVENTS_HPP
#define TOOL_UI_EVENTS_HPP

#include <optional>
#include <string>
#include <vector>

#include <neon/input/key.hpp>

#include "tool-ui-point.hpp"

namespace neon
{
  /// Input for the tool interface in one frame, in place of or next to the
  /// devices: what a script of the editor feeds, see
  /// ToolUiContext::FeedEvents(). The backend takes it in the way it takes
  /// the events of the window, so that what a script does is what a hand
  /// would do. What is left out does not happen in the frame: a pointer
  /// that is not moved stays, a button that is not pressed or released
  /// stays as it is.
  struct ToolUiEvents
  {
    /// Where the pointer goes, in points of the window.
    std::optional<ToolUiPoint> pointer;

    /// The primary button of the pointer goes down (true) or up (false).
    std::optional<bool> button_down;

    /// Keys that go down and up, in order, with what is held with them.
    std::vector<KeyEvent> keys;

    /// Text that is typed into the field that has the focus.
    std::string text;

    /// The wheel, in notches to the right and down.
    float wheel_x = 0.0f;
    float wheel_y = 0.0f;

    [[nodiscard]] bool IsEmpty() const
    {
      return !pointer.has_value() && !button_down.has_value() && keys.empty() && text.empty() && wheel_x == 0.0f &&
             wheel_y == 0.0f;
    }
  };
} // neon

#endif //TOOL_UI_EVENTS_HPP
