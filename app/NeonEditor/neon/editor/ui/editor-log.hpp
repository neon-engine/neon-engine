#ifndef EDITOR_LOG_HPP
#define EDITOR_LOG_HPP

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include <neon/logging/logging-system.hpp>
#include <neon/tool-ui/tool-ui-context.hpp>

namespace neon
{
  /// The window of the editor that shows the log as it is written: the
  /// last lines of every logger, the newest at the bottom, where the
  /// window scrolls to whenever a line is added.
  class EditorLog final
  {
    LoggingSystem *_logging_system;
    std::vector<std::string> _lines;
    std::uint64_t _revision = 0;
    bool _changed = false;

  public:
    static constexpr std::string_view title = "Log";

    explicit EditorLog(LoggingSystem *logging_system);

    /// Declares the window with the lines in it.
    void Draw(ToolUiContext &ui);

    /// The lines as they were last shown.
    [[nodiscard]] const std::vector<std::string> &GetLines() const { return _lines; }
  };
} // neon

#endif //EDITOR_LOG_HPP
